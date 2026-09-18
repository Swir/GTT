param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [string]$ExpectedGitSha=$env:GITHUB_SHA
)

$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$gatePath=Join-Path $PackageDirectory 'DEMO_TECHNICAL_GATE.json'
$hoursPath=Join-Path $PackageDirectory 'WORKSHOP_HOURS_RUNTIME.json'
if(-not(Test-Path $gatePath -PathType Leaf)){throw "Demo technical gate missing: $gatePath"}
if(-not(Test-Path $hoursPath -PathType Leaf)){throw "Workshop-hours runtime evidence missing: $hoursPath"}

$gate=Get-Content -Raw $gatePath|ConvertFrom-Json
$hours=Get-Content -Raw $hoursPath|ConvertFrom-Json
if($gate.result -ne 'PASS'){throw 'Existing demo technical gate is not PASS.'}
if([int]$gate.schema -ne 13){throw "Workshop-hours promotion expects technical gate schema 13, found $($gate.schema)."}
if($gate.farm_cargo_workshop_recovery_runtime -ne 'PASS'){throw 'Schema-13 Farm Cargo workshop recovery prerequisite is not PASS.'}
if($hours.result -ne 'PASS'){throw 'Workshop-hours runtime did not PASS.'}
if($hours.schema -ne 'gtt.workshop-hours-runtime.v1'){throw "Workshop-hours schema mismatch: $($hours.schema)"}
if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown'){
    if($gate.git_sha -ne $ExpectedGitSha){throw "Technical gate SHA mismatch: gate=$($gate.git_sha), expected=$ExpectedGitSha"}
    if($hours.git_sha -ne $ExpectedGitSha){throw "Workshop-hours SHA mismatch: evidence=$($hours.git_sha), expected=$ExpectedGitSha"}
}

foreach($field in @(
    'boundary_contract','ordinary_after_hours_rejected','ordinary_after_hours_no_charge','ordinary_after_hours_no_mutation',
    'tow_single_charge','workshop_hold_detected','emergency_quote_exact','emergency_single_charge',
    'emergency_service_applied','workshop_hold_cleared','exact_vehicle_preserved','mechanical_repaired','refuelled'
)){
    if(-not $hours.$field){throw "Workshop-hours runtime missing PASS gate: $field"}
}
if([double]$hours.opening_hour -ne 6.5 -or [double]$hours.closing_hour -ne 20.0){throw 'Workshop-hours evidence boundaries do not match 06:30-20:00.'}
if([int]$hours.emergency_surcharge_percent -ne 35){throw 'Workshop-hours evidence does not prove the +35% emergency surcharge.'}
if([int]$hours.tow_locked_quote -le 0 -or [int]$hours.emergency_base_quote -le 0 -or [int]$hours.emergency_checkout_quote -le [int]$hours.emergency_base_quote){throw 'Workshop-hours runtime quotes are invalid.'}
$expected=[int]$hours.emergency_base_quote + [int][math]::Ceiling([int]$hours.emergency_base_quote * 0.35)
if([int]$hours.emergency_checkout_quote -ne $expected){throw "Workshop-hours emergency quote mismatch: observed=$($hours.emergency_checkout_quote) expected=$expected"}
if([int]$hours.native_tow_complete_markers -lt 1){throw 'Workshop-hours runtime lacks production tow-complete evidence.'}
if([int]$hours.diagnostic_failure_count -ne 0){throw 'Workshop-hours runtime reported diagnostic failures.'}
if([string]::IsNullOrWhiteSpace([string]$hours.stable_vehicle_id)){throw 'Workshop-hours stable vehicle id is empty.'}

$gate.schema=14
$fields=[ordered]@{
    workshop_hours_runtime='PASS'
    workshop_hours_schema=$hours.schema
    workshop_hours_vehicle_id=$hours.stable_vehicle_id
    workshop_opening_hour=$hours.opening_hour
    workshop_closing_hour=$hours.closing_hour
    workshop_ordinary_after_hours_rejected=$hours.ordinary_after_hours_rejected
    workshop_emergency_surcharge_percent=$hours.emergency_surcharge_percent
    workshop_emergency_base_quote=$hours.emergency_base_quote
    workshop_emergency_checkout_quote=$hours.emergency_checkout_quote
    workshop_emergency_single_charge=$hours.emergency_single_charge
    workshop_hold_cleared=$hours.workshop_hold_cleared
    workshop_hours_gate_promoted_utc=(Get-Date).ToUniversalTime().ToString('o')
}
foreach($entry in $fields.GetEnumerator()){
    $gate | Add-Member -NotePropertyName $entry.Key -NotePropertyValue $entry.Value -Force
}
$gate|ConvertTo-Json -Depth 10|Set-Content -Encoding UTF8 $gatePath
Write-Host '[GTT] Demo technical evidence gate promoted to schema 14: workshop-hours runtime PASS'
Write-Host "[GTT] Evidence: $gatePath"
