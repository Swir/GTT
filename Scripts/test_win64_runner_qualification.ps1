$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$Root = Split-Path -Parent $PSScriptRoot
$Qualifier = Join-Path $PSScriptRoot "qualify_win64_runner.ps1"
$Config = Join-Path $Root "Config\DefaultGame.ini"

if (-not (Test-Path $Qualifier -PathType Leaf)) { throw "Qualifier missing: $Qualifier" }
if (-not (Test-Path $Config -PathType Leaf)) { throw "Config missing: $Config" }

$ini = Get-Content -Raw $Config
$versionMatch = [regex]::Match($ini, '(?m)^ProjectVersion=(.+)$')
if (-not $versionMatch.Success) { throw "ProjectVersion missing." }
$Version = $versionMatch.Groups[1].Value.Trim()
$Sha = (& git -C $Root rev-parse HEAD 2>$null).Trim()
if ($LASTEXITCODE -ne 0 -or $Sha -notmatch '^[0-9a-fA-F]{40}$') { throw "Exact git SHA unavailable." }

$TestRoot = Join-Path $env:RUNNER_TEMP "gtt-win64-runner-qualification-fixture"
$EngineRoot = Join-Path $TestRoot "UE_5.8"
$BuildDir = Join-Path $EngineRoot "Engine\Build"
$BatchDir = Join-Path $BuildDir "BatchFiles"
$BinaryDir = Join-Path $EngineRoot "Engine\Binaries\Win64"
$UbtDir = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool"
$OutDir = Join-Path $TestRoot "out"

function Write-FakeEngine {
    New-Item -ItemType Directory -Force -Path $BatchDir,$BinaryDir,$UbtDir,$OutDir | Out-Null
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

function Invoke-Qualifier {
    param(
        [string]$Name,
        [string]$ExpectedSha,
        [string]$ExpectedVer,
        [bool]$ExpectPass
    )
    $Output = Join-Path $OutDir "$Name.json"
    & $Qualifier `
        -EngineRoot $EngineRoot `
        -ProjectFile (Join-Path $Root "GTT.uproject") `
        -OutputPath $Output `
        -ExpectedGitSha $ExpectedSha `
        -ExpectedVersion $ExpectedVer `
        -MinimumFreeGiB 1 `
        -SkipEditorProbe
    $exitCode = $LASTEXITCODE

    if (-not (Test-Path $Output -PathType Leaf)) { throw "$Name did not emit qualification JSON." }
    $doc = Get-Content -Raw $Output | ConvertFrom-Json
    if ($doc.schema -ne "gtt.win64-runner-qualification.v1") { throw "$Name schema mismatch." }
    if ($doc.human_visual_review -ne "REQUIRED" -or [bool]$doc.demo_release_authorized) {
        throw "$Name crossed the human-review / release authorization boundary."
    }
    if ($doc.git_sha -ne $Sha.ToLowerInvariant() -or $doc.version -ne $Version) {
        throw "$Name candidate identity mismatch."
    }

    if ($ExpectPass) {
        if ($exitCode -ne 0 -or $doc.result -ne "PASS" -or $doc.preflight -ne "PASS") {
            throw "$Name expected PASS, got exit=$exitCode result=$($doc.result) preflight=$($doc.preflight)."
        }
    } else {
        if ($exitCode -eq 0 -or $doc.result -ne "FAIL") {
            throw "$Name expected FAIL, got exit=$exitCode result=$($doc.result)."
        }
    }
}

try {
    if (Test-Path $TestRoot) { Remove-Item -Recurse -Force $TestRoot }
    Write-FakeEngine

    Invoke-Qualifier -Name "positive" -ExpectedSha $Sha -ExpectedVer $Version -ExpectPass $true
    Invoke-Qualifier -Name "wrong-version" -ExpectedSha $Sha -ExpectedVer "0.0.0-invalid" -ExpectPass $false
    Invoke-Qualifier -Name "wrong-sha" -ExpectedSha ("0" * 40) -ExpectedVer $Version -ExpectPass $false

    Write-Host "[GTT][RUNNER-TEST] PASS: positive qualification plus exact-version/exact-SHA negative fixtures."
}
finally {
    if (Test-Path $TestRoot) { Remove-Item -Recurse -Force $TestRoot }
}

# Expected negative fixtures intentionally leave LASTEXITCODE non-zero. Reset the
# script result explicitly so a successful fail-closed test does not poison CI.
exit 0
