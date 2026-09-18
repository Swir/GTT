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
scenario_step = 'Evaluate structural limp-home, persistence and workshop recovery scenario'
packaged_step = 'Evaluate packaged gameplay smoke'
workflow_order_ok = scenario_step in workflow and packaged_step in workflow and workflow.index(scenario_step) < workflow.index(packaged_step)

runtime_match = re.search(r'-MinimumAliveSeconds\s+(\d+)\s+-LaunchTimeoutSeconds\s+(\d+)', workflow)
gameplay_runtime_match = re.search(r'-MinimumRuntimeSeconds\s+(\d+)', workflow)
runtime_window_ok = False
minimum_alive = 0
launch_timeout = 0
if runtime_match and gameplay_runtime_match:
    minimum_alive = int(runtime_match.group(1))
    launch_timeout = int(runtime_match.group(2))
    gameplay_minimum = int(gameplay_runtime_match.group(1))
    runtime_window_ok = minimum_alive >= 125 and gameplay_minimum >= minimum_alive and launch_timeout > minimum_alive

# This verifier was introduced for the 0.1.14 candidate. Later additive evidence milestones
# must be allowed to advance the candidate version without weakening the original scenario gates.
version_match = re.search(r"default:\s*'([0-9]+)\.([0-9]+)\.([0-9]+)'", workflow)
candidate_version = None
candidate_version_ok = False
if version_match:
    candidate_tuple = tuple(int(version_match.group(i)) for i in range(1, 4))
    candidate_version = '.'.join(version_match.groups())
    candidate_version_ok = candidate_tuple >= (0, 1, 14)

checks = {
    'core world subsystem': 'UTickableWorldSubsystem' in h,
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
    'workflow order': workflow_order_ok,
    'extended packaged runtime': runtime_window_ok,
    'artifact retained': '\\DEMO_SCENARIO.json' in workflow,
    'current candidate version': candidate_version_ok,
    'current build evidence': all(x in workflow for x in ['WIN64_PREFLIGHT.json', 'BUILD_ATTEMPT.json', 'RUNTIME_SMOKE.json', 'DEMO_TECHNICAL_GATE.json']),
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('Deterministic demo scenario verification failed: ' + ', '.join(failed))
print(f'Deterministic demo scenario verification passed under current {candidate_version} candidate workflow ({len(checks)} checks; runtime={minimum_alive}s, timeout={launch_timeout}s).')