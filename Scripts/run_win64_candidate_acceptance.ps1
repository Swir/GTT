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
$ProjectFile = Join-Path $ProjectRoot "GTT.uproject"
$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

function Invoke-GTTScript {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$false)][object[]]$Arguments = @()
    )
    $resolved = Join-Path $PSScriptRoot $Path
    if (-not (Test-Path $resolved -PathType Leaf)) { throw "Required acceptance script missing: $resolved" }
    Write-Host "[GTT][ACCEPTANCE] $Path $($Arguments -join ' ')"
    & $resolved @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Path failed with exit code $LASTEXITCODE" }
}

if (-not (Test-Path $ConfigPath -PathType Leaf)) { throw "Config/DefaultGame.ini missing." }
if (-not (Test-Path $ProjectFile -PathType Leaf)) { throw "GTT.uproject missing." }

$ini = Get-Content -Raw $ConfigPath
$match = [regex]::Match($ini, '(?m)^ProjectVersion=(.+)$')
if (-not $match.Success) { throw "ProjectVersion is missing from Config/DefaultGame.ini." }
$ProjectVersion = $match.Groups[1].Value.Trim()
if ([string]::IsNullOrWhiteSpace($ProjectVersion)) { throw "ProjectVersion must not be empty." }
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = $ProjectVersion
} elseif ($Version -ne $ProjectVersion) {
    throw "Acceptance version '$Version' does not match ProjectVersion '$ProjectVersion'."
}

$GitSha = (& git -C $ProjectRoot rev-parse HEAD 2>$null).Trim()
if ($LASTEXITCODE -ne 0 -or $GitSha -notmatch '^[0-9a-fA-F]{40}$') { throw "Unable to resolve exact 40-character git SHA." }
$dirty = (& git -C $ProjectRoot status --porcelain --untracked-files=no 2>$null)
if ($LASTEXITCODE -ne 0) { throw "Unable to inspect git working tree." }
if (-not [string]::IsNullOrWhiteSpace(($dirty -join "`n"))) {
    throw "Win64 acceptance requires a clean tracked working tree so evidence can be bound to an exact commit."
}

if ([string]::IsNullOrWhiteSpace($PackageDirectory)) {
    $PackageDirectory = Join-Path $ProjectRoot "Releases\GTT-$Version-Windows-$Configuration"
}
$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
$EvidenceRoot = Split-Path -Parent $PackageDirectory
New-Item -ItemType Directory -Force -Path $EvidenceRoot | Out-Null

$PreflightFile = "$PackageDirectory.preflight.json"
$ImportFile = "$PackageDirectory.authored-trailer-import.json"
$RuntimeLog = Join-Path $PackageDirectory "GTT_RUNTIME.log"
$VisualRuntimeLog = Join-Path $PackageDirectory "GTT_VISUAL_RUNTIME.log"
$FinalAsset = Join-Path $ProjectRoot "Content\GTT\Vehicles\Trailer\SK_GTT_FarmTrailer.uasset"
$PreviousGithubSha = $env:GITHUB_SHA
$env:GITHUB_SHA = $GitSha

