from pathlib import Path

root = Path(__file__).resolve().parents[1]
h = (root / 'Source/GTT/Public/Core/GTTDemoSmokeScenarioSubsystem.h').read_text(encoding='utf-8')
cpp = (root / 'Source/GTT/Private/Core/GTTDemoSmokeScenarioSubsystem.cpp').read_text(encoding='utf-8')
smoke = (root / 'Scripts/smoke_test_windows.ps1').read_text(encoding='utf-8')
eval_ps = (root / 'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
demo = (root / 'Scripts/evaluate_demo_candidate.ps1').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')

steps = [
    'WORLD', 'HUD', 'TRAFFIC', 'NPC', 'MISSION', 'COMBAT',
    'FIELDMASTER', 'FIELDMASTER_MOTION', 'FIELDMASTER_CONTROL',
    'RATTLEBACK', 'RATTLEBACK_MOTION', 'RATTLEBACK_CONTROL',
    'MULEBOX', 'MULEBOX_MOTION', 'MULEBOX_CONTROL',
    'WANTED_COMPONENT', 'WANTED_ESCALATION', 'POLICE_RESPONSE',
    'PURSUIT_ACTIVE', 'PURSUIT_CLOSING', 'ROADBLOCK_ACTIVE',
    'INTERCEPTION_ACTIVE', 'ROADBLOCK_PHYSICAL_CROSSING',
    'HANDLING_CONSEQUENCE', 'POST_SPIKE_ESCAPE', 'SAVE'
]

scenario_step = 'Evaluate deterministic post-spike escape dynamics scenario'
packaged_step = 'Evaluate packaged gameplay smoke'
workflow_order_ok = (
    scenario_step in workflow and packaged_step in workflow
    and workflow.index(scenario_step) < workflow.index(packaged_step)
)

checks = {
    'world subsystem': 'UTickableWorldSubsystem' in h,
    'opt-in commandline': 'GTTDemoSmokeScenario' in cpp and 'GTTDemoSmokeScenario' in smoke,
    'explicit 26-step markers': all(f'TEXT("{s}")' in cpp for s in steps) and 'steps=26' in cpp,
    'native control actuation': all(x in cpp for x in ['SetThrottleInput', 'SetSteeringInput', 'SetBrakeInput', 'DEMO_SCENARIO_CONTROL']),
    'live wheel motion evidence': all(x in cpp for x in ['GetWheelState', 'bInContact', 'NormalizedSuspensionLength', 'GetVelocity().SizeSquared2D()']),
    'wanted-4 escalation': 'AddHeat(130.f)' in cpp and 'GetWantedLevel()>=4' in cpp,
    'active police response': 'GetActiveFootUnitCount()>0' in cpp,
    'active pursuit vehicle': 'GetActivePursuitVehicleCount()>0' in cpp,
    'pursuit interaction proof': all(x in cpp for x in ['PursuitStartDistance', 'Distance+250.f<PursuitStartDistance', 'PURSUIT_CLOSING']),
    'roadblock runtime proof': all(x in cpp for x in ['GetActiveRoadblockCount()>0', 'ROADBLOCK_ACTIVE', 'DEMO_SCENARIO_ROADBLOCK']),
    'physical crossing proof': all(x in cpp for x in ['DriveNativeRoadblockCrossing', 'GetSpikeStripWorldLocation', 'ROADBLOCK_PHYSICAL_CROSSING', 'HANDLING_CONSEQUENCE']),
    'post-spike escape proof': all(x in cpp for x in ['POST_SPIKE_ESCAPE', 'PostSpikeEscapeStartSeconds', 'GetRuntimeWheelRisk', 'GetRuntimeThrottleLimit', 'GetRuntimeSteeringLimit']),
    'save is executed': 'SaveProgress()' in cpp,
    'evaluator hard gates': all(s in eval_ps for s in steps) and 'gtt.demo-scenario.v8' in eval_ps and 'required_step_count=$required.Count' in eval_ps,
    'native spike runtime gate': 'ROADBLOCK_SPIKE_CONSEQUENCE vehicle=' in eval_ps and 'path=NATIVE_CHAOS' in eval_ps and 'physical_crossing_passed' in eval_ps,
    'handling and escape runtime gates': 'handling_consequence_passed=$handlingEvidence' in eval_ps and 'post_spike_escape_passed=$postSpikeEscape' in eval_ps,
    'demo gate consumes scenario': "scenario.result -ne 'PASS'" in demo,
    'workflow order': workflow_order_ok,
    'artifact retained': '\\DEMO_SCENARIO.json' in workflow,
    'version': "default: '0.0.91'" in workflow,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('Post-spike deterministic demo scenario verification failed: ' + ', '.join(failed))

print(f'Post-spike deterministic demo scenario verification passed ({len(checks)} checks).')
