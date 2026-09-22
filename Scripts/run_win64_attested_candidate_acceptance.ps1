param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [ValidateSet("Development", "Shipping")]
    [string]$Configuration = "Shipping",
    [string]$Version = "",
    [string]$PackageDirectory = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ConfigPath = Join-Path $ProjectRoot "Config\DefaultGame.ini"
$BaseRunner = Join-Path $PSScriptRoot "run_win64_candidate_acceptance.ps1"
$HillHaulEvaluator = Join-Path $PSScriptRoot "evaluate_fieldmaster_hill_haul_runtime.ps1"
$HudEvaluator = Join-Path $PSScriptRoot "evaluate_fieldmaster_hud_runtime.ps1"
$Attestor = Join-Path $PSScriptRoot "write_win64_candidate_attestation.ps1"
$ArchiveVerifier = Join-Path $PSScriptRoot "verify_win64_candidate_archive.ps1"
$TrailerArchiveVerifier = Join-Path $PSScriptRoot "verify_authored_trailer_archive_evidence.ps1"
if (-not (Test-Path $ConfigPath -PathType Leaf)) { throw "Config/DefaultGame.ini missing." }
if (-not (Test-Path $BaseRunner -PathType Leaf)) { throw "Base candidate acceptance runner missing: $BaseRunner" }
if (-not (Test-Path $HillHaulEvaluator -PathType Leaf)) { throw "Hill-haul runtime evaluator missing: $HillHaulEvaluator" }
if (-not (Test-Path $HudEvaluator -PathType Leaf)) { throw "Fieldmaster HUD runtime evaluator missing: $HudEvaluator" }
if (-not (Test-Path $Attestor -PathType Leaf)) { throw "Candidate attestation writer missing: $Attestor" }
if (-not (Test-Path $ArchiveVerifier -PathType Leaf)) { throw "Candidate archive verifier missing: $ArchiveVerifier" }
if (-not (Test-Path $TrailerArchiveVerifier -PathType Leaf)) { throw "Authored trailer archive verifier missing: $TrailerArchiveVerifier" }

$ini = Get-Content -Raw $ConfigPath
$match = [regex]::Match($ini, '(?m)^ProjectVersion=(.+)$')
if (-not $match.Success) { throw "ProjectVersion is missing from Config/DefaultGame.ini." }
$ProjectVersion = $match.Groups[1].Value.Trim()
if ([string]::IsNullOrWhiteSpace($Version)) { $Version = $ProjectVersion }
elseif ($Version -ne $ProjectVersion) { throw "Acceptance version '$Version' does not match ProjectVersion '$ProjectVersion'." }

if ([string]::IsNullOrWhiteSpace($PackageDirectory)) {
    $PackageDirectory = Join-Path $ProjectRoot "Releases\GTT-$Version-Windows-$Configuration"
}
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)

Write-Host "[GTT][ATTESTED] Running full exact-candidate technical/rendered acceptance..."
& $BaseRunner -EngineRoot $EngineRoot -Configuration $Configuration -Version $Version -PackageDirectory $PackageDirectory
if ($LASTEXITCODE -ne 0) { throw "Base Win64 candidate acceptance failed with exit code $LASTEXITCODE." }

$summaryPath = Join-Path $PackageDirectory "WIN64_ACCEPTANCE_SUMMARY.json"
if (-not (Test-Path $summaryPath -PathType Leaf)) { throw "Base acceptance summary missing: $summaryPath" }
$summary = Get-Content -Raw $summaryPath | ConvertFrom-Json
if ($summary.result -ne "PASS" -or $summary.version -ne $Version -or $summary.configuration -ne $Configuration) {
    throw "Base acceptance summary does not match requested candidate identity."
}
if ([string]$summary.git_sha -notmatch '^[0-9a-fA-F]{40}$') { throw "Base acceptance summary does not contain an exact Git SHA." }
if ($summary.human_visual_review -ne "REQUIRED" -or [bool]$summary.demo_release_authorized) {
    throw "Base acceptance crossed the human-review release boundary."
}

