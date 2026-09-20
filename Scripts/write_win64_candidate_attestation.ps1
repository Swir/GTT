param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$Version,
    [ValidateSet("Development", "Shipping")]
    [Parameter(Mandatory=$true)][string]$Configuration,
    [Parameter(Mandatory=$true)][string]$ExpectedGitSha
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$PackageDirectory = [IO.Path]::GetFullPath($PackageDirectory)
if (-not (Test-Path $PackageDirectory -PathType Container)) { throw "Package directory does not exist: $PackageDirectory" }
if ([string]::IsNullOrWhiteSpace($Version)) { throw "Version must not be empty." }
if ($ExpectedGitSha -notmatch '^[0-9a-fA-F]{40}$') { throw "ExpectedGitSha must be an exact 40-character Git SHA." }
$ExpectedGitSha = $ExpectedGitSha.ToLowerInvariant()

function Read-JsonRequired {
    param([Parameter(Mandatory=$true)][string]$Name)
    $path = Join-Path $PackageDirectory $Name
    if (-not (Test-Path $path -PathType Leaf)) { throw "Required candidate evidence missing: $Name" }
    try { return (Get-Content -Raw $path | ConvertFrom-Json) }
    catch { throw "Invalid JSON in required candidate evidence '$Name': $($_.Exception.Message)" }
}

function Assert-ExactIdentity {
    param([Parameter(Mandatory=$true)][object]$Object, [Parameter(Mandatory=$true)][string]$Scope)
    if (-not ($Object.PSObject.Properties.Name -contains 'git_sha') -or [string]$Object.git_sha -ne $ExpectedGitSha) {
        throw "$Scope git_sha does not match exact candidate $ExpectedGitSha."
    }
    if (($Object.PSObject.Properties.Name -contains 'version') -and [string]$Object.version -ne $Version) {
        throw "$Scope version '$($Object.version)' does not match candidate '$Version'."
    }
}

$buildInfo = Read-JsonRequired "BUILD_INFO.json"
Assert-ExactIdentity $buildInfo "BUILD_INFO.json"
if ([string]$buildInfo.configuration -ne $Configuration) { throw "BUILD_INFO.json configuration does not match '$Configuration'." }
if ([string]$buildInfo.platform -ne "Win64") { throw "BUILD_INFO.json platform must be Win64." }

$attempt = Read-JsonRequired "BUILD_ATTEMPT.json"
Assert-ExactIdentity $attempt "BUILD_ATTEMPT.json"
if ([string]$attempt.result -ne "PASS") { throw "BUILD_ATTEMPT.json is not PASS." }

$import = Read-JsonRequired "AUTHORED_TRAILER_IMPORT.json"
Assert-ExactIdentity $import "AUTHORED_TRAILER_IMPORT.json"
if ([string]$import.result -ne "PASS") { throw "AUTHORED_TRAILER_IMPORT.json is not PASS." }

$runtime = Read-JsonRequired "RUNTIME_SMOKE.json"
Assert-ExactIdentity $runtime "RUNTIME_SMOKE.json"
if ([string]$runtime.result -ne "PASS") { throw "RUNTIME_SMOKE.json is not PASS." }

$chaos = Read-JsonRequired "NATIVE_CHAOS_RUNTIME.json"
Assert-ExactIdentity $chaos "NATIVE_CHAOS_RUNTIME.json"
if ([string]$chaos.result -ne "PASS") { throw "NATIVE_CHAOS_RUNTIME.json is not PASS." }

$authority = Read-JsonRequired "NATIVE_AUTHORITY_RUNTIME.json"
Assert-ExactIdentity $authority "NATIVE_AUTHORITY_RUNTIME.json"
if ([string]$authority.result -ne "PASS") { throw "NATIVE_AUTHORITY_RUNTIME.json is not PASS." }
if ([string]$authority.authority -ne "NATIVE_CHAOS") { throw "NATIVE_AUTHORITY_RUNTIME.json authority must be NATIVE_CHAOS." }
if ([int]$authority.authority_faults -ne 0) { throw "NATIVE_AUTHORITY_RUNTIME.json contains split-authority faults." }

