param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [ValidateSet("Development", "Shipping")]
    [string]$Configuration = "Development",
    [string]$ArchiveDirectory = ""
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot "GTT.uproject"
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"

if (-not (Test-Path $ProjectFile)) {
    throw "GTT.uproject was not found at $ProjectFile"
}

if (-not (Test-Path $RunUAT)) {
    throw "RunUAT.bat was not found. Install Unreal Engine 5.8 or pass -EngineRoot. Expected: $RunUAT"
}

if ([string]::IsNullOrWhiteSpace($ArchiveDirectory)) {
    $ArchiveDirectory = Join-Path $ProjectRoot "Releases\Windows-$Configuration"
}

New-Item -ItemType Directory -Force -Path $ArchiveDirectory | Out-Null

Write-Host "[GTT] Packaging Windows build..."
Write-Host "[GTT] Engine: $EngineRoot"
Write-Host "[GTT] Config: $Configuration"
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
    -archive `
    -archivedirectory="$ArchiveDirectory" `
    -utf8output

if ($LASTEXITCODE -ne 0) {
    throw "Unreal Automation Tool failed with exit code $LASTEXITCODE"
}

Write-Host "[GTT] Windows package finished successfully."
Write-Host "[GTT] Build is in: $ArchiveDirectory"
