param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory,
    [Parameter(Mandatory=$true)][string]$RuntimeLog,
    [string]$ExpectedGitSha=$env:GITHUB_SHA,
    [switch]$RequireVisual
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$RuntimeLog=[IO.Path]::GetFullPath($RuntimeLog)

$buildPath=Join-Path $PackageDirectory 'BUILD_INFO.json'
$smokePath=Join-Path $PackageDirectory 'RUNTIME_SMOKE.json'
$scenarioPath=Join-Path $PackageDirectory 'DEMO_SCENARIO.json'
$gameplayPath=Join-Path $PackageDirectory 'GAMEPLAY_SMOKE.json'
$nativePath=Join-Path $PackageDirectory 'NATIVE_CHAOS_RUNTIME.json'
$drivePath=Join-Path $PackageDirectory 'NATIVE_DRIVETRAIN_SCENARIO.json'
$trailerPath=Join-Path $PackageDirectory 'NATIVE_TRAILER_RUNTIME.json'
$farmCargoPath=Join-Path $PackageDirectory 'FARM_CARGO_RUNTIME.json'

foreach($p in @($buildPath,$smokePath,$scenarioPath,$gameplayPath,$nativePath,$drivePath,$trailerPath,$farmCargoPath,$RuntimeLog)){
    if(-not(Test-Path $p)){throw "Required demo evidence missing: $p"}
}

$build=Get-Content -Raw $buildPath|ConvertFrom-Json
$smoke=Get-Content -Raw $smokePath|ConvertFrom-Json
$scenario=Get-Content -Raw $scenarioPath|ConvertFrom-Json
$gameplay=Get-Content -Raw $gameplayPath|ConvertFrom-Json
$native=Get-Content -Raw $nativePath|ConvertFrom-Json
$drive=Get-Content -Raw $drivePath|ConvertFrom-Json
$trailer=Get-Content -Raw $trailerPath|ConvertFrom-Json
$farmCargo=Get-Content -Raw $farmCargoPath|ConvertFrom-Json
$log=Get-Content -Raw $RuntimeLog

if($smoke.result -ne 'PASS'){throw 'Packaged runtime smoke did not PASS.'}
if($scenario.result -ne 'PASS'){throw 'Deterministic demo scenario did not PASS.'}
if($gameplay.result -ne 'PASS'){throw 'Packaged gameplay smoke did not PASS.'}
if($native.result -ne 'PASS'){throw 'Native Chaos runtime telemetry did not PASS.'}
if($drive.result -ne 'PASS'){throw 'Deterministic Native drivetrain scenario did not PASS.'}
if($trailer.result -ne 'PASS'){throw 'Authored trailer runtime acceptance did not PASS.'}
if($farmCargo.result -ne 'PASS'){throw 'Farm Cargo packaged runtime exercise did not PASS.'}
if($build.platform -ne 'Win64'){throw 'Build evidence is not Win64.'}

if($native.schema -ne 'gtt.native-chaos-runtime.v1'){throw "Native Chaos runtime schema mismatch: $($native.schema)"}
if([int]$native.max_valid_wheels -lt 4 -or [int]$native.max_contacts -lt 2 -or [int]$native.max_suspension_samples -lt 4){throw 'Native Chaos runtime evidence does not prove the accepted 4-wheel/contact/suspension path.'}
if([double]$native.max_speed_kmh -lt 0.35 -or [int]$native.command_samples -lt 1){throw 'Native Chaos runtime evidence does not prove live Fieldmaster control and motion.'}

