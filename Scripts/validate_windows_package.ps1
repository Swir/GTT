param(
    [Parameter(Mandatory=$true)]
    [string]$PackageDirectory,
    [ValidateSet("Development", "Shipping")]
    [string]$Configuration = "Shipping",
    [string]$Version = "unknown"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$PackageDirectory = [System.IO.Path]::GetFullPath($PackageDirectory)
if (-not (Test-Path $PackageDirectory -PathType Container)) {
    throw "Package directory does not exist: $PackageDirectory"
}

$exeCandidates = Get-ChildItem -Path $PackageDirectory -Recurse -File -Filter "GTT.exe"
if ($exeCandidates.Count -ne 1) {
    throw "Expected exactly one GTT.exe, found $($exeCandidates.Count)."
}
$exe = $exeCandidates[0]
if ($exe.Length -le 0) {
    throw "GTT.exe is empty."
}

$pakFiles = @(Get-ChildItem -Path $PackageDirectory -Recurse -File | Where-Object { $_.Extension -in '.pak', '.utoc', '.ucas' })
if ($pakFiles.Count -eq 0) {
    throw "No cooked package container (.pak/.utoc/.ucas) was found."
}
if (($pakFiles | Measure-Object -Property Length -Sum).Sum -le 0) {
    throw "Cooked package containers are empty."
}

$requiredRuntimeFolders = @('Engine', 'GTT')
foreach ($folderName in $requiredRuntimeFolders) {
    $found = Get-ChildItem -Path $PackageDirectory -Recurse -Directory | Where-Object { $_.Name -eq $folderName } | Select-Object -First 1
    if (-not $found) {
        throw "Required packaged runtime folder '$folderName' was not found."
    }
}

if ($Configuration -eq 'Shipping') {
    $debugArtifacts = @(Get-ChildItem -Path $PackageDirectory -Recurse -File | Where-Object { $_.Extension -in '.pdb', '.exp', '.lib' })
    if ($debugArtifacts.Count -gt 0) {
        throw "Shipping package unexpectedly contains debug/linker artifacts: $($debugArtifacts.Name -join ', ')"
    }
}

$totalBytes = (Get-ChildItem -Path $PackageDirectory -Recurse -File | Measure-Object -Property Length -Sum).Sum
if (-not $totalBytes) { $totalBytes = 0 }
if ($totalBytes -lt 10MB) {
    throw "Packaged build is suspiciously small ($totalBytes bytes)."
}

$summary = [ordered]@{
    game = 'Grand Theft Tractor'
    version = $Version
    configuration = $Configuration
    executable = [System.IO.Path]::GetRelativePath($PackageDirectory, $exe.FullName).Replace('\','/')
    executable_bytes = $exe.Length
    container_files = $pakFiles.Count
    package_bytes = [int64]$totalBytes
    package_mib = [Math]::Round($totalBytes / 1MB, 2)
    validated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$summaryPath = Join-Path $PackageDirectory 'PACKAGE_VALIDATION.json'
$summary | ConvertTo-Json | Set-Content -Encoding UTF8 $summaryPath

Write-Host "[GTT] Package validation passed."
Write-Host "[GTT] EXE: $($summary.executable)"
Write-Host "[GTT] Cook containers: $($summary.container_files)"
Write-Host "[GTT] Package size: $($summary.package_mib) MiB"