$runtimeLog = Join-Path $PackageDirectory "GTT_RUNTIME.log"
Write-Host "[GTT][ATTESTED] Evaluating packaged Fieldmaster hill-haul/thermal evidence..."
& $HillHaulEvaluator -PackageDirectory $PackageDirectory -RuntimeLog $runtimeLog -ExpectedGitSha ([string]$summary.git_sha)
if ($LASTEXITCODE -ne 0) { throw "Fieldmaster hill-haul runtime evidence failed with exit code $LASTEXITCODE." }

$hillHaulPath = Join-Path $PackageDirectory "FIELDMASTER_HILL_HAUL_RUNTIME.json"
if (-not (Test-Path $hillHaulPath -PathType Leaf)) { throw "Fieldmaster hill-haul runtime evidence missing: $hillHaulPath" }
$hillHaul = Get-Content -Raw $hillHaulPath | ConvertFrom-Json
if ($hillHaul.result -ne "PASS" -or $hillHaul.git_sha -ne $summary.git_sha -or $hillHaul.version -ne $Version) {
    throw "Fieldmaster hill-haul runtime evidence does not match the exact candidate identity."
}
if ([int]$hillHaul.loaded_trailer_samples -lt 2 -or [int]$hillHaul.assist_samples -lt 1 -or [int]$hillHaul.thermal_samples -lt 1) {
    throw "Fieldmaster hill-haul runtime evidence is missing required loaded/assist/thermal coverage."
}
$summary | Add-Member -NotePropertyName fieldmaster_hill_haul_runtime -NotePropertyValue "PASS" -Force
$summary | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $summaryPath

# Bind the driver-facing HUD safety contract to the same packaged runtime log and exact candidate.
Write-Host "[GTT][ATTESTED] Evaluating packaged Native Fieldmaster HUD safety evidence..."
& $HudEvaluator -PackageDirectory $PackageDirectory -RuntimeLog $runtimeLog -ExpectedGitSha ([string]$summary.git_sha)
if ($LASTEXITCODE -ne 0) { throw "Fieldmaster HUD runtime evidence failed with exit code $LASTEXITCODE." }

$hudPath = Join-Path $PackageDirectory "FIELDMASTER_HUD_RUNTIME.json"
if (-not (Test-Path $hudPath -PathType Leaf)) { throw "Fieldmaster HUD runtime evidence missing: $hudPath" }
$hud = Get-Content -Raw $hudPath | ConvertFrom-Json
if ($hud.schema -ne "gtt.fieldmaster-hud-runtime.v1" -or $hud.result -ne "PASS" -or $hud.git_sha -ne $summary.git_sha -or $hud.version -ne $Version) {
    throw "Fieldmaster HUD runtime evidence does not match the exact candidate identity."
}
if ([int]$hud.telemetry_samples -lt 2 -or [int]$hud.visible_alert_samples -lt 1) {
    throw "Fieldmaster HUD runtime evidence lacks required telemetry/visible-alert coverage."
}
$summary = Get-Content -Raw $summaryPath | ConvertFrom-Json
$summary | Add-Member -NotePropertyName fieldmaster_hud_runtime -NotePropertyValue "PASS" -Force
$summary | Add-Member -NotePropertyName fieldmaster_hud_visible_alert_samples -NotePropertyValue ([int]$hud.visible_alert_samples) -Force
$summary | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $summaryPath

Write-Host "[GTT][ATTESTED] Sealing final package/evidence identity and rebuilding the candidate archive..."
& $Attestor -PackageDirectory $PackageDirectory -Version $Version -Configuration $Configuration -ExpectedGitSha ([string]$summary.git_sha)
if ($LASTEXITCODE -ne 0) { throw "Win64 candidate attestation failed with exit code $LASTEXITCODE." }