$trailer = Read-JsonRequired "NATIVE_TRAILER_RUNTIME.json"
Assert-ExactIdentity $trailer "NATIVE_TRAILER_RUNTIME.json"
if ([string]$trailer.result -ne "PASS") { throw "NATIVE_TRAILER_RUNTIME.json is not PASS." }

$gate = Read-JsonRequired "DEMO_TECHNICAL_GATE.json"
Assert-ExactIdentity $gate "DEMO_TECHNICAL_GATE.json"
if ([int]$gate.schema -ne 17 -or [string]$gate.result -ne "PASS") { throw "DEMO_TECHNICAL_GATE.json must be PASS schema 17." }

$visual = Read-JsonRequired "DEMO_VISUAL_EVIDENCE.json"
Assert-ExactIdentity $visual "DEMO_VISUAL_EVIDENCE.json"
if ([string]$visual.result -ne "PASS") { throw "DEMO_VISUAL_EVIDENCE.json is not PASS." }

$summary = Read-JsonRequired "WIN64_ACCEPTANCE_SUMMARY.json"
Assert-ExactIdentity $summary "WIN64_ACCEPTANCE_SUMMARY.json"
if ([string]$summary.result -ne "PASS") { throw "WIN64_ACCEPTANCE_SUMMARY.json is not PASS." }
if ([string]$summary.configuration -ne $Configuration) { throw "WIN64_ACCEPTANCE_SUMMARY.json configuration does not match '$Configuration'." }
if ([string]$summary.native_authority_runtime -ne "PASS") { throw "WIN64_ACCEPTANCE_SUMMARY.json native authority runtime is not PASS." }
if ([string]$summary.human_visual_review -ne "REQUIRED") { throw "Human visual review boundary must remain REQUIRED." }
if ([bool]$summary.demo_release_authorized) { throw "Technical attestation must never authorize a Demo Release." }

$exeCandidates = @(Get-ChildItem -Path $PackageDirectory -Recurse -File -Filter "GTT.exe")
if ($exeCandidates.Count -ne 1) { throw "Expected exactly one packaged GTT.exe, found $($exeCandidates.Count)." }
$exe = $exeCandidates[0]

$shots = @(Get-ChildItem -Path (Join-Path $PackageDirectory "DemoVisualEvidence") -File -Filter "GTT_visual_*.png" | Sort-Object Name)
if ($shots.Count -ne 5) { throw "Expected exactly five rendered screenshots, found $($shots.Count)." }

$criticalNames = @(
    "WIN64_PREFLIGHT.json",
    "BUILD_ATTEMPT.json",
    "BUILD_INFO.json",
    "PACKAGE_VALIDATION.json",
    "AUTHORED_TRAILER_IMPORT.json",
    "RUNTIME_SMOKE.json",
    "GAMEPLAY_SMOKE.json",
    "NATIVE_CHAOS_RUNTIME.json",
    "NATIVE_AUTHORITY_RUNTIME.json",
    "NATIVE_DRIVETRAIN_SCENARIO.json",
    "NATIVE_TRAILER_RUNTIME.json",
    "FARM_CARGO_RUNTIME.json",
    "FARM_CARGO_RECOVERY_RUNTIME.json",
    "FARM_CARGO_BREAKDOWN_RUNTIME.json",
    "FARM_CARGO_DISPATCH_RUNTIME.json",
    "FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME.json",
    "FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json",
    "WORKSHOP_HOURS_RUNTIME.json",
    "WORKSHOP_QUEUE_RUNTIME.json",
    "WORKSHOP_CAPACITY_RUNTIME.json",
    "WORKSHOP_PRIORITY_PICKUP_RUNTIME.json",
    "DEMO_TECHNICAL_GATE.json",
    "VISUAL_RUNTIME_SMOKE.json",
    "DEMO_VISUAL_EVIDENCE.json",
    "WIN64_ACCEPTANCE_SUMMARY.json",
    "GTT_RUNTIME.log",
    "GTT_VISUAL_RUNTIME.log"
)

