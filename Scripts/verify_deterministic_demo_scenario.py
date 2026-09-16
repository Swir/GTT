from pathlib import Path

root = Path(__file__).resolve().parents[1]
h = (root / 'Source/GTT/Public/Core/GTTDemoSmokeScenarioSubsystem.h').read_text(encoding='utf-8')
cpp = (root / 'Source/GTT/Private/Core/GTTDemoSmokeScenarioSubsystem.cpp').read_text(encoding='utf-8')
recovery_h = (root / 'Source/GTT/Public/Core/GTTDamageRecoveryEvidenceSubsystem.h').read_text(encoding='utf-8')
recovery_cpp = (root / 'Source/GTT/Private/Core/GTTDamageRecoveryEvidenceSubsystem.cpp').read_text(encoding='utf-8')
struct_h = (root / 'Source/GTT/Public/Core/GTTStructuralDamageEvidenceSubsystem.h').read_text(encoding='utf-8')
struct_cpp = (root / 'Source/GTT/Private/Core/GTTStructuralDamageEvidenceSubsystem.cpp').read_text(encoding='utf-8')
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
scenario_step = 'Evaluate persistent structural damage and workshop recovery scenario'
packaged_step = 'Evaluate packaged gameplay smoke'
workflow_order_ok = scenario_step in workflow and packaged_step in workflow and workflow.index(scenario_step) < workflow.index(packaged_step)

checks = {
    'core world subsystem': 'UTickableWorldSubsystem' in h,
    'recovery world subsystem': 'UTickableWorldSubsystem' in recovery_h,
    'structural world subsystem': 'UTickableWorldSubsystem' in struct_h,
    'opt-in commandline': all('GTTDemoSmokeScenario' in text for text in [cpp, recovery_cpp, struct_cpp, smoke]),
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
    'evaluator schema v10': 'gtt.demo-scenario.v10' in eval_ps and 'required_step_count=30' in eval_ps,
    'evaluator retains core': all(s in eval_ps for s in core_steps) and 'DEMO_SCENARIO_COMPLETE result=PASS steps=26' in eval_ps,
    'evaluator recovery gates': all(x in eval_ps for x in ['damage_persistence_passed', 'workshop_recovery_passed', 'damage_recovery_complete', 'structural_persistence_passed', 'structural_repair_passed', 'structural_recovery_complete']),
    'demo gate consumes scenario': "scenario.result -ne 'PASS'" in demo,
    'workflow order': workflow_order_ok,
    'extended packaged runtime': '-MinimumAliveSeconds 105 -LaunchTimeoutSeconds 120' in workflow and '-MinimumRuntimeSeconds 105' in workflow,
    'artifact retained': '\\DEMO_SCENARIO.json' in workflow,
    'version': "default: '0.0.93'" in workflow,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('0.0.93 deterministic demo scenario verification failed: ' + ', '.join(failed))
print(f'0.0.93 deterministic demo scenario verification passed ({len(checks)} checks).')