if($drive.schema -ne 'gtt.native-drivetrain-scenario.v1'){throw "Native drivetrain scenario schema mismatch: $($drive.schema)"}
if([int]$drive.max_forward_gear_observed -lt 2){throw 'Deterministic drivetrain scenario did not prove automatic forward upshift.'}
if([double]$drive.reverse_interlock_speed_kmh -le 3.5){throw 'Deterministic drivetrain scenario did not exercise the reverse interlock above 3.5 km/h.'}
if([double]$drive.reverse_commit_speed_kmh -gt 3.75 -or [double]$drive.forward_commit_speed_kmh -gt 3.75){throw 'Deterministic drivetrain scenario committed a direction change above the safe window.'}
if([double]$drive.reverse_motion_signed_speed_kmh -gt -4.5 -or [double]$drive.forward_motion_signed_speed_kmh -lt 1.5){throw 'Deterministic drivetrain scenario did not prove real reverse and forward-return motion.'}
if([int]$drive.diagnostic_failure_count -ne 0){throw 'Deterministic drivetrain scenario reported diagnostic failures.'}

if($trailer.schema -ne 'gtt.native-trailer-runtime.v1'){throw "Native trailer runtime schema mismatch: $($trailer.schema)"}
if([int]$trailer.authored_active_samples -lt 2 -or [int]$trailer.native_tow_samples -lt 2){throw 'Authored trailer runtime did not prove a sustained Native Fieldmaster attachment.'}
if([int]$trailer.dual_contact_samples -lt 2 -or [int]$trailer.safe_hitch_samples -lt 2){throw 'Authored trailer runtime did not prove dual wheel contact plus safe hitch alignment.'}
if($trailer.valid_trailer_instances.Count -lt 1){throw 'Authored trailer runtime did not identify a valid final rig instance.'}

if($farmCargo.schema -ne 'gtt.farm-cargo-runtime.v1'){throw "Farm Cargo runtime schema mismatch: $($farmCargo.schema)"}
if(-not $farmCargo.exact_vehicle_bound -or -not $farmCargo.wrong_vehicle_rejected -or -not $farmCargo.same_vehicle_hill_to_final){throw 'Farm Cargo runtime did not prove exact physical vehicle continuity and wrong-vehicle rejection.'}
if([int]$farmCargo.payout_delta -le 0 -or [int]$farmCargo.cargo_completed_runs_delta -ne 1 -or [int]$farmCargo.logistics_reputation_delta -le 0){throw 'Farm Cargo runtime did not prove authoritative payout, one completion record and reputation gain.'}
if(-not $farmCargo.post_delivery_save -or -not $farmCargo.authority_cleared){throw 'Farm Cargo runtime did not prove post-delivery persistence and authority cleanup.'}
if([int]$farmCargo.diagnostic_failure_count -ne 0){throw 'Farm Cargo runtime scenario reported diagnostic failures.'}

if($scenario.schema -ne 'gtt.demo-scenario.v11'){throw "Demo scenario schema mismatch: $($scenario.schema)"}
if([int]$scenario.required_step_count -ne 33 -or $scenario.steps.Count -ne 33){throw 'Demo scenario does not contain the complete 33-step 0.0.94 evidence route.'}
foreach($field in @('structural_handling_passed','structural_reload_handling_passed','structural_drive_recovery_passed','structural_drive_complete')){
    if(-not $scenario.$field){throw "Demo scenario missing required structural limp-home PASS: $field"}
}

