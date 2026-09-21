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
if ($ExpectedGitSha -notmatch '^[0-9a-fA-F]{40}$') { throw "ExpectedGitSha must be an exact 40-character Git SHA." }
$ExpectedGitSha = $ExpectedGitSha.ToLowerInvariant()
$ZipPath = "$PackageDirectory.zip"
$ZipHashPath = "$ZipPath.sha256"
$VerificationPath = "$ZipPath.verify.json"

foreach ($path in @($ZipPath, $ZipHashPath)) {
    if (-not (Test-Path $path -PathType Leaf)) { throw "Sealed candidate artifact missing: $path" }
}

$sidecar = (Get-Content -Raw $ZipHashPath).Trim()
$sidecarMatch = [regex]::Match($sidecar, '^(?<hash>[0-9a-f]{64})  (?<name>[^/\\]+\.zip)$')
if (-not $sidecarMatch.Success) { throw "Candidate ZIP SHA256 sidecar has invalid canonical format." }
if ($sidecarMatch.Groups['name'].Value -ne [IO.Path]::GetFileName($ZipPath)) {
    throw "Candidate ZIP SHA256 sidecar names a different archive."
}
$actualZipHash = (Get-FileHash -Algorithm SHA256 -Path $ZipPath).Hash.ToLowerInvariant()
if ($actualZipHash -ne $sidecarMatch.Groups['hash'].Value) {
    throw "Candidate ZIP SHA256 sidecar hash does not match archive bytes."
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [IO.Compression.ZipFile]::OpenRead($ZipPath)
try {
    if ($archive.Entries.Count -lt 2) { throw "Candidate archive is unexpectedly empty." }
    foreach ($entry in $archive.Entries) {
        $name = $entry.FullName.Replace('\','/')
        if ([string]::IsNullOrWhiteSpace($name)) { throw "Candidate archive contains an empty entry name." }
        if ($name.StartsWith('/') -or $name -match '^[A-Za-z]:' -or $name -match '(^|/)\.\.?(/|$)') {
            throw "Candidate archive contains an unsafe path: $name"
        }
    }
}
finally {
    $archive.Dispose()
}

$tempRoot = Join-Path ([IO.Path]::GetTempPath()) ("gtt-candidate-verify-" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempRoot | Out-Null
try {
    Expand-Archive -LiteralPath $ZipPath -DestinationPath $tempRoot -Force

    $manifestPath = Join-Path $tempRoot "FINAL_SHA256SUMS.txt"
    $attestationPath = Join-Path $tempRoot "WIN64_CANDIDATE_ATTESTATION.json"
    foreach ($path in @($manifestPath, $attestationPath)) {
        if (-not (Test-Path $path -PathType Leaf)) { throw "Candidate archive missing sealed file: $([IO.Path]::GetFileName($path))" }
    }

    $manifest = @{}
    foreach ($line in @(Get-Content $manifestPath)) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        $match = [regex]::Match($line, '^(?<hash>[0-9a-f]{64})  (?<path>.+)$')
        if (-not $match.Success) { throw "FINAL_SHA256SUMS.txt contains a malformed line." }
        $relative = $match.Groups['path'].Value.Replace('\','/')
        if ($relative.StartsWith('/') -or $relative -match '^[A-Za-z]:' -or $relative -match '(^|/)\.\.?(/|$)') {
            throw "FINAL_SHA256SUMS.txt contains an unsafe path: $relative"
        }
        if ($manifest.ContainsKey($relative)) { throw "FINAL_SHA256SUMS.txt contains duplicate path: $relative" }
        $manifest[$relative] = $match.Groups['hash'].Value
    }
    if ($manifest.Count -lt 2) { throw "FINAL_SHA256SUMS.txt does not cover a usable candidate." }

    $actualFiles = @(
        Get-ChildItem -Path $tempRoot -Recurse -File |
            Where-Object { $_.FullName -ne $manifestPath }
    )
    if ($actualFiles.Count -ne $manifest.Count) {
        throw "Candidate archive file set does not exactly match FINAL_SHA256SUMS.txt ($($actualFiles.Count) files vs $($manifest.Count) manifest entries)."
    }

    foreach ($file in $actualFiles) {
        $relative = [IO.Path]::GetRelativePath($tempRoot, $file.FullName).Replace('\','/')
        if (-not $manifest.ContainsKey($relative)) { throw "Candidate archive contains unmanifested file: $relative" }
        $actualHash = (Get-FileHash -Algorithm SHA256 -Path $file.FullName).Hash.ToLowerInvariant()
        if ($actualHash -ne [string]$manifest[$relative]) { throw "Manifest hash mismatch for: $relative" }
    }

    try { $attestation = Get-Content -Raw $attestationPath | ConvertFrom-Json }
    catch { throw "WIN64_CANDIDATE_ATTESTATION.json is invalid JSON: $($_.Exception.Message)" }

    if ([string]$attestation.schema -ne "gtt.win64-candidate-attestation.v1" -or [string]$attestation.result -ne "PASS") {
        throw "Candidate attestation is not PASS schema v1."
    }
    if ([string]$attestation.git_sha -ne $ExpectedGitSha) { throw "Candidate attestation Git SHA does not match exact checkout." }
    if ([string]$attestation.version -ne $Version) { throw "Candidate attestation version does not match requested candidate." }
    if ([string]$attestation.configuration -ne $Configuration) { throw "Candidate attestation configuration does not match requested candidate." }
    if ([string]$attestation.platform -ne "Win64") { throw "Candidate attestation platform must be Win64." }
    if ([string]$attestation.engine -ne "Unreal Engine 5.8") { throw "Candidate attestation engine must be Unreal Engine 5.8." }
    if ([string]$attestation.evidence_hash_algorithm -ne "SHA256") { throw "Candidate attestation evidence hash algorithm must be SHA256." }

    # FINISH-FIRST current-target boundary: the consumer-side round-trip must
    # independently prove that the sealed archive still carries the concrete
    # Native Chaos tractor, drivetrain/suspension/wheel and authored-trailer
    # acceptance claims. Do not reduce this to a generic attestation PASS token.
    if ([string]$attestation.native_chaos_tractor_movement -ne "PASS" -or
        [int]$attestation.native_chaos_movement_samples -lt 2 -or
        [double]$attestation.native_chaos_max_speed_kmh -lt 0.35) {
        throw "Candidate archive does not preserve dedicated Native Chaos tractor movement acceptance."
    }
    if ([string]$attestation.native_chaos_drivetrain_suspension_wheels -ne "PASS" -or
        [int]$attestation.native_chaos_valid_wheels -lt 4 -or
        [int]$attestation.native_chaos_suspension_samples -lt 4 -or
        [int]$attestation.native_chaos_contact_samples -lt 2) {
        throw "Candidate archive does not preserve Native Chaos drivetrain/suspension/wheel acceptance."
    }
    if ([string]$attestation.native_drivetrain_scenario -ne "PASS" -or
        [int]$attestation.native_drivetrain_max_forward_gear -lt 2 -or
        [int]$attestation.native_drivetrain_diagnostic_failures -ne 0) {
        throw "Candidate archive does not preserve the accepted native drivetrain scenario."
    }
    if ([string]$attestation.authored_trailer_runtime -ne "PASS" -or
        [int]$attestation.authored_trailer_dual_contact_samples -lt 2 -or
        [int]$attestation.authored_trailer_safe_hitch_samples -lt 2 -or
        [int]$attestation.authored_trailer_safe_loaded_motion_samples -lt 8 -or
        -not [bool]$attestation.authored_trailer_controlled_stop -or
        [int]$attestation.authored_trailer_invalid_rig_observations -ne 0) {
        throw "Candidate archive does not preserve authored trailer runtime/hitch/wheel acceptance."
    }

    if ([string]$attestation.native_authority_runtime -ne "PASS" -or [int]$attestation.native_authority_faults -ne 0) {
        throw "Candidate attestation does not preserve clean Native Chaos authority."
    }
    if ([string]$attestation.fieldmaster_hill_haul_runtime -ne "PASS" -or [string]$attestation.fieldmaster_hud_runtime -ne "PASS") {
        throw "Candidate attestation does not preserve Fieldmaster hill-haul/HUD runtime acceptance."
    }
    if ([int]$attestation.technical_gate_schema -ne 17 -or [string]$attestation.technical_gate -ne "PASS") {
        throw "Candidate attestation technical gate is not PASS schema 17."
    }
    if ([string]$attestation.rendered_visual_evidence -ne "PASS" -or [int]$attestation.rendered_screenshot_count -ne 5) {
        throw "Candidate attestation does not preserve the five rendered evidence screenshots."
    }
    if ([string]$attestation.human_visual_review -ne "REQUIRED" -or [bool]$attestation.demo_release_authorized) {
        throw "Candidate archive crossed the human visual review / Demo Release boundary."
    }

    $evidenceFiles = @($attestation.evidence_files)
    if ($evidenceFiles.Count -ne [int]$attestation.evidence_file_count) {
        throw "Candidate attestation evidence_file_count does not match evidence_files."
    }
    $seenEvidence = @{}
    foreach ($entry in $evidenceFiles) {
        $relative = ([string]$entry.path).Replace('\','/')
        if ([string]::IsNullOrWhiteSpace($relative) -or $relative.StartsWith('/') -or $relative -match '^[A-Za-z]:' -or $relative -match '(^|/)\.\.?(/|$)') {
            throw "Candidate attestation contains an unsafe evidence path: $relative"
        }
        if ($seenEvidence.ContainsKey($relative)) { throw "Candidate attestation contains duplicate evidence path: $relative" }
        $seenEvidence[$relative] = $true
        if (-not $manifest.ContainsKey($relative)) { throw "Attested evidence is absent from final manifest: $relative" }

        $filePath = Join-Path $tempRoot ($relative.Replace('/', [IO.Path]::DirectorySeparatorChar))
        if (-not (Test-Path $filePath -PathType Leaf)) { throw "Attested evidence is absent from archive: $relative" }
        $file = Get-Item $filePath
        $actualHash = (Get-FileHash -Algorithm SHA256 -Path $filePath).Hash.ToLowerInvariant()
        if ($actualHash -ne [string]$entry.sha256 -or $actualHash -ne [string]$manifest[$relative]) {
            throw "Attested evidence hash mismatch: $relative"
        }
        if ([int64]$file.Length -ne [int64]$entry.bytes) { throw "Attested evidence byte count mismatch: $relative" }
    }

    $exeCandidates = @(Get-ChildItem -Path $tempRoot -Recurse -File -Filter "GTT.exe")
    if ($exeCandidates.Count -ne 1) { throw "Expected exactly one packaged GTT.exe in archive, found $($exeCandidates.Count)." }
    $exe = $exeCandidates[0]
    $exeRelative = [IO.Path]::GetRelativePath($tempRoot, $exe.FullName).Replace('\','/')
    $exeHash = (Get-FileHash -Algorithm SHA256 -Path $exe.FullName).Hash.ToLowerInvariant()
    if ($exeRelative -ne [string]$attestation.packaged_exe) { throw "Attested packaged_exe path does not match archive." }
    if ($exeHash -ne [string]$attestation.packaged_exe_sha256) { throw "Attested packaged_exe_sha256 does not match archive." }
    if (-not $manifest.ContainsKey($exeRelative) -or $exeHash -ne [string]$manifest[$exeRelative]) {
        throw "Packaged GTT.exe is not consistently sealed by FINAL_SHA256SUMS.txt."
    }

    $verification = [ordered]@{
        schema = "gtt.win64-candidate-archive-verification.v1"
        result = "PASS"
        git_sha = $ExpectedGitSha
        version = $Version
        configuration = $Configuration
        platform = "Win64"
        engine = "Unreal Engine 5.8"
        archive = [IO.Path]::GetFileName($ZipPath)
        archive_sha256 = $actualZipHash
        manifest_entry_count = $manifest.Count
        attested_evidence_count = $evidenceFiles.Count
        packaged_exe = $exeRelative
        packaged_exe_sha256 = $exeHash
        human_visual_review = "REQUIRED"
        demo_release_authorized = $false
        verified_utc = (Get-Date).ToUniversalTime().ToString("o")
    }
    $verification | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $VerificationPath
}
finally {
    if (Test-Path $tempRoot) { Remove-Item -Recurse -Force $tempRoot }
}

Write-Host "[GTT][ARCHIVE] PASS: ZIP round-trip, full manifest coverage, exact candidate identity and attested current-target gate integrity verified."
Write-Host "[GTT][ARCHIVE] Verification: $VerificationPath"
