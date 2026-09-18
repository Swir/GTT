param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [string]$ExpectedGitSha=$env:GITHUB_SHA
)

$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$gatePath=Join-Path $PackageDirectory 'DEMO_TECHNICAL_GATE.json'
$workshopPath=Join-Path $PackageDirectory 'FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json'
if(-not(Test-Path $gatePath -PathType Leaf)){throw "Demo technical gate missing: $gatePath"}
if(-not(Test-Path $workshopPath -PathType Leaf)){throw "Workshop recovery evidence missing: $workshopPath"}

$gate=Get-Content -Raw $gatePath|ConvertFrom-Json
$workshop=Get-Content -Raw $workshopPath|ConvertFrom-Json
if($gate.result -ne 'PASS'){throw 'Existing demo technical gate is not PASS.'}
if([int]$gate.schema -ne 12){throw "Workshop recovery promotion expects technical gate schema 12, found $($gate.schema)."}
if($workshop.result -ne 'PASS'){throw 'Farm Cargo workshop recovery runtime did not PASS.'}
if($workshop.schema -ne 'gtt.farm-cargo-workshop-recovery-runtime.v1'){throw "Workshop recovery schema mismatch: $($workshop.schema)"}
if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown'){
    if($gate.git_sha -ne $ExpectedGitSha){throw "Technical gate SHA mismatch: gate=$($gate.git_sha), expected=$ExpectedGitSha"}
    if($workshop.git_sha -ne $ExpectedGitSha){throw "Workshop runtime SHA mismatch: evidence=$($workshop.git_sha), expected=$ExpectedGitSha"}
}

foreach($field in @(
    'tow_completed','tow_single_charge','tow_damage_preserved','tow_identity_preserved','workshop_destination',
    'workshop_hold_detected','garage_recall_blocked','garage_recall_no_charge','garage_recall_no_move',
    'workshop_service_applied','workshop_single_charge','workshop_hold_cleared','mechanical_repaired','refuelled',
    'exact_vehicle_preserved','cargo_timer_continued','cargo_integrity_not_improved','wrong_vehicle_rejected',
    'hill_handoff','final_handoff','final_save','authority_cleared'
)){
    if(-not $workshop.$field){throw "Workshop recovery runtime missing PASS gate: $field"}
}
if([int]$workshop.tow_locked_quote -le 0 -or [int]$workshop.workshop_quote -le 0){throw 'Workshop recovery did not prove positive tow/workshop charges.'}
if([int]$workshop.payout_delta -le 0 -or [int]$workshop.cargo_completed_runs_delta -ne 1 -or [int]$workshop.logistics_reputation_delta -le 0){throw 'Workshop recovery did not prove exactly one authoritative delivery completion.'}
if([int]$workshop.native_tow_complete_markers -lt 1){throw 'Workshop recovery lacks production tow-complete evidence.'}
if([int]$workshop.diagnostic_failure_count -ne 0){throw 'Workshop recovery runtime reported diagnostic failures.'}
if([string]::IsNullOrWhiteSpace([string]$workshop.stable_vehicle_id)){throw 'Workshop recovery stable vehicle id is empty.'}

$gate.schema=13
$fields=[ordered]@{
    farm_cargo_workshop_recovery_runtime='PASS'
    farm_cargo_workshop_recovery_schema=$workshop.schema
    farm_cargo_workshop_recovery_vehicle_id=$workshop.stable_vehicle_id
    farm_cargo_workshop_recovery_tow_quote=$workshop.tow_locked_quote
    farm_cargo_workshop_recovery_workshop_quote=$workshop.workshop_quote
    farm_cargo_workshop_recovery_hold_detected=$workshop.workshop_hold_detected
    farm_cargo_workshop_recovery_garage_recall_blocked=$workshop.garage_recall_blocked
    farm_cargo_workshop_recovery_workshop_service=$workshop.workshop_service_applied
    farm_cargo_workshop_recovery_hold_cleared=$workshop.workshop_hold_cleared
    farm_cargo_workshop_recovery_exact_vehicle=$workshop.exact_vehicle_preserved
    farm_cargo_workshop_recovery_wrong_vehicle_rejected=$workshop.wrong_vehicle_rejected
    farm_cargo_workshop_recovery_payout_delta=$workshop.payout_delta
    farm_cargo_workshop_recovery_reputation_delta=$workshop.logistics_reputation_delta
    workshop_recovery_gate_promoted_utc=(Get-Date).ToUniversalTime().ToString('o')
}
foreach($entry in $fields.GetEnumerator()){
    $gate | Add-Member -NotePropertyName $entry.Key -NotePropertyValue $entry.Value -Force
}
$gate|ConvertTo-Json -Depth 10|Set-Content -Encoding UTF8 $gatePath
Write-Host '[GTT] Demo technical evidence gate promoted to schema 13: workshop recovery PASS'
Write-Host "[GTT] Evidence: $gatePath"
