#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
h = (root / 'Source/GTT/Public/Core/GTTDemoSmokeScenarioSubsystem.h').read_text(encoding='utf-8')
cpp = (root / 'Source/GTT/Private/Core/GTTDemoSmokeScenarioSubsystem.cpp').read_text(encoding='utf-8')
recovery_h = (root / 'Source/GTT/Public/Core/GTTDamageRecoveryEvidenceSubsystem.h').read_text(encoding='utf-8')
recovery_cpp = (root / 'Source/GTT/Private/Core/GTTDamageRecoveryEvidenceSubsystem.cpp').read_text(encoding='utf-8')
struct_h = (root / 'Source/GTT/Public/Core/GTTStructuralDamageEvidenceSubsystem.h').read_text(encoding='utf-8')
struct_cpp = (root / 'Source/GTT/Private/Core/GTTStructuralDamageEvidenceSubsystem.cpp').read_text(encoding='utf-8')
drive_h = (root / 'Source/GTT/Public/Vehicles/GTTStructuralDriveConsequenceSubsystem.h').read_text(encoding='utf-8')
drive_cpp = (root / 'Source/GTT/Private/Vehicles/GTTStructuralDriveConsequenceSubsystem.cpp').read_text(encoding='utf-8')
smoke = (root / 'Scripts/smoke_test_windows.ps1').read_text(encoding='utf-8')
eval_ps = (root / 'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
demo = (root / 'Scripts/evaluate_demo_candidate.ps1').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
runner = (root / 'Scripts/run_win64_candidate_acceptance.ps1').read_text(encoding='utf-8')
attestor = (root / 'Scripts/write_win64_candidate_attestation.ps1').read_text(encoding='utf-8')

core_steps = [
    'WORLD', 'HUD', 'TRAFFIC', 'NPC', 'MISSION', 'COMBAT',
    'FIELDMASTER', 'FIELDMASTER_MOTION', 'FIELDMASTER_CONTROL',
    'RATTLEBACK', 'RATTLEBACK_MOTION', 'RATTLEBACK_CONTROL',
    'MULEBOX', 'MULEBOX_MOTION', 'MULEBOX_CONTROL',
    'WANTED_COMPONENT', 'WANTED_ESCALATION', 'POLICE_RESPONSE',
    'PURSUIT_ACTIVE', 'PURSUIT_CLOSING', 'ROADBLOCK_ACTIVE',
    'INTERCEPTION_ACTIVE', 'ROADBLOCK_PHYSICAL_CROSSING',
    'HANDLING_CONSEQUENCE', 'POST_SPIKE_ESCAPE', 'SAVE'
]

# The Win64 workflow now delegates the executable sequence to the canonical runner.
# Historical verifiers must inspect that runner rather than requiring duplicated YAML steps.
scenario_eval = 'evaluate_demo_scenario.ps1'
gameplay_eval = 'evaluate_packaged_gameplay_smoke.ps1'
workflow_order_ok = scenario_eval in runner and gameplay_eval in runner and runner.index(scenario_eval) < runner.index(gameplay_eval)

runtime_match = re.search(r'"-MinimumAliveSeconds",\s*(\d+).*?"-LaunchTimeoutSeconds",\s*(\d+)', runner, flags=re.S)
gameplay_runtime_match = re.search(r'"-MinimumRuntimeSeconds",\s*(\d+)', runner)
runtime_window_ok = False
minimum_alive = launch_timeout = gameplay_minimum = 0
if runtime_match and gameplay_runtime_match:
    minimum_alive = int(runtime_match.group(1))
    launch_timeout = int(runtime_match.group(2))
    gameplay_minimum = int(gameplay_runtime_match.group(1))
    runtime_window_ok = minimum_alive >= 125 and gameplay_minimum >= minimum_alive and launch_timeout > minimum_alive

version_match = re.search(r"default:\s*'([0-9]+)\.([0-9]+)\.([0-9]+)'", workflow)
candidate_version = None
candidate_version_ok = False
if version_match:
    candidate_version = tuple(int(part) for part in version_match.groups())
    candidate_version_ok = candidate_version >= (0, 1, 14)

sealed_demo_evidence = (
    'DEMO_SCENARIO.json' in attestor
    and 'FINAL_SHA256SUMS.txt' in workflow
    and 'run_win64_attested_candidate_acceptance.ps1' in workflow
)
current_build_evidence = all(x in attestor for x in [
    'WIN64_PREFLIGHT.json', 'BUILD_ATTEMPT.json', 'RUNTIME_SMOKE.json', 'DEMO_TECHNICAL_GATE.json'
])

checks = {
    'core world subsystem': 'UTickableWorldSubsystem' in h,
    'fresh acceptance fleet ownership fixture': all(x in cpp for x in ['PrepareAcceptanceFleet', 'MarkOwnedByPlayer', 'DEMO_SCENARIO_FLEET_PREP result=PASS']),
    'recovery world subsystem': 'UTickableWorldSubsystem' in recovery_h,
    'structural world subsystem': 'UTickableWorldSubsystem' in struct_h,
    'structural drive world subsystem': 'UTickableWorldSubsystem' in drive_h,
    'opt-in commandline': all('GTTDemoSmokeScenario' in text for text in [cpp, recovery_cpp, struct_cpp, drive_cpp, smoke]),
    'explicit 26 core markers': all(f'TEXT("{s}")' in cpp for s in core_steps) and 'steps=26' in cpp,
    'native control actuation': all(x in cpp for x in ['SetThrottleInput', 'SetSteeringInput', 'SetBrakeInput', 'DEMO_SCENARIO_CONTROL']),
    'live wheel motion evidence': all(x in cpp for x in ['GetWheelState', 'bInContact', 'NormalizedSuspensionLength', 'GetVelocity().SizeSquared2D()']),
    'wanted-4 escalation': 'AddHeat(130.f)' in cpp and 'GetWantedLevel()>=4' in cpp,
    'active police response': 'GetActiveFootUnitCount()>0' in cpp,
    'active pursuit vehicle': 'GetActivePursuitVehicleCount()>0' in cpp,
    'pursuit interaction proof': all(x in cpp for x in ['PursuitStartDistance', 'Distance+250.f<PursuitStartDistance', 'PURSUIT_CLOSING']),
    'physical crossing proof': all(x in cpp for x in ['DriveNativeRoadblockCrossing', 'GetSpikeStripWorldLocation', 'ROADBLOCK_PHYSICAL_CROSSING', 'HANDLING_CONSEQUENCE']),
    'post-spike escape proof': all(x in cpp for x in ['POST_SPIKE_ESCAPE', 'PostSpikeEscapeStartSeconds', 'GetRuntimeWheelRisk', 'GetRuntimeThrottleLimit', 'GetRuntimeSteeringLimit']),
    'damage persistence proof': all(x in recovery_cpp for x in ['SaveProgress()', 'LoadProgress()', 'DEMO_SCENARIO_DAMAGE_PERSISTENCE', 'TryActivateLegacyTakeover']),
    'paid workshop proof': all(x in recovery_cpp for x in ['EGTTServiceType::Workshop', 'Interact_Implementation(PlayerPawn)', 'CashBeforeWorkshop', 'CashAfterWorkshop', 'DEMO_SCENARIO_WORKSHOP_RECOVERY']),
    'structural persistence proof': all(x in struct_cpp for x in ['ApplyScriptedImpactDamage', 'RoadStructuralDamage.FindByPredicate', 'DEMO_SCENARIO_STRUCTURAL_PERSISTENCE', 'SavedPanelMask']),
    'structural repair proof': all(x in struct_cpp for x in ['SavedRepairSurcharge', 'Interact_Implementation(PlayerPawn)', 'DEMO_SCENARIO_STRUCTURAL_REPAIR', 'Paid > SavedRepairSurcharge']),
    'structural physical consequence': all(x in drive_cpp for x in ['Mesh->AddForce(DragForce', 'Mesh->AddForce(LateralForce', 'bLimpHomeActive', 'DEMO_SCENARIO_STRUCTURAL_HANDLING']),
    'structural consequence save-load proof': all(x in drive_cpp for x in ['SaveProgress()', 'RestorePersistentBodyDamage(Pristine, 0)', 'LoadProgress()', 'DEMO_SCENARIO_STRUCTURAL_RELOAD_HANDLING']),
    'structural consequence workshop proof': all(x in drive_cpp for x in ['Interact_Implementation(PlayerPawn)', 'DEMO_SCENARIO_STRUCTURAL_DRIVE_RECOVERY', 'limp_after=NO']),
    'evaluator schema v11': 'gtt.demo-scenario.v11' in eval_ps and 'required_step_count=33' in eval_ps,
    'evaluator retains core': all(s in eval_ps for s in core_steps) and 'DEMO_SCENARIO_COMPLETE result=PASS steps=26' in eval_ps,
    'evaluator recovery gates': all(x in eval_ps for x in ['damage_persistence_passed', 'workshop_recovery_passed', 'damage_recovery_complete', 'structural_persistence_passed', 'structural_repair_passed', 'structural_recovery_complete', 'structural_handling_passed', 'structural_reload_handling_passed', 'structural_drive_recovery_passed']),
    'demo gate consumes scenario': "scenario.result -ne 'PASS'" in demo,
    'canonical runner order': workflow_order_ok,
    'extended packaged runtime': runtime_window_ok,
    'scenario sealed in candidate': sealed_demo_evidence,
    'candidate version not regressed below 0.1.14': candidate_version_ok,
    'current build evidence sealed': current_build_evidence,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('Deterministic demo scenario verification failed: ' + ', '.join(failed))
label = '.'.join(str(part) for part in candidate_version) if candidate_version else 'unknown'
print(f'Deterministic demo scenario verification passed under additive candidate {label} ({len(checks)} checks; runtime={minimum_alive}s, timeout={launch_timeout}s).')
