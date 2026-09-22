param(
    [Parameter(Mandatory=$true)][string]$PacketPath,
    [Parameter(Mandatory=$true)][string]$CandidateArchivePath,
    [Parameter(Mandatory=$true)][string]$Reviewer,
    [ValidateSet("PASS","FAIL")][Parameter(Mandatory=$true)][string]$CoherentGameplayPresentation,
    [ValidateSet("PASS","FAIL")][Parameter(Mandatory=$true)][string]$ReadableHudUi,
    [ValidateSet("PASS","FAIL")][Parameter(Mandatory=$true)][string]$NoPlaceholderDebugClutter,
    [ValidateSet("PASS","FAIL")][Parameter(Mandatory=$true)][string]$NativeVehicleVisualIntegrity,
    [ValidateSet("PASS","FAIL")][Parameter(Mandatory=$true)][string]$AuthoredTrailerWheelsHitchVisualIntegrity,
    [string]$Notes = "",
    [string]$OutputPath = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$PacketPath = [IO.Path]::GetFullPath($PacketPath)
$CandidateArchivePath = [IO.Path]::GetFullPath($CandidateArchivePath)
if (-not (Test-Path $PacketPath -PathType Leaf)) { throw "Human review packet missing: $PacketPath" }
if (-not (Test-Path $CandidateArchivePath -PathType Leaf)) { throw "Candidate archive missing: $CandidateArchivePath" }
if ([string]::IsNullOrWhiteSpace($Reviewer)) { throw "Reviewer must not be empty." }

try { $packet = Get-Content -Raw $PacketPath | ConvertFrom-Json }
catch { throw "Human review packet is invalid JSON: $($_.Exception.Message)" }
if ([string]$packet.schema -ne "gtt.demo-human-review-packet.v1") { throw "Human review packet schema mismatch." }
if ([string]$packet.status -ne "READY_FOR_HUMAN_REVIEW") { throw "Human review packet is not ready for review." }
if ([string]$packet.git_sha -notmatch '^[0-9a-f]{40}$') { throw "Human review packet git_sha is invalid." }
if ([string]$packet.archive_sha256 -notmatch '^[0-9a-f]{64}$') { throw "Human review packet archive hash is invalid." }
if ([bool]$packet.demo_release_authorized) { throw "Human review packet must never pre-authorize a Demo Release." }
if ([string]$packet.canonical_archive_verification -ne "PASS" -or [string]$packet.technical_gate -ne "PASS" -or [string]$packet.rendered_visual_evidence -ne "PASS") {
    throw "Human review packet does not carry complete automated prerequisite evidence."
}

$archiveHash = (Get-FileHash -Algorithm SHA256 -Path $CandidateArchivePath).Hash.ToLowerInvariant()
if ($archiveHash -ne [string]$packet.archive_sha256) { throw "Candidate archive hash does not match the immutable human review packet." }
if ([IO.Path]::GetFileName($CandidateArchivePath) -ne [string]$packet.archive_file) { throw "Candidate archive filename does not match the human review packet." }

$expectedScenes = @('world_gameplay','law_pressure','native_vehicle','loaded_trailer','hud_overview')
$packetScenes = @($packet.screenshots)
if ($packetScenes.Count -ne $expectedScenes.Count) { throw "Human review packet must contain exactly five screenshot records." }
foreach ($scene in $expectedScenes) {
    $record = @($packetScenes | Where-Object { [string]$_.scene -eq $scene })
    if ($record.Count -ne 1) { throw "Human review packet does not contain exactly one '$scene' screenshot record." }
    if ([string]$record[0].sha256 -notmatch '^[0-9a-f]{64}$') { throw "Human review packet screenshot hash is invalid for '$scene'." }
    if ([string]$record[0].human_decision -ne "PENDING") { throw "Human review packet screenshot '$scene' was modified before review." }
}

$expectedCriteria = @(
    'coherent_gameplay_presentation',
    'readable_hud_ui',
    'no_placeholder_debug_clutter',
    'native_vehicle_visual_integrity',
    'authored_trailer_wheels_hitch_visual_integrity'
)
$packetCriteria = @($packet.criteria)
if ($packetCriteria.Count -ne $expectedCriteria.Count) { throw "Human review packet criteria count mismatch." }
foreach ($criterion in $expectedCriteria) {
    $entry = @($packetCriteria | Where-Object { [string]$_.id -eq $criterion })
    if ($entry.Count -ne 1 -or [string]$entry[0].status -ne 'PENDING') { throw "Human review packet criterion '$criterion' is missing or was pre-decided." }
}

$decisions = [ordered]@{
    coherent_gameplay_presentation = $CoherentGameplayPresentation
    readable_hud_ui = $ReadableHudUi
    no_placeholder_debug_clutter = $NoPlaceholderDebugClutter
    native_vehicle_visual_integrity = $NativeVehicleVisualIntegrity
    authored_trailer_wheels_hitch_visual_integrity = $AuthoredTrailerWheelsHitchVisualIntegrity
}
$allPass = $true
foreach ($value in $decisions.Values) { if ([string]$value -ne 'PASS') { $allPass = $false } }
$result = if ($allPass) { 'PASS' } else { 'FAIL' }

if ([string]::IsNullOrWhiteSpace($OutputPath)) { $OutputPath = "$CandidateArchivePath.human-review.json" }
$OutputPath = [IO.Path]::GetFullPath($OutputPath)
$packetHash = (Get-FileHash -Algorithm SHA256 -Path $PacketPath).Hash.ToLowerInvariant()
$review = [ordered]@{
    schema = "gtt.demo-human-visual-review.v1"
    result = $result
    game = "Grand Theft Tractor"
    git_sha = [string]$packet.git_sha
    version = [string]$packet.version
    configuration = [string]$packet.configuration
    platform = [string]$packet.platform
    engine = [string]$packet.engine
    archive_file = [IO.Path]::GetFileName($CandidateArchivePath)
    archive_sha256 = $archiveHash
    review_packet_sha256 = $packetHash
    reviewer = $Reviewer.Trim()
    criteria = $decisions
    notes = $Notes
    human_visual_acceptance = $result
    demo_release_authorized = $false
    reviewed_utc = (Get-Date).ToUniversalTime().ToString("o")
}
$review | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $OutputPath

if ($result -ne 'PASS') {
    Write-Host "[GTT] Human visual review: FAIL. Demo Release remains blocked."
    Write-Host "[GTT] Review evidence: $OutputPath"
    exit 1
}
Write-Host "[GTT] Human visual review: PASS for exact sealed candidate $($packet.git_sha)."
Write-Host "[GTT] Review evidence: $OutputPath"
Write-Host "[GTT] PASS does NOT itself authorize Demo Release; all canonical roadmap/release gates must still be closed."