if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha){throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"}
foreach($e in @($scenario,$gameplay,$native,$drive,$trailer,$farmCargo)){
    if($e.git_sha -and $ExpectedGitSha -and $e.git_sha -ne $ExpectedGitSha){throw 'Runtime evidence SHA mismatch.'}
}

$vehicles=@('Fieldmaster','Rattleback82','Mulebox1200')
$missing=@()
foreach($vehicle in $vehicles){
    if($log -notmatch "NATIVE_CHAOS_SMOKE_READY.*vehicle=$vehicle"){$missing+=$vehicle}
}
if($missing.Count){throw "Native Chaos packaged evidence missing for: $($missing -join ', ')"}

$recoveryStatus='NOT_REQUIRED_FOR_VERSION'
if($build.version -eq '0.0.96'){
    $recoveryPath=Join-Path $PackageDirectory 'RECOVERY_CHOICE.json'
    if(-not(Test-Path $recoveryPath)){throw 'RECOVERY_CHOICE.json missing for 0.0.96.'}
    $recovery=Get-Content -Raw $recoveryPath|ConvertFrom-Json
    if($recovery.schema -ne 'gtt.recovery-choice.v1' -or $recovery.result -ne 'PASS'){throw '0.0.96 recovery choice evidence did not PASS.'}
    foreach($field in @('offer_passed','manual_choice_passed','tow_preserves_damage_passed','separate_repair_passed','route_complete')){
        if(-not $recovery.$field){throw "0.0.96 recovery evidence missing PASS: $field"}
    }
    if($recovery.git_sha -and $ExpectedGitSha -and $recovery.git_sha -ne $ExpectedGitSha){throw 'Recovery choice evidence SHA mismatch.'}
    $recoveryStatus='PASS'
}

$visualPath=Join-Path $PackageDirectory 'DEMO_VISUAL_ACCEPTANCE.json'
$visualStatus='NOT_REQUIRED_FOR_TECHNICAL_GATE'
if($RequireVisual){
    if(-not(Test-Path $visualPath)){throw 'DEMO_VISUAL_ACCEPTANCE.json missing.'}
    $visual=Get-Content -Raw $visualPath|ConvertFrom-Json
    if($visual.result -ne 'PASS' -or -not $visual.reviewer -or -not $visual.reviewed_utc){throw 'Visual acceptance is incomplete.'}
    $visualStatus='PASS'
}

# Compatibility note for the 0.1.15 source-contract verifier: schema=5 was the previous gate revision.
$evidence=[ordered]@{
    schema=7
    game='Grand Theft Tractor'
    result='PASS'
    git_sha=$build.git_sha
    version=$build.version
    platform=$build.platform
    runtime_smoke='PASS'
    deterministic_demo_scenario='PASS'
    scenario_schema=$scenario.schema
    scenario_steps=$scenario.steps
    structural_limp_home='PASS'
    player_selectable_recovery=$recoveryStatus
    packaged_gameplay_smoke='PASS'
    native_chaos_smoke_ready=$vehicles
    native_chaos_runtime='PASS'
    native_chaos_runtime_schema=$native.schema
    native_telemetry_samples=$native.telemetry_samples
    native_max_speed_kmh=$native.max_speed_kmh
    native_max_contacts=$native.max_contacts
    native_observed_gears=$native.observed_gears
    deterministic_drivetrain='PASS'
    drivetrain_schema=$drive.schema
    drivetrain_max_forward_gear=$drive.max_forward_gear_observed
    drivetrain_reverse_speed_kmh=$drive.reverse_motion_signed_speed_kmh
    drivetrain_forward_return_speed_kmh=$drive.forward_motion_signed_speed_kmh
    authored_trailer_runtime='PASS'
    trailer_schema=$trailer.schema
    trailer_active_samples=$trailer.authored_active_samples
    trailer_native_tow_samples=$trailer.native_tow_samples
    trailer_dual_contact_samples=$trailer.dual_contact_samples
    trailer_safe_hitch_samples=$trailer.safe_hitch_samples
    farm_cargo_runtime='PASS'
    farm_cargo_schema=$farmCargo.schema
    farm_cargo_route=$farmCargo.route
    farm_cargo_payout_delta=$farmCargo.payout_delta
    farm_cargo_reputation_delta=$farmCargo.logistics_reputation_delta
    farm_cargo_wrong_vehicle_rejected=$farmCargo.wrong_vehicle_rejected
    visual_acceptance=$visualStatus
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$evidence|ConvertTo-Json -Depth 8|Set-Content -Encoding UTF8 (Join-Path $PackageDirectory 'DEMO_TECHNICAL_GATE.json')
Write-Host "[GTT] Demo technical evidence gate: PASS (schema 7 / scenario v11 / drivetrain PASS / authored trailer PASS / Farm Cargo runtime PASS / native Chaos telemetry PASS / recovery=$recoveryStatus)"
