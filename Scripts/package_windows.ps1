param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [ValidateSet("Development", "Shipping")]
    [string]$Configuration = "Shipping",
    [string]$ArchiveDirectory = "",
    [string]$Version = "0.1.15",
    [switch]$SkipZip
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot "GTT.uproject"
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$Validator = Join-Path $PSScriptRoot "validate_windows_package.ps1"
$Preflight = Join-Path $PSScriptRoot "preflight_win64_unreal.ps1"

if (-not (Test-Path $ProjectFile)) { throw "GTT.uproject was not found at $ProjectFile" }
if (-not (Test-Path $Validator)) { throw "Package validator was not found at $Validator" }
if (-not (Test-Path $Preflight)) { throw "Win64 Unreal preflight was not found at $Preflight" }
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) { $ArchiveDirectory = Join-Path $ProjectRoot "Releases\GTT-$Version-Windows-$Configuration" }

$ArchiveDirectory = [System.IO.Path]::GetFullPath($ArchiveDirectory)
$ArchiveParent = Split-Path -Parent $ArchiveDirectory
New-Item -ItemType Directory -Force -Path $ArchiveParent | Out-Null
$PreflightReport = "$ArchiveDirectory.preflight.json"
$AttemptReport = "$ArchiveDirectory.attempt.json"

Write-Host "[GTT] Running Win64/UE 5.8 build preflight before touching the archive directory..."
& $Preflight -EngineRoot $EngineRoot -ProjectFile $ProjectFile -OutputPath $PreflightReport
$preflightExit = $LASTEXITCODE
if ($preflightExit -ne 0) { throw "Win64 Unreal preflight failed with exit code $preflightExit. Evidence: $PreflightReport" }
if (-not (Test-Path $RunUAT)) { throw "RunUAT.bat was not found after preflight. Expected: $RunUAT" }

if (Test-Path $ArchiveDirectory) { Remove-Item -Recurse -Force $ArchiveDirectory }
New-Item -ItemType Directory -Force -Path $ArchiveDirectory | Out-Null

$gitSha = "unknown"
try {
    $candidate = (& git -C $ProjectRoot rev-parse HEAD 2>$null).Trim()
    if ($candidate) { $gitSha = $candidate }
} catch { }

$attempt = [ordered]@{
    evidence_schema = 2
    gate = "GTT_WIN64_BUILD_ATTEMPT"
    result = "RUNNING"
    game = "Grand Theft Tractor"
    version = $Version
    configuration = $Configuration
    platform = "Win64"
    engine_root = $EngineRoot
    git_sha = $gitSha
    started_utc = (Get-Date).ToUniversalTime().ToString("o")
    completed_utc = $null
    uat_exit_code = $null
    error = $null
}
$attempt | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $AttemptReport

Write-Host "[GTT] Packaging Windows build..."
Write-Host "[GTT] Engine: $EngineRoot"
Write-Host "[GTT] Config: $Configuration"
Write-Host "[GTT] Version: $Version"
Write-Host "[GTT] Output: $ArchiveDirectory"

$uatExit = -1
try {
    & $RunUAT BuildCookRun `
        -project="$ProjectFile" `
        -noP4 `
        -platform=Win64 `
        -target=GTT `
        -clientconfig=$Configuration `
        -build `
        -cook `
        -stage `
        -pak `
        -iostore `
        -prereqs `
        -archive `
        -archivedirectory="$ArchiveDirectory" `
        -utf8output

    $uatExit = $LASTEXITCODE
    if ($uatExit -ne 0) { throw "Unreal Automation Tool failed with exit code $uatExit" }
    $attempt.result = "PASS"
} catch {
    $attempt.result = "FAIL"
    $attempt.error = $_.Exception.Message
    if ($uatExit -ge 0) { $attempt.uat_exit_code = $uatExit }
    $attempt.completed_utc = (Get-Date).ToUniversalTime().ToString("o")
    $attempt | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $AttemptReport
    throw
}

$attempt.uat_exit_code = $uatExit
$attempt.completed_utc = (Get-Date).ToUniversalTime().ToString("o")
$attempt | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $AttemptReport

Copy-Item -Force $PreflightReport (Join-Path $ArchiveDirectory "WIN64_PREFLIGHT.json")
Copy-Item -Force $AttemptReport (Join-Path $ArchiveDirectory "BUILD_ATTEMPT.json")

$buildInfo = [ordered]@{
    game = "Grand Theft Tractor"
    version = $Version
    configuration = $Configuration
    platform = "Win64"
    engine = Split-Path -Leaf $EngineRoot
    git_sha = $gitSha
    built_utc = (Get-Date).ToUniversalTime().ToString("o")
    evidence_schema = 2
    preflight = "WIN64_PREFLIGHT.json"
    build_attempt = "BUILD_ATTEMPT.json"
}
$buildInfoPath = Join-Path $ArchiveDirectory "BUILD_INFO.json"
$buildInfo | ConvertTo-Json | Set-Content -Encoding UTF8 $buildInfoPath

& $Validator -PackageDirectory $ArchiveDirectory -Configuration $Configuration -Version $Version
if ($LASTEXITCODE -ne 0) { throw "Package validation failed with exit code $LASTEXITCODE" }

$hashFile = Join-Path $ArchiveDirectory "SHA256SUMS.txt"
$filesToHash = Get-ChildItem -Path $ArchiveDirectory -Recurse -File | Where-Object { $_.FullName -ne $hashFile } | Sort-Object FullName
$hashLines = foreach ($file in $filesToHash) {
    $relative = [System.IO.Path]::GetRelativePath($ArchiveDirectory, $file.FullName).Replace('\','/')
    $hash = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash.ToLowerInvariant()
    "$hash  $relative"
}
$hashLines | Set-Content -Encoding ASCII $hashFile

if (-not $SkipZip) {
    $zipPath = "$ArchiveDirectory.zip"
    if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
    Compress-Archive -Path (Join-Path $ArchiveDirectory '*') -DestinationPath $zipPath -CompressionLevel Optimal
    $zipHash = (Get-FileHash -Algorithm SHA256 -Path $zipPath).Hash.ToLowerInvariant()
    Set-Content -Encoding ASCII -Path "$zipPath.sha256" -Value "$zipHash  $([System.IO.Path]::GetFileName($zipPath))"
    Write-Host "[GTT] Release ZIP: $zipPath"
}

Write-Host "[GTT] Windows package finished and validated successfully."
Write-Host "[GTT] Build: $ArchiveDirectory"
Write-Host "[GTT] Manifest: $hashFile"
Write-Host "[GTT] Preflight evidence: $(Join-Path $ArchiveDirectory 'WIN64_PREFLIGHT.json')"
Write-Host "[GTT] Build attempt evidence: $(Join-Path $ArchiveDirectory 'BUILD_ATTEMPT.json')"