# Verify the produced ZIP as a consumer would receive it. Sidecars live outside the archive
# and are removed before each verification so stale PASS evidence can never mask failure.
$archiveVerificationPath = "$PackageDirectory.zip.verify.json"
if (Test-Path $archiveVerificationPath) { Remove-Item -Force $archiveVerificationPath }
Write-Host "[GTT][ATTESTED] Round-trip verifying sealed Win64 candidate archive..."
& $ArchiveVerifier -PackageDirectory $PackageDirectory -Version $Version -Configuration $Configuration -ExpectedGitSha ([string]$summary.git_sha)
$archiveVerifierExit = $LASTEXITCODE
if ($archiveVerifierExit -ne 0) {
    throw "Win64 candidate archive verification failed with exit code $archiveVerifierExit."
}
if (-not (Test-Path $archiveVerificationPath -PathType Leaf)) { throw "WIN64_ARCHIVE_VERIFICATION evidence missing: $archiveVerificationPath" }
$archiveVerification = Get-Content -Raw $archiveVerificationPath | ConvertFrom-Json
if ($archiveVerification.schema -ne "gtt.win64-candidate-archive-verification.v1" -or
    $archiveVerification.result -ne "PASS" -or
    $archiveVerification.git_sha -ne $summary.git_sha -or
    $archiveVerification.version -ne $Version -or
    $archiveVerification.configuration -ne $Configuration -or
    $archiveVerification.human_visual_review -ne "REQUIRED" -or
    [bool]$archiveVerification.demo_release_authorized) {
    throw "WIN64_ARCHIVE_VERIFICATION does not match the exact sealed candidate/release boundary."
}

# Gate 3 consumer-side proof: independently inspect the sealed ZIP and require the exact
# authored skeletal trailer editor contract (wheel bones, final hitch/axle sockets,
# PhysicsAsset and project-owned source hash) to survive compression unchanged.
$trailerArchiveVerificationPath = "$PackageDirectory.zip.trailer-editor-verify.json"
if (Test-Path $trailerArchiveVerificationPath) { Remove-Item -Force $trailerArchiveVerificationPath }
Write-Host "[GTT][ATTESTED] Verifying authored trailer editor evidence from sealed archive..."
& $TrailerArchiveVerifier -PackageDirectory $PackageDirectory -Version $Version -Configuration $Configuration -ExpectedGitSha ([string]$summary.git_sha)
$trailerArchiveVerifierExit = $LASTEXITCODE
if ($trailerArchiveVerifierExit -ne 0) {
    throw "Authored trailer sealed-archive verification failed with exit code $trailerArchiveVerifierExit."
}
if (-not (Test-Path $trailerArchiveVerificationPath -PathType Leaf)) {
    throw "Authored trailer sealed-archive verification evidence missing: $trailerArchiveVerificationPath"
}
$trailerArchiveVerification = Get-Content -Raw $trailerArchiveVerificationPath | ConvertFrom-Json
if ($trailerArchiveVerification.schema -ne "gtt.authored-trailer-archive-evidence.v1" -or
    $trailerArchiveVerification.result -ne "PASS" -or
    $trailerArchiveVerification.git_sha -ne $summary.git_sha -or
    $trailerArchiveVerification.version -ne $Version -or
    $trailerArchiveVerification.configuration -ne $Configuration -or
    $trailerArchiveVerification.archive_sha256 -ne $archiveVerification.archive_sha256 -or
    $trailerArchiveVerification.authored_skeletal_mesh -ne "PASS" -or
    $trailerArchiveVerification.authored_skeleton -ne "PASS" -or
    $trailerArchiveVerification.authored_physics_asset -ne "PASS" -or
    $trailerArchiveVerification.final_hitch_socket -ne "PASS" -or
    $trailerArchiveVerification.final_axle_sockets -ne "PASS" -or
    $trailerArchiveVerification.skeletal_wheel_bones -ne "PASS" -or
    $trailerArchiveVerification.human_visual_review -ne "REQUIRED" -or
    [bool]$trailerArchiveVerification.demo_release_authorized) {
    throw "Authored trailer sealed-archive verification does not match the exact candidate/current gate boundary."
}

Write-Host "[GTT][ATTESTED] PASS: sealed technical candidate, archive round-trip and authored trailer editor evidence are ready for human visual review only."
Write-Host "[GTT][ATTESTED] Demo Release remains unauthorized until the separate human-reviewed release gate passes."
