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
if (-not (Test-Path $PackageDirectory -PathType Container)) { throw "Package directory does not exist: $PackageDirectory" }

$preflightPath = Join-Path $PackageDirectory "WIN64_PREFLIGHT.json"
$attemptPath = Join-Path $PackageDirectory "BUILD_ATTEMPT.json"
if (-not (Test-Path $preflightPath -PathType Leaf)) { throw "WIN64_PREFLIGHT.json is missing; package cannot be accepted without runner/toolchain evidence." }
if (-not (Test-Path $attemptPath -PathType Leaf)) { throw "BUILD_ATTEMPT.json is missing; package cannot be accepted without UAT build evidence." }

$preflight = Get-Content -Raw $preflightPath | ConvertFrom-Json
$attempt = Get-Content -Raw $attemptPath | ConvertFrom-Json
if ($preflight.result -ne "PASS") { throw "Win64 preflight evidence is not PASS." }
if ([int]$preflight.evidence_schema -lt 2) { throw "Win64 preflight evidence schema is too old." }
if ($attempt.result -ne "PASS" -or [int]$attempt.uat_exit_code -ne 0) { throw "UAT build attempt evidence is not PASS/0." }
if ([int]$attempt.evidence_schema -lt 2) { throw "Build attempt evidence schema is too old." }

$exeCandidates = @(Get-ChildItem -Path $PackageDirectory -Recurse -File -Filter "GTT.exe")
$exeCandidateCount = ($exeCandidates | Measure-Object).Count
if ($exeCandidateCount -ne 1) { throw "Expected exactly one GTT.exe, found $exeCandidateCount." }
$exe = $exeCandidates[0]
if ($exe.Length -le 0) { throw "GTT.exe is empty." }

$pakFiles = @(Get-ChildItem -Path $PackageDirectory -Recurse -File | Where-Object { $_.Extension -in '.pak', '.utoc', '.ucas' })
$pakFileCount = ($pakFiles | Measure-Object).Count
if ($pakFileCount -eq 0) { throw "No cooked package container (.pak/.utoc/.ucas) was found." }
if (($pakFiles | Measure-Object -Property Length -Sum).Sum -le 0) { throw "Cooked package containers are empty." }

$requiredRuntimeFolders = @('Engine', 'GTT')
foreach ($folderName in $requiredRuntimeFolders) {
    $found = Get-ChildItem -Path $PackageDirectory -Recurse -Directory | Where-Object { $_.Name -eq $folderName } | Select-Object -First 1
    if (-not $found) { throw "Required packaged runtime folder '$folderName' was not found." }
}

if ($Configuration -eq 'Shipping') {
    $debugArtifacts = @(Get-ChildItem -Path $PackageDirectory -Recurse -File | Where-Object { $_.Extension -in '.pdb', '.exp', '.lib' })
    $debugArtifactCount = ($debugArtifacts | Measure-Object).Count
    if ($debugArtifactCount -gt 0) { throw "Shipping package unexpectedly contains debug/linker artifacts: $($debugArtifacts.Name -join ', ')" }
}

$totalBytes = (Get-ChildItem -Path $PackageDirectory -Recurse -File | Measure-Object -Property Length -Sum).Sum
if (-not $totalBytes) { $totalBytes = 0 }
if ($totalBytes -lt 10MB) { throw "Packaged build is suspiciously small ($totalBytes bytes)." }

$summary = [ordered]@{
    evidence_schema = 2
    game = 'Grand Theft Tractor'
    version = $Version
    configuration = $Configuration
    executable = [System.IO.Path]::GetRelativePath($PackageDirectory, $exe.FullName).Replace('\','/')
    executable_bytes = $exe.Length
    container_files = $pakFileCount
    package_bytes = [int64]$totalBytes
    package_mib = [Math]::Round($totalBytes / 1MB, 2)
    win64_preflight = $preflight.result
    uat_build_attempt = $attempt.result
    uat_exit_code = [int]$attempt.uat_exit_code
    source_git_sha = [string]$attempt.git_sha
    validated_utc = (Get-Date).ToUniversalTime().ToString('o')
}
$summaryPath = Join-Path $PackageDirectory 'PACKAGE_VALIDATION.json'
$summary | ConvertTo-Json | Set-Content -Encoding UTF8 $summaryPath

Write-Host "[GTT] Package validation passed."
Write-Host "[GTT] EXE: $($summary.executable)"
Write-Host "[GTT] Cook containers: $($summary.container_files)"
Write-Host "[GTT] Package size: $($summary.package_mib) MiB"
Write-Host "[GTT] Preflight/build evidence: PASS/PASS"
