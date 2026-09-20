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
$Attestor = Join-Path $PSScriptRoot "write_win64_candidate_attestation.ps1"
if (-not (Test-Path $ConfigPath -PathType Leaf)) { throw "Config/DefaultGame.ini missing." }
if (-not (Test-Path $BaseRunner -PathType Leaf)) { throw "Base candidate acceptance runner missing: $BaseRunner" }
if (-not (Test-Path $HillHaulEvaluator -PathType Leaf)) { throw "Hill-haul runtime evaluator missing: $HillHaulEvaluator" }
if (-not (Test-Path $Attestor -PathType Leaf)) { throw "Candidate attestation writer missing: $Attestor" }

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

# 0.1.65 closes a source/evidence gap without pretending a Windows run occurred.
# The exact packaged candidate must demonstrate a loaded trailer, real hill assist
# and trailer-brake thermal behavior in the same runtime log before attestation.
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

Write-Host "[GTT][ATTESTED] Sealing final package/evidence identity and rebuilding the candidate archive..."
& $Attestor -PackageDirectory $PackageDirectory -Version $Version -Configuration $Configuration -ExpectedGitSha ([string]$summary.git_sha)
if ($LASTEXITCODE -ne 0) { throw "Win64 candidate attestation failed with exit code $LASTEXITCODE." }

Write-Host "[GTT][ATTESTED] PASS: sealed technical candidate is ready for human visual review only."
Write-Host "[GTT][ATTESTED] Demo Release remains unauthorized until the separate human-reviewed release gate passes."
