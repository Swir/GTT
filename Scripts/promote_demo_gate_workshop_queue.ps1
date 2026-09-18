param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [string]$ExpectedGitSha=$env:GITHUB_SHA
)

$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$gatePath=Join-Path $PackageDirectory 'DEMO_TECHNICAL_GATE.json'
$queuePath=Join-Path $PackageDirectory 'WORKSHOP_QUEUE_RUNTIME.json'
if(-not(Test-Path $gatePath -PathType Leaf)){throw "Demo technical gate missing: $gatePath"}
if(-not(Test-Path $queuePath -PathType Leaf)){throw "Workshop-queue runtime evidence missing: $queuePath"}

$gate=Get-Content -Raw $gatePath|ConvertFrom-Json
$queue=Get-Content -Raw $queuePath|ConvertFrom-Json
if($gate.result -ne 'PASS'){throw 'Existing demo technical gate is not PASS.'}
if([int]$gate.schema -ne 14){throw "Workshop-queue promotion expects technical gate schema 14, found $($gate.schema)."}
if($gate.workshop_hours_runtime -ne 'PASS'){throw 'Schema-14 workshop-hours prerequisite is not PASS.'}
if($queue.result -ne 'PASS'){throw 'Workshop-queue runtime did not PASS.'}
if($queue.schema -ne 'gtt.workshop-queue-runtime.v1'){throw "Workshop-queue schema mismatch: $($queue.schema)"}
if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown'){
    if($gate.git_sha -ne $ExpectedGitSha){throw "Technical gate SHA mismatch: gate=$($gate.git_sha), expected=$ExpectedGitSha"}
    if($queue.git_sha -ne $ExpectedGitSha){throw "Workshop-queue SHA mismatch: evidence=$($queue.git_sha), expected=$ExpectedGitSha"}
}
foreach($field in @('no_precharge','checkpoint_disk_roundtrip','exact_id_persisted','locked_quote_persisted','substitute_vehicle_rejected','reservation_preserved_until_exact_vehicle','exact_vehicle_service','single_debit','sidecar_cleared','identity_preserved','mechanical_repaired','refuelled','farm_cargo_authority_preserved')){
    if(-not $queue.$field){throw "Workshop-queue runtime missing PASS gate: $field"}
}
if([int]$queue.locked_quote -le 0){throw 'Workshop-queue runtime locked quote is invalid.'}
if([int]$queue.production_accept_markers -lt 1 -or [int]$queue.production_complete_markers -ne 1){throw 'Workshop-queue runtime production marker counts are invalid.'}
if([int]$queue.diagnostic_failure_count -ne 0){throw 'Workshop-queue runtime reported diagnostic failures.'}
if([string]::IsNullOrWhiteSpace([string]$queue.stable_vehicle_id)){throw 'Workshop-queue stable vehicle id is empty.'}

$gate.schema=15
$fields=[ordered]@{
    workshop_queue_runtime='PASS'
    workshop_queue_schema=$queue.schema
    workshop_queue_vehicle_id=$queue.stable_vehicle_id
    workshop_queue_locked_quote=$queue.locked_quote
    workshop_queue_no_precharge=$queue.no_precharge
    workshop_queue_checkpoint_roundtrip=$queue.checkpoint_disk_roundtrip
    workshop_queue_substitute_rejected=$queue.substitute_vehicle_rejected
    workshop_queue_single_debit=$queue.single_debit
    workshop_queue_sidecar_cleared=$queue.sidecar_cleared
    workshop_queue_farm_cargo_authority_preserved=$queue.farm_cargo_authority_preserved
    workshop_queue_gate_promoted_utc=(Get-Date).ToUniversalTime().ToString('o')
}
foreach($entry in $fields.GetEnumerator()){$gate|Add-Member -NotePropertyName $entry.Key -NotePropertyValue $entry.Value -Force}
$gate|ConvertTo-Json -Depth 10|Set-Content -Encoding UTF8 $gatePath
Write-Host '[GTT] Demo technical evidence gate promoted to schema 15: workshop repair queue runtime PASS'
Write-Host "[GTT] Evidence: $gatePath"
