param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [string]$ExpectedGitSha=$env:GITHUB_SHA
)

$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$gatePath=Join-Path $PackageDirectory 'DEMO_TECHNICAL_GATE.json'
$capacityPath=Join-Path $PackageDirectory 'WORKSHOP_CAPACITY_RUNTIME.json'
if(-not(Test-Path $gatePath -PathType Leaf)){throw "Demo technical gate missing: $gatePath"}
if(-not(Test-Path $capacityPath -PathType Leaf)){throw "Workshop-capacity runtime evidence missing: $capacityPath"}

$gate=Get-Content -Raw $gatePath|ConvertFrom-Json
$capacity=Get-Content -Raw $capacityPath|ConvertFrom-Json
if($gate.result -ne 'PASS'){throw 'Existing demo technical gate is not PASS.'}
if([int]$gate.schema -ne 15){throw "Workshop-capacity promotion expects technical gate schema 15, found $($gate.schema)."}
if($gate.workshop_queue_runtime -ne 'PASS'){throw 'Schema-15 workshop-queue prerequisite is not PASS.'}
if($capacity.result -ne 'PASS'){throw 'Workshop-capacity runtime did not PASS.'}
if($capacity.schema -ne 'gtt.workshop-capacity-runtime.v1'){throw "Workshop-capacity schema mismatch: $($capacity.schema)"}
if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown'){
    if($gate.git_sha -ne $ExpectedGitSha){throw "Technical gate SHA mismatch: gate=$($gate.git_sha), expected=$ExpectedGitSha"}
    if($capacity.git_sha -ne $ExpectedGitSha){throw "Workshop-capacity SHA mismatch: evidence=$($capacity.git_sha), expected=$ExpectedGitSha"}
}
foreach($field in @('two_exact_bookings','no_precharge','additive_disk_roundtrip','independent_exact_id_cancel','rebook_preserves_locked_quote','deterministic_45_minute_spacing','underfunded_earlier_nonblocking','earlier_reservation_preserved','later_exact_vehicle_service','later_single_debit','identity_preserved','farm_cargo_authority_preserved')){
    if(-not $capacity.$field){throw "Workshop-capacity runtime missing PASS gate: $field"}
}
if([int]$capacity.capacity -ne 4 -or [int]$capacity.appointment_spacing_minutes -ne 45){throw 'Workshop-capacity contract mismatch.'}
if([int]$capacity.first_locked_quote -le [int]$capacity.second_locked_quote -or [int]$capacity.second_locked_quote -le 0){throw 'Workshop-capacity quote ordering is invalid.'}
if([string]::IsNullOrWhiteSpace([string]$capacity.first_vehicle_id) -or [string]::IsNullOrWhiteSpace([string]$capacity.second_vehicle_id) -or $capacity.first_vehicle_id -eq $capacity.second_vehicle_id){throw 'Workshop-capacity vehicle identities are invalid.'}
if([int]$capacity.production_complete_markers -ne 1 -or [int]$capacity.diagnostic_failure_count -ne 0){throw 'Workshop-capacity production/diagnostic marker counts are invalid.'}

$gate.schema=16
$fields=[ordered]@{
    workshop_capacity_runtime='PASS'
    workshop_capacity_schema=$capacity.schema
    workshop_capacity_limit=$capacity.capacity
    workshop_capacity_spacing_minutes=$capacity.appointment_spacing_minutes
    workshop_capacity_first_vehicle_id=$capacity.first_vehicle_id
    workshop_capacity_second_vehicle_id=$capacity.second_vehicle_id
    workshop_capacity_no_precharge=$capacity.no_precharge
    workshop_capacity_disk_roundtrip=$capacity.additive_disk_roundtrip
    workshop_capacity_independent_cancel=$capacity.independent_exact_id_cancel
    workshop_capacity_underfunded_nonblocking=$capacity.underfunded_earlier_nonblocking
    workshop_capacity_single_debit=$capacity.later_single_debit
    workshop_capacity_farm_cargo_authority_preserved=$capacity.farm_cargo_authority_preserved
    workshop_capacity_gate_promoted_utc=(Get-Date).ToUniversalTime().ToString('o')
}
foreach($entry in $fields.GetEnumerator()){$gate|Add-Member -NotePropertyName $entry.Key -NotePropertyValue $entry.Value -Force}
$gate|ConvertTo-Json -Depth 10|Set-Content -Encoding UTF8 $gatePath
Write-Host '[GTT] Demo technical evidence gate promoted to schema 16: multi-vehicle workshop capacity runtime PASS'
Write-Host "[GTT] Evidence: $gatePath"