try {
    Write-Host "[GTT][ACCEPTANCE] Exact candidate: $GitSha / version $Version / $Configuration"

    Invoke-GTTScript "preflight_win64_unreal.ps1" @(
        "-EngineRoot", $EngineRoot,
        "-ProjectFile", $ProjectFile,
        "-OutputPath", $PreflightFile
    )

    Invoke-GTTScript "import_gtt_farm_trailer_unreal.ps1" @(
        "-UnrealEditorCmd", $EditorCmd,
        "-Project", $ProjectFile
    )
    if (-not (Test-Path $FinalAsset -PathType Leaf)) {
        throw "Authored trailer skeletal asset missing after UE import: $FinalAsset"
    }

    $importEvidence = [ordered]@{
        schema = "gtt.authored-trailer-import.v1"
        result = "PASS"
        git_sha = $GitSha
        version = $Version
        asset = "Content/GTT/Vehicles/Trailer/SK_GTT_FarmTrailer.uasset"
        engine = "Unreal Engine 5.8"
        generated_utc = (Get-Date).ToUniversalTime().ToString("o")
    }
    $importEvidence | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 $ImportFile

    Invoke-GTTScript "package_windows.ps1" @(
        "-EngineRoot", $EngineRoot,
        "-Configuration", $Configuration,
        "-ArchiveDirectory", $PackageDirectory,
        "-Version", $Version,
        "-SkipZip"
    )
    Copy-Item -Force $ImportFile (Join-Path $PackageDirectory "AUTHORED_TRAILER_IMPORT.json")

    Invoke-GTTScript "smoke_test_windows.ps1" @(
        "-PackageDirectory", $PackageDirectory,
        "-Version", $Version,
        "-MinimumAliveSeconds", 472,
        "-LaunchTimeoutSeconds", 505
    )

    $runtimeEvaluators = @(
        "evaluate_demo_scenario.ps1",
        "evaluate_packaged_gameplay_smoke.ps1",
        "evaluate_native_chaos_runtime.ps1",
        "evaluate_drivetrain_scenario.ps1",
        "evaluate_authored_trailer_runtime.ps1",
        "evaluate_farm_cargo_runtime.ps1",
        "evaluate_farm_cargo_recovery_runtime.ps1",
        "evaluate_farm_cargo_breakdown_runtime.ps1",
        "evaluate_farm_cargo_dispatch_runtime.ps1",
        "evaluate_farm_cargo_dispatch_persistence_runtime.ps1",
        "evaluate_farm_cargo_workshop_recovery_runtime.ps1",
        "evaluate_workshop_hours_runtime.ps1",
        "evaluate_workshop_queue_runtime.ps1",
        "evaluate_workshop_capacity_runtime.ps1",
        "evaluate_workshop_priority_pickup_runtime.ps1"
    )
    foreach ($script in $runtimeEvaluators) {
        $args = @("-PackageDirectory", $PackageDirectory, "-RuntimeLog", $RuntimeLog, "-ExpectedGitSha", $GitSha)
        if ($script -eq "evaluate_packaged_gameplay_smoke.ps1") { $args += @("-MinimumRuntimeSeconds", 472) }
        Invoke-GTTScript $script $args
    }

    Invoke-GTTScript "validate_windows_package.ps1" @(
        "-PackageDirectory", $PackageDirectory,
        "-Configuration", $Configuration,
        "-Version", $Version
    )

    $import = Get-Content -Raw (Join-Path $PackageDirectory "AUTHORED_TRAILER_IMPORT.json") | ConvertFrom-Json
    if ($import.schema -ne "gtt.authored-trailer-import.v1" -or $import.result -ne "PASS" -or
        $import.git_sha -ne $GitSha -or $import.version -ne $Version) {
        throw "Authored trailer import evidence is not bound to this exact candidate."
    }

    Invoke-GTTScript "evaluate_demo_candidate.ps1" @(
        "-PackageDirectory", $PackageDirectory,
        "-RuntimeLog", $RuntimeLog,
        "-ExpectedGitSha", $GitSha
    )

    foreach ($promotion in @(
        "promote_demo_gate_workshop_recovery.ps1",
        "promote_demo_gate_workshop_hours.ps1",
        "promote_demo_gate_workshop_queue.ps1",
        "promote_demo_gate_workshop_capacity.ps1",
        "promote_demo_gate_workshop_priority_pickup.ps1"
    )) {
        Invoke-GTTScript $promotion @("-PackageDirectory", $PackageDirectory, "-ExpectedGitSha", $GitSha)
    }

    $gatePath = Join-Path $PackageDirectory "DEMO_TECHNICAL_GATE.json"
    if (-not (Test-Path $gatePath -PathType Leaf)) { throw "DEMO_TECHNICAL_GATE.json missing." }
    $gate = Get-Content -Raw $gatePath | ConvertFrom-Json
    if ([int]$gate.schema -ne 17 -or $gate.result -ne "PASS") {
        throw "Demo technical gate is not a PASS schema-17 candidate."
    }

    Invoke-GTTScript "capture_demo_visual_evidence.ps1" @(
        "-PackageDirectory", $PackageDirectory,
        "-Version", $Version,
        "-MinimumAliveSeconds", 178,
        "-LaunchTimeoutSeconds", 205
    )
    Invoke-GTTScript "evaluate_demo_visual_evidence.ps1" @(
        "-PackageDirectory", $PackageDirectory,
        "-RuntimeLog", $VisualRuntimeLog,
        "-ExpectedGitSha", $GitSha
    )

    $visualEvidencePath = Join-Path $PackageDirectory "DEMO_VISUAL_EVIDENCE.json"
    if (-not (Test-Path $visualEvidencePath -PathType Leaf)) { throw "DEMO_VISUAL_EVIDENCE.json missing." }
    $visual = Get-Content -Raw $visualEvidencePath | ConvertFrom-Json
    if ($visual.result -ne "PASS") { throw "Rendered visual evidence did not PASS automated validation." }

    $shots = @(Get-ChildItem (Join-Path $PackageDirectory "DemoVisualEvidence") -File -Filter "GTT_visual_*.png")
    if ($shots.Count -ne 5) { throw "Expected exactly five rendered review screenshots, found $($shots.Count)." }

    $summary = [ordered]@{
        schema = "gtt.win64-candidate-acceptance.v1"
        result = "PASS"
        game = "Grand Theft Tractor"
        git_sha = $GitSha
        version = $Version
        configuration = $Configuration
        engine = "Unreal Engine 5.8"
        authored_trailer_import = "PASS"
        packaged_exe_smoke = "PASS"
        native_chaos_runtime = "PASS"
        authored_trailer_runtime = "PASS"
        technical_gate_schema = 17
        technical_gate = "PASS"
        rendered_visual_evidence = "PASS"
        rendered_screenshot_count = $shots.Count
        human_visual_review = "REQUIRED"
        demo_release_authorized = $false
        generated_utc = (Get-Date).ToUniversalTime().ToString("o")
    }
    $summaryPath = Join-Path $PackageDirectory "WIN64_ACCEPTANCE_SUMMARY.json"
    $summary | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 $summaryPath

    $zipPath = "$PackageDirectory.zip"
    if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
    Compress-Archive -Path (Join-Path $PackageDirectory "*") -DestinationPath $zipPath -CompressionLevel Optimal
    $zipHash = (Get-FileHash -Algorithm SHA256 -Path $zipPath).Hash.ToLowerInvariant()
    Set-Content -Encoding ASCII -Path "$zipPath.sha256" -Value "$zipHash  $([IO.Path]::GetFileName($zipPath))"

    Write-Host "[GTT][ACCEPTANCE] PASS: exact candidate is technically qualified for human visual review."
    Write-Host "[GTT][ACCEPTANCE] Human visual review is still REQUIRED; Demo Release is NOT authorized by this script."
    Write-Host "[GTT][ACCEPTANCE] Summary: $summaryPath"
    Write-Host "[GTT][ACCEPTANCE] Candidate ZIP: $zipPath"
}
finally {
    $env:GITHUB_SHA = $PreviousGithubSha
}