$criticalFiles = @()
foreach ($name in $criticalNames) {
    $path = Join-Path $PackageDirectory $name
    if (-not (Test-Path $path -PathType Leaf)) { throw "Critical candidate file missing: $name" }
    $criticalFiles += Get-Item $path
}
$criticalFiles += $exe
$criticalFiles += $shots

$hashed = foreach ($file in ($criticalFiles | Sort-Object FullName -Unique)) {
    $relative = [IO.Path]::GetRelativePath($PackageDirectory, $file.FullName).Replace('\','/')
    [ordered]@{
        path = $relative
        bytes = [int64]$file.Length
        sha256 = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash.ToLowerInvariant()
    }
}

$attestation = [ordered]@{
    schema = "gtt.win64-candidate-attestation.v1"
    result = "PASS"
    game = "Grand Theft Tractor"
    git_sha = $ExpectedGitSha
    version = $Version
    configuration = $Configuration
    platform = "Win64"
    engine = "Unreal Engine 5.8"
    native_authority_runtime = "PASS"
    native_authority_faults = 0
    technical_gate_schema = 17
    technical_gate = "PASS"
    rendered_visual_evidence = "PASS"
    rendered_screenshot_count = $shots.Count
    packaged_exe = [IO.Path]::GetRelativePath($PackageDirectory, $exe.FullName).Replace('\','/')
    packaged_exe_sha256 = (Get-FileHash -Algorithm SHA256 -Path $exe.FullName).Hash.ToLowerInvariant()
    evidence_hash_algorithm = "SHA256"
    evidence_file_count = @($hashed).Count
    evidence_files = @($hashed)
    human_visual_review = "REQUIRED"
    demo_release_authorized = $false
    generated_utc = (Get-Date).ToUniversalTime().ToString("o")
}

$attestationPath = Join-Path $PackageDirectory "WIN64_CANDIDATE_ATTESTATION.json"
$attestation | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $attestationPath

# Final integrity manifest intentionally runs after all runtime/rendered evidence and attestation exist.
# It excludes only itself so a verifier can reproduce every listed digest without self-reference.
$finalManifestPath = Join-Path $PackageDirectory "FINAL_SHA256SUMS.txt"
$finalFiles = Get-ChildItem -Path $PackageDirectory -Recurse -File |
    Where-Object { $_.FullName -ne $finalManifestPath } |
    Sort-Object FullName
$manifestLines = foreach ($file in $finalFiles) {
    $relative = [IO.Path]::GetRelativePath($PackageDirectory, $file.FullName).Replace('\','/')
    $hash = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash.ToLowerInvariant()
    "$hash  $relative"
}
$manifestLines | Set-Content -Encoding ASCII $finalManifestPath

# Rebuild the archive only after attestation + final hashes exist, so the uploaded ZIP is the sealed candidate.
$zipPath = "$PackageDirectory.zip"
if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
Compress-Archive -Path (Join-Path $PackageDirectory "*") -DestinationPath $zipPath -CompressionLevel Optimal
$zipHash = (Get-FileHash -Algorithm SHA256 -Path $zipPath).Hash.ToLowerInvariant()
Set-Content -Encoding ASCII -Path "$zipPath.sha256" -Value "$zipHash  $([IO.Path]::GetFileName($zipPath))"

$roundTrip = Get-Content -Raw $attestationPath | ConvertFrom-Json
if ($roundTrip.schema -ne "gtt.win64-candidate-attestation.v1" -or $roundTrip.result -ne "PASS" -or
    $roundTrip.git_sha -ne $ExpectedGitSha -or $roundTrip.version -ne $Version -or
    $roundTrip.configuration -ne $Configuration -or $roundTrip.native_authority_runtime -ne "PASS" -or
    [int]$roundTrip.native_authority_faults -ne 0 -or $roundTrip.human_visual_review -ne "REQUIRED" -or
    [bool]$roundTrip.demo_release_authorized) {
    throw "WIN64_CANDIDATE_ATTESTATION.json failed round-trip identity/boundary validation."
}

Write-Host "[GTT][ATTEST] PASS: exact candidate identity and final evidence hashes are sealed."
Write-Host "[GTT][ATTEST] Attestation: $attestationPath"
Write-Host "[GTT][ATTEST] Final manifest: $finalManifestPath"
