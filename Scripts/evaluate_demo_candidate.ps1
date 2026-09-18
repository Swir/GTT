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
$farmCargoRecoveryPath=Join-Path $PackageDirectory 'FARM_CARGO_RECOVERY_RUNTIME.json'
$farmCargoBreakdownPath=Join-Path $PackageDirectory 'FARM_CARGO_BREAKDOWN_RUNTIME.json'
$farmCargoDispatchPath=Join-Path $PackageDirectory 'FARM_CARGO_DISPATCH_RUNTIME.json'
$farmCargoDispatchPersistencePath=Join-Path $PackageDirectory 'FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME.json'

foreach($p in @($buildPath,$smokePath,$scenarioPath,$gameplayPath,$nativePath,$drivePath,$trailerPath,$farmCargoPath,$farmCargoRecoveryPath,$farmCargoBreakdownPath,$farmCargoDispatchPath,$farmCargoDispatchPersistencePath,$RuntimeLog)){
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
$farmCargoRecovery=Get-Content -Raw $farmCargoRecoveryPath|ConvertFrom-Json
$farmCargoBreakdown=Get-Content -Raw $farmCargoBreakdownPath|ConvertFrom-Json
$farmCargoDispatch=Get-Content -Raw $farmCargoDispatchPath|ConvertFrom-Json
$farmCargoDispatchPersistence=Get-Content -Raw $farmCargoDispatchPersistencePath|ConvertFrom-Json
$log=Get-Content -Raw $RuntimeLog

if($smoke.result -ne 'PASS'){throw 'Packaged runtime smoke did not PASS.'}
if($scenario.result -ne 'PASS'){throw 'Deterministic demo scenario did not PASS.'}
if($gameplay.result -ne 'PASS'){throw 'Packaged gameplay smoke did not PASS.'}
if($native.result -ne 'PASS'){throw 'Native Chaos runtime telemetry did not PASS.'}
if($drive.result -ne 'PASS'){throw 'Deterministic Native drivetrain scenario did not PASS.'}
if($trailer.result -ne 'PASS'){throw 'Authored trailer runtime acceptance did not PASS.'}
if($farmCargo.result -ne 'PASS'){throw 'Farm Cargo packaged runtime exercise did not PASS.'}
if($farmCargoRecovery.result -ne 'PASS'){throw 'Farm Cargo save/load recovery runtime did not PASS.'}
if($farmCargoBreakdown.result -ne 'PASS'){throw 'Farm Cargo emergency-patch/breakdown/tow recovery runtime did not PASS.'}
if($farmCargoDispatch.result -ne 'PASS'){throw 'Farm Cargo roadside dispatch contract runtime did not PASS.'}
if($farmCargoDispatchPersistence.result -ne 'PASS'){throw 'Farm Cargo roadside dispatch persistence runtime did not PASS.'}
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

if($farmCargoRecovery.schema -ne 'gtt.farm-cargo-recovery-runtime.v1'){throw "Farm Cargo recovery runtime schema mismatch: $($farmCargoRecovery.schema)"}
foreach($field in @(
    'same_model_vehicle_identity_unique','loaded_checkpoint_saved','loaded_stage_restored','loaded_vehicle_id_restored',
    'recreated_actor_rebound','loaded_timer_restored','loaded_integrity_restored','loaded_stock_stable',
    'wrong_vehicle_rejected_after_reload','relay_checkpoint_saved','relay_stage_restored','relay_vehicle_id_restored',
    'relay_same_vehicle','relay_timer_restored','relay_integrity_restored','relay_stock_stable',
    'completion_reload_stable','authority_cleared','final_save'
)){
    if(-not $farmCargoRecovery.$field){throw "Farm Cargo recovery runtime missing PASS gate: $field"}
}
if([int]$farmCargoRecovery.payout_delta -le 0 -or [int]$farmCargoRecovery.cargo_completed_runs_delta -ne 1 -or [int]$farmCargoRecovery.logistics_reputation_delta -le 0){throw 'Farm Cargo recovery runtime did not prove one authoritative completion after mid-route reloads.'}
if(-not $farmCargoRecovery.stable_vehicle_id){throw 'Farm Cargo recovery runtime did not report the stable physical vehicle identity.'}
if([int]$farmCargoRecovery.diagnostic_failure_count -ne 0){throw 'Farm Cargo recovery runtime scenario reported diagnostic failures.'}

if($farmCargoBreakdown.schema -ne 'gtt.farm-cargo-breakdown-runtime.v2'){throw "Farm Cargo breakdown runtime schema mismatch: $($farmCargoBreakdown.schema)"}
foreach($field in @(
    'emergency_patch_breakdown_proven','player_authorized_patch','patch_completed','patch_exact_vehicle_identity_preserved',
    'patch_body_preserved','patch_timer_continued','patch_cargo_integrity_not_improved','patch_workshop_still_required',
    'production_pre_patch_checkpoint','production_post_patch_identity_verification','native_patch_request_marker','native_patch_complete_marker',
    'native_breakdown_proven','player_authorized_tow','tow_completed','exact_vehicle_identity_preserved',
    'timer_continued','cargo_integrity_not_improved','damage_preserved','wrong_vehicle_rejected_after_tow',
    'authority_cleared','final_save','production_pre_tow_checkpoint','production_post_tow_identity_verification',
    'native_tow_request_marker','native_tow_complete_marker'
)){
    if(-not $farmCargoBreakdown.$field){throw "Farm Cargo emergency-patch/breakdown runtime missing PASS gate: $field"}
}
if([int]$farmCargoBreakdown.patch_cost_delta -le 0){throw 'Farm Cargo breakdown runtime did not prove a paid emergency patch.'}
if([double]$farmCargoBreakdown.patch_condition_after -lt 0.299 -or [double]$farmCargoBreakdown.patch_tire_after -lt 0.319 -or [double]$farmCargoBreakdown.patch_fuel_after -lt 4.99){throw 'Farm Cargo emergency patch did not prove the limp-home floors.'}
if([double]$farmCargoBreakdown.patch_cooldown_wait_seconds -lt 12.0){throw 'Farm Cargo emergency patch route did not survive the real roadside recovery cooldown.'}
if([int]$farmCargoBreakdown.tow_cost_delta -le 0){throw 'Farm Cargo breakdown runtime did not prove a paid roadside tow after the patch.'}
if([int]$farmCargoBreakdown.payout_delta -le 0 -or [int]$farmCargoBreakdown.cargo_completed_runs_delta -ne 1 -or [int]$farmCargoBreakdown.logistics_reputation_delta -le 0){throw 'Farm Cargo breakdown runtime did not prove one authoritative completion after emergency patch and paid tow.'}
if(-not $farmCargoBreakdown.stable_vehicle_id){throw 'Farm Cargo breakdown runtime did not report the stable physical vehicle identity.'}
if([int]$farmCargoBreakdown.diagnostic_failure_count -ne 0){throw 'Farm Cargo breakdown runtime scenario reported diagnostic failures.'}

if($farmCargoDispatch.schema -ne 'gtt.farm-cargo-dispatch-runtime.v1'){throw "Farm Cargo dispatch runtime schema mismatch: $($farmCargoDispatch.schema)"}
foreach($field in @(
    'patch_live_eta_advanced','patch_cancel_no_charge','tow_live_eta_advanced','tow_cancel_no_charge',
    'patch_charge_matched','exact_vehicle_preserved','cargo_timer_continued','cargo_integrity_not_improved',
    'wrong_vehicle_rejected','hill_handoff','final_handoff','final_save','authority_cleared'
)){
    if(-not $farmCargoDispatch.$field){throw "Farm Cargo dispatch runtime missing PASS gate: $field"}
}
if([int]$farmCargoDispatch.patch_locked_quote -le 0 -or [int]$farmCargoDispatch.tow_locked_quote -le 0 -or [int]$farmCargoDispatch.patch_rerequest_locked_quote -le 0){throw 'Farm Cargo dispatch runtime did not prove positive request-time locked quotes.'}
if([int]$farmCargoDispatch.payout_delta -le 0 -or [int]$farmCargoDispatch.cargo_completed_runs_delta -ne 1 -or [int]$farmCargoDispatch.logistics_reputation_delta -le 0){throw 'Farm Cargo dispatch runtime did not prove one authoritative completion after cancellation/re-request.'}
if(-not $farmCargoDispatch.stable_vehicle_id){throw 'Farm Cargo dispatch runtime did not report the stable physical vehicle identity.'}
if([int]$farmCargoDispatch.diagnostic_failure_count -ne 0){throw 'Farm Cargo dispatch runtime scenario reported diagnostic failures.'}

if($farmCargoDispatchPersistence.schema -ne 'gtt.farm-cargo-dispatch-persistence-runtime.v1'){throw "Farm Cargo dispatch persistence schema mismatch: $($farmCargoDispatchPersistence.schema)"}
foreach($field in @(
    'tow_checkpoint_saved','tow_primary_save','tow_primary_load','tow_restore_rearmed','tow_restored','tow_quote_preserved',
    'tow_eta_preserved','tow_exact_vehicle','tow_cancel_no_charge','wanted_checkpoint_saved','wanted_primary_save',
    'wanted_primary_load','wanted_restore_rearmed','wanted_restore_rejected_no_charge','wanted_sidecar_cleared',
    'patch_checkpoint_saved','patch_primary_save','patch_primary_load','patch_restore_rearmed','patch_restored',
    'patch_quote_preserved','patch_eta_preserved','patch_exact_vehicle','patch_no_charge_before_arrival','patch_single_charge',
    'cargo_timer_continued','cargo_integrity_not_improved','wrong_vehicle_rejected','hill_handoff','final_handoff','final_save','authority_cleared'
)){
    if(-not $farmCargoDispatchPersistence.$field){throw "Farm Cargo dispatch persistence runtime missing PASS gate: $field"}
}
if([int]$farmCargoDispatchPersistence.tow_locked_quote -le 0 -or [int]$farmCargoDispatchPersistence.wanted_tow_locked_quote -le 0 -or [int]$farmCargoDispatchPersistence.patch_locked_quote -le 0){throw 'Dispatch persistence runtime did not prove positive locked quotes.'}
if([int]$farmCargoDispatchPersistence.payout_delta -le 0 -or [int]$farmCargoDispatchPersistence.cargo_completed_runs_delta -ne 1 -or [int]$farmCargoDispatchPersistence.logistics_reputation_delta -le 0){throw 'Dispatch persistence runtime did not prove one authoritative completion.'}
if(-not $farmCargoDispatchPersistence.stable_vehicle_id){throw 'Dispatch persistence runtime did not report a stable physical vehicle identity.'}
if([int]$farmCargoDispatchPersistence.guarded_reload_markers -lt 3 -or [int]$farmCargoDispatchPersistence.restored_dispatch_markers -lt 2 -or [int]$farmCargoDispatchPersistence.wanted_reject_markers -lt 1){throw 'Dispatch persistence runtime did not prove guarded reload/restore/Wanted rejection markers.'}
if([int]$farmCargoDispatchPersistence.diagnostic_failure_count -ne 0){throw 'Dispatch persistence runtime scenario reported diagnostic failures.'}

if($scenario.schema -ne 'gtt.demo-scenario.v11'){throw "Demo scenario schema mismatch: $($scenario.schema)"}
if([int]$scenario.required_step_count -ne 33 -or $scenario.steps.Count -ne 33){throw 'Demo scenario does not contain the complete 33-step 0.0.94 evidence route.'}
foreach($field in @('structural_handling_passed','structural_reload_handling_passed','structural_drive_recovery_passed','structural_drive_complete')){
    if(-not $scenario.$field){throw "Demo scenario missing required structural limp-home PASS: $field"}
}

if($ExpectedGitSha -and $ExpectedGitSha -ne 'unknown' -and $build.git_sha -ne $ExpectedGitSha){throw "Build SHA mismatch: package=$($build.git_sha), expected=$ExpectedGitSha"}
foreach($e in @($scenario,$gameplay,$native,$drive,$trailer,$farmCargo,$farmCargoRecovery,$farmCargoBreakdown,$farmCargoDispatch,$farmCargoDispatchPersistence)){
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

# Schema 12 adds mandatory roadside dispatch SaveGame persistence/replay safety on top of schema 11.
$evidence=[ordered]@{
    schema=12
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
    farm_cargo_recovery_runtime='PASS'
    farm_cargo_recovery_schema=$farmCargoRecovery.schema
    farm_cargo_recovery_vehicle_id=$farmCargoRecovery.stable_vehicle_id
    farm_cargo_recovery_loaded_rebind=$farmCargoRecovery.recreated_actor_rebound
    farm_cargo_recovery_relay_rebind=$farmCargoRecovery.relay_same_vehicle
    farm_cargo_recovery_completion_reload=$farmCargoRecovery.completion_reload_stable
    farm_cargo_breakdown_runtime='PASS'
    farm_cargo_breakdown_schema=$farmCargoBreakdown.schema
    farm_cargo_breakdown_vehicle_id=$farmCargoBreakdown.stable_vehicle_id
    farm_cargo_emergency_patch='PASS'
    farm_cargo_patch_cost=$farmCargoBreakdown.patch_cost_delta
    farm_cargo_patch_identity_preserved=$farmCargoBreakdown.patch_exact_vehicle_identity_preserved
    farm_cargo_patch_body_preserved=$farmCargoBreakdown.patch_body_preserved
    farm_cargo_patch_timer_continued=$farmCargoBreakdown.patch_timer_continued
    farm_cargo_patch_integrity_not_improved=$farmCargoBreakdown.patch_cargo_integrity_not_improved
    farm_cargo_patch_workshop_still_required=$farmCargoBreakdown.patch_workshop_still_required
    farm_cargo_breakdown_tow_cost=$farmCargoBreakdown.tow_cost_delta
    farm_cargo_breakdown_payout_delta=$farmCargoBreakdown.payout_delta
    farm_cargo_breakdown_reputation_delta=$farmCargoBreakdown.logistics_reputation_delta
    farm_cargo_breakdown_wrong_vehicle_rejected=$farmCargoBreakdown.wrong_vehicle_rejected_after_tow
    farm_cargo_breakdown_timer_continued=$farmCargoBreakdown.timer_continued
    farm_cargo_breakdown_damage_preserved=$farmCargoBreakdown.damage_preserved
    farm_cargo_dispatch_runtime='PASS'
    farm_cargo_dispatch_schema=$farmCargoDispatch.schema
    farm_cargo_dispatch_vehicle_id=$farmCargoDispatch.stable_vehicle_id
    farm_cargo_dispatch_patch_locked_quote=$farmCargoDispatch.patch_locked_quote
    farm_cargo_dispatch_patch_eta_advanced=$farmCargoDispatch.patch_live_eta_advanced
    farm_cargo_dispatch_patch_cancel_no_charge=$farmCargoDispatch.patch_cancel_no_charge
    farm_cargo_dispatch_tow_locked_quote=$farmCargoDispatch.tow_locked_quote
    farm_cargo_dispatch_tow_eta_advanced=$farmCargoDispatch.tow_live_eta_advanced
    farm_cargo_dispatch_tow_cancel_no_charge=$farmCargoDispatch.tow_cancel_no_charge
    farm_cargo_dispatch_patch_rerequest_quote=$farmCargoDispatch.patch_rerequest_locked_quote
    farm_cargo_dispatch_patch_charge_matched=$farmCargoDispatch.patch_charge_matched
    farm_cargo_dispatch_exact_vehicle=$farmCargoDispatch.exact_vehicle_preserved
    farm_cargo_dispatch_wrong_vehicle_rejected=$farmCargoDispatch.wrong_vehicle_rejected
    farm_cargo_dispatch_payout_delta=$farmCargoDispatch.payout_delta
    farm_cargo_dispatch_reputation_delta=$farmCargoDispatch.logistics_reputation_delta
    farm_cargo_dispatch_persistence_runtime='PASS'
    farm_cargo_dispatch_persistence_schema=$farmCargoDispatchPersistence.schema
    farm_cargo_dispatch_persistence_vehicle_id=$farmCargoDispatchPersistence.stable_vehicle_id
    farm_cargo_dispatch_persistence_tow_quote=$farmCargoDispatchPersistence.tow_locked_quote
    farm_cargo_dispatch_persistence_tow_restored=$farmCargoDispatchPersistence.tow_restored
    farm_cargo_dispatch_persistence_tow_cancel_no_charge=$farmCargoDispatchPersistence.tow_cancel_no_charge
    farm_cargo_dispatch_persistence_wanted_rejected=$farmCargoDispatchPersistence.wanted_restore_rejected_no_charge
    farm_cargo_dispatch_persistence_patch_quote=$farmCargoDispatchPersistence.patch_locked_quote
    farm_cargo_dispatch_persistence_patch_restored=$farmCargoDispatchPersistence.patch_restored
    farm_cargo_dispatch_persistence_patch_single_charge=$farmCargoDispatchPersistence.patch_single_charge
    farm_cargo_dispatch_persistence_exact_vehicle=$farmCargoDispatchPersistence.patch_exact_vehicle
    farm_cargo_dispatch_persistence_wrong_vehicle_rejected=$farmCargoDispatchPersistence.wrong_vehicle_rejected
    farm_cargo_dispatch_persistence_payout_delta=$farmCargoDispatchPersistence.payout_delta
    farm_cargo_dispatch_persistence_reputation_delta=$farmCargoDispatchPersistence.logistics_reputation_delta
    visual_acceptance=$visualStatus
    evaluated_utc=(Get-Date).ToUniversalTime().ToString('o')
}
$evidence|ConvertTo-Json -Depth 8|Set-Content -Encoding UTF8 (Join-Path $PackageDirectory 'DEMO_TECHNICAL_GATE.json')
Write-Host "[GTT] Demo technical evidence gate: PASS (schema 12 / scenario v11 / drivetrain PASS / authored trailer PASS / Farm Cargo runtime + save/load + emergency patch + re-breakdown/tow + dispatch contract + dispatch persistence PASS / native Chaos telemetry PASS / recovery=$recoveryStatus)"
