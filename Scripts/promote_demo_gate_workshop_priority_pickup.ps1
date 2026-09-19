param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [string]$ExpectedGitSha=$env:GITHUB_SHA
)

$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$gatePath=Join-Path $PackageDirectory 'DEMO_TECHNICAL_GATE.json'
$evidencePath=Join-Path $PackageDirectory 'WORKSHOP_PRIORITY_PICKUP_RUNTIME.json'
if(-not(Test-Path $gatePath -PathType Leaf)){throw "Demo technical gate missing: $gatePath"}
if(-not(Test-Path $evidencePath -PathType Leaf)){throw "Workshop priority/pickup runtime evidence missing: $evidencePath"}

$gate=Get-Content -Raw $gatePath|ConvertFrom-Json
$evidence=Get-Content -Raw $evidencePath|ConvertFrom-Json
if($gate.result -ne 'PASS'){throw 'Existing demo technical gate is not PASS.'}
if([int]$gate.schema -ne 16){throw "Workshop priority/pickup promotion expects technical gate schema 16, found $($gate.schema)."}
if($gate.workshop_capacity_runtime -ne 'PASS'){throw 'Schema-16 workshop-capacity prerequisite is not PASS.'}
if($evidence.result -ne 'PASS'){throw 'Workshop priority/pickup runtime did not PASS.'}
if($evidence.schema -ne 'gtt.workshop-priority-pickup-runtime.v1'){throw "Workshop priority/pickup schema mismatch: $($evidence.schema)"}
if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown'){
    if($gate.git_sha -ne $ExpectedGitSha){throw "Technical gate SHA mismatch: gate=$($gate.git_sha), expected=$ExpectedGitSha"}
    if($evidence.git_sha -ne $ExpectedGitSha){throw "Workshop priority/pickup SHA mismatch: evidence=$($evidence.git_sha), expected=$ExpectedGitSha"}
}
foreach($field in @('exact_id_priority_promotion','no_precharge','priority_disk_persistence','urgent_timing_verified','single_locked_quote_debit','ready_for_pickup_persisted','pickup_hold_observed','wrong_id_pickup_rejected','exact_id_pickup_release','pickup_has_no_second_charge','identity_preserved','farm_cargo_authority_preserved')){
    if(-not $evidence.$field){throw "Workshop priority/pickup runtime missing PASS gate: $field"}
}
if([int]$evidence.urgent_surcharge_percent -ne 20){throw 'Urgent surcharge contract mismatch.'}
if([Math]::Abs([double]$evidence.urgent_service_multiplier-0.80) -gt 0.0001){throw 'Urgent service multiplier contract mismatch.'}
if([int]$evidence.standard_locked_quote -le 0 -or [int]$evidence.urgent_locked_quote -le [int]$evidence.standard_locked_quote){throw 'Workshop priority/pickup quote contract invalid.'}
if([string]::IsNullOrWhiteSpace([string]$evidence.vehicle_id) -or [string]::IsNullOrWhiteSpace([string]$evidence.decoy_vehicle_id) -or $evidence.vehicle_id -eq $evidence.decoy_vehicle_id){throw 'Workshop priority/pickup vehicle identities are invalid.'}
if([int]$evidence.production_priority_markers -ne 1 -or [int]$evidence.production_checkin_markers -ne 1 -or [int]$evidence.production_ready_markers -ne 1 -or [int]$evidence.production_pickup_markers -ne 1 -or [int]$evidence.diagnostic_failure_count -ne 0){throw 'Workshop priority/pickup production/diagnostic marker counts are invalid.'}

$gate.schema=17
$fields=[ordered]@{
    workshop_priority_pickup_runtime='PASS'
    workshop_priority_pickup_schema=$evidence.schema
    workshop_priority_pickup_vehicle_id=$evidence.vehicle_id
    workshop_priority_standard_quote=$evidence.standard_locked_quote
    workshop_priority_urgent_quote=$evidence.urgent_locked_quote
    workshop_priority_surcharge_percent=$evidence.urgent_surcharge_percent
    workshop_priority_service_multiplier=$evidence.urgent_service_multiplier
    workshop_priority_no_precharge=$evidence.no_precharge
    workshop_priority_disk_persistence=$evidence.priority_disk_persistence
    workshop_priority_single_debit=$evidence.single_locked_quote_debit
    workshop_priority_ready_for_pickup=$evidence.ready_for_pickup_persisted
    workshop_priority_exact_pickup=$evidence.exact_id_pickup_release
    workshop_priority_no_second_charge=$evidence.pickup_has_no_second_charge
    workshop_priority_farm_cargo_authority_preserved=$evidence.farm_cargo_authority_preserved
    workshop_priority_pickup_gate_promoted_utc=(Get-Date).ToUniversalTime().ToString('o')
}
foreach($entry in $fields.GetEnumerator()){$gate|Add-Member -NotePropertyName $entry.Key -NotePropertyValue $entry.Value -Force}
$gate|ConvertTo-Json -Depth 10|Set-Content -Encoding UTF8 $gatePath
Write-Host '[GTT] Demo technical evidence gate promoted to schema 17: workshop priority/pickup runtime PASS'
Write-Host "[GTT] Evidence: $gatePath"
