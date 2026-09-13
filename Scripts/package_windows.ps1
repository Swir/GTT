param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [ValidateSet("Development", "Shipping")]
    [string]$Configuration = "Shipping",
    [string]$ArchiveDirectory = "",
    [string]$Version = "0.0.43",
    [switch]$SkipZip
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot "GTT.uproject"
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
$Validator = Join-Path $PSScriptRoot "validate_windows_package.ps1"

if (-not (Test-Path $ProjectFile)) {
    throw "GTT.uproject was not found at $ProjectFile"
}
if (-not (Test-Path $RunUAT)) {
    throw "RunUAT.bat was not found. Install Unreal Engine 5.8 or pass -EngineRoot. Expected: $RunUAT"
}
if (-not (Test-Path $Validator)) {
    throw "Package validator was not found at $Validator"
}
if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
    $ArchiveDirectory = Join-Path $ProjectRoot "Releases\GTT-$Version-Windows-$Configuration"
}

$ArchiveDirectory = [System.IO.Path]::GetFullPath($ArchiveDirectory)
$ArchiveParent = Split-Path -Parent $ArchiveDirectory
New-Item -ItemType Directory -Force -Path $ArchiveParent | Out-Null
if (Test-Path $ArchiveDirectory) {
    Remove-Item -Recurse -Force $ArchiveDirectory
}
New-Item -ItemType Directory -Force -Path $ArchiveDirectory | Out-Null

Write-Host "[GTT] Packaging Windows build..."
Write-Host "[GTT] Engine: $EngineRoot"
Write-Host "[GTT] Config: $Configuration"
Write-Host "[GTT] Version: $Version"
Write-Host "[GTT] Output: $ArchiveDirectory"

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

if ($LASTEXITCODE -ne 0) {
    throw "Unreal Automation Tool failed with exit code $LASTEXITCODE"
}

$gitSha = "unknown"
try {
    $candidate = (& git -C $ProjectRoot rev-parse HEAD 2>$null).Trim()
    if ($candidate) { $gitSha = $candidate }
} catch { }

$buildInfo = [ordered]@{
    game = "Grand Theft Tractor"
    version = $Version
    configuration = $Configuration
    platform = "Win64"
    engine = Split-Path -Leaf $EngineRoot
    git_sha = $gitSha
    built_utc = (Get-Date).ToUniversalTime().ToString("o")
    evidence_schema = 1
}
$buildInfoPath = Join-Path $ArchiveDirectory "BUILD_INFO.json"
$buildInfo | ConvertTo-Json | Set-Content -Encoding UTF8 $buildInfoPath

& $Validator -PackageDirectory $ArchiveDirectory -Configuration $Configuration -Version $Version
if ($LASTEXITCODE -ne 0) {
    throw "Package validation failed with exit code $LASTEXITCODE"
}

$hashFile = Join-Path $ArchiveDirectory "SHA256SUMS.txt"
$filesToHash = Get-ChildItem -Path $ArchiveDirectory -Recurse -File |
    Where-Object { $_.FullName -ne $hashFile } |
    Sort-Object FullName
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
