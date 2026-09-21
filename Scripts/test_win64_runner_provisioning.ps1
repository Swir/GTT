$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Split-Path -Parent $PSScriptRoot
$Provisioner = Join-Path $PSScriptRoot "provision_win64_ue58_runner.ps1"
if (-not (Test-Path $Provisioner -PathType Leaf)) { throw "Provisioner missing: $Provisioner" }

$TestRoot = Join-Path $env:RUNNER_TEMP "gtt-win64-runner-provisioning-fixture"
$RunnerDir = Join-Path $TestRoot "runner"
$EngineRoot = Join-Path $TestRoot "UE_5.8"
$BuildDir = Join-Path $EngineRoot "Engine\Build"
$BatchDir = Join-Path $BuildDir "BatchFiles"
$BinaryDir = Join-Path $EngineRoot "Engine\Binaries\Win64"
$UbtDir = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool"
$OutDir = Join-Path $TestRoot "out"

function Write-Fixture {
    New-Item -ItemType Directory -Force -Path $RunnerDir,$BuildDir,$BatchDir,$BinaryDir,$UbtDir,$OutDir | Out-Null
    Set-Content -Encoding ASCII -Path (Join-Path $RunnerDir "config.cmd") -Value "@echo off`r`nexit /b 0"
    Set-Content -Encoding ASCII -Path (Join-Path $RunnerDir "run.cmd") -Value "@echo off`r`nexit /b 0"
    Set-Content -Encoding ASCII -Path (Join-Path $RunnerDir "svc.cmd") -Value "@echo off`r`nexit /b 0"
    Set-Content -Encoding ASCII -Path (Join-Path $BatchDir "RunUAT.bat") -Value "@echo off`r`nexit /b 0"
    Set-Content -Encoding ASCII -Path (Join-Path $BinaryDir "UnrealEditor-Cmd.exe") -Value "fixture"
    Set-Content -Encoding ASCII -Path (Join-Path $UbtDir "UnrealBuildTool.dll") -Value "fixture"
    [ordered]@{
        MajorVersion = 5
        MinorVersion = 8
        PatchVersion = 0
        Changelist = 0
        CompatibleChangelist = 0
        IsLicenseeVersion = 0
        IsPromotedBuild = 0
        BranchName = "fixture"
    } | ConvertTo-Json | Set-Content -Encoding UTF8 (Join-Path $BuildDir "Build.version")
}

function Read-Report {
    param([string]$Path)
    if (-not (Test-Path $Path -PathType Leaf)) { throw "Expected provisioning report missing: $Path" }
    return Get-Content -Raw $Path | ConvertFrom-Json
}

function Assert-TruthBoundary {
    param($Doc)
    if ($Doc.schema -ne "gtt.win64-runner-provisioning.v1") { throw "Provisioning schema mismatch." }
    if (-not [bool]$Doc.qualification_required) { throw "Provisioning must require real qualification." }
    if ([bool]$Doc.roadmap_gate_closed) { throw "Provisioning must not close a roadmap gate." }
    if ([bool]$Doc.native_chaos_runtime_verified -or [bool]$Doc.authored_trailer_runtime_verified -or [bool]$Doc.packaged_exe_smoke_verified) {
        throw "Provisioning crossed runtime evidence boundaries."
    }
    if ($Doc.human_visual_review -ne "REQUIRED" -or [bool]$Doc.demo_release_authorized) {
        throw "Provisioning crossed visual/release authorization boundaries."
    }
    $labels = @($Doc.required_labels)
    foreach ($required in @("self-hosted","windows","x64","unreal-5.8")) {
        if ($labels -notcontains $required) { throw "Missing required label in report: $required" }
    }
}

try {
    if (Test-Path $TestRoot) { Remove-Item -Recurse -Force $TestRoot }
    Write-Fixture

    $PositiveOut = Join-Path $OutDir "positive.json"
    & $Provisioner -RunnerDirectory $RunnerDir -EngineRoot $EngineRoot -OutputPath $PositiveOut -PlanOnly
    $positiveExit = $LASTEXITCODE
    $positive = Read-Report $PositiveOut
    Assert-TruthBoundary $positive
    if ($positiveExit -ne 0 -or $positive.result -ne "PASS" -or [bool]$positive.mutation_performed) {
        throw "Positive PlanOnly expected PASS/no mutation, got exit=$positiveExit result=$($positive.result) mutation=$($positive.mutation_performed)."
    }

    Remove-Item -Force (Join-Path $BinaryDir "UnrealEditor-Cmd.exe")
    $MissingEditorOut = Join-Path $OutDir "missing-editor.json"
    & $Provisioner -RunnerDirectory $RunnerDir -EngineRoot $EngineRoot -OutputPath $MissingEditorOut -PlanOnly
    $missingEditorExit = $LASTEXITCODE
    $missingEditor = Read-Report $MissingEditorOut
    Assert-TruthBoundary $missingEditor
    if ($missingEditorExit -eq 0 -or $missingEditor.result -ne "FAIL") {
        throw "Missing editor fixture must fail closed."
    }

    Set-Content -Encoding ASCII -Path (Join-Path $BinaryDir "UnrealEditor-Cmd.exe") -Value "fixture"
    $oldToken = $env:GTT_GITHUB_RUNNER_TOKEN
    Remove-Item Env:GTT_GITHUB_RUNNER_TOKEN -ErrorAction SilentlyContinue
    $NoTokenOut = Join-Path $OutDir "no-token.json"
    & $Provisioner -RunnerDirectory $RunnerDir -EngineRoot $EngineRoot -OutputPath $NoTokenOut -Configure
    $noTokenExit = $LASTEXITCODE
    $noToken = Read-Report $NoTokenOut
    Assert-TruthBoundary $noToken
    if ($noTokenExit -eq 0 -or $noToken.result -ne "FAIL" -or (Test-Path (Join-Path $RunnerDir ".runner") -PathType Leaf)) {
        throw "Configure without short-lived token must fail closed without registering."
    }
    if ($null -ne $oldToken) {
        $env:GTT_GITHUB_RUNNER_TOKEN = $oldToken
    }

    Write-Host "[GTT][RUNNER-PROVISION-TEST] PASS: readiness, missing-UE-tool and token fail-closed fixtures."
}
finally {
    if (Test-Path $TestRoot) { Remove-Item -Recurse -Force $TestRoot }
}

# Negative fixtures intentionally leave LASTEXITCODE non-zero. A successful
# fail-closed test suite itself must return success to CI.
exit 0
