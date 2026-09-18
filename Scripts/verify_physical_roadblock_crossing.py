from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
h=(root/'Source/GTT/Public/Core/GTTDemoSmokeScenarioSubsystem.h').read_text(encoding='utf-8')
cpp=(root/'Source/GTT/Private/Core/GTTDemoSmokeScenarioSubsystem.cpp').read_text(encoding='utf-8')
road_h=(root/'Source/GTT/Public/Police/GTTRoadblock.h').read_text(encoding='utf-8')
eval_ps=(root/'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
workflow=(root/'.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
playtest=(root/'Docs/PLAYTEST_0.0.91.md').read_text(encoding='utf-8')
changelog=(root/'CHANGELOG.d/0.0.91.md').read_text(encoding='utf-8')
version_match=re.search(r"default:\s*'([0-9]+)\.([0-9]+)\.([0-9]+)'",workflow)
candidate_version=tuple(int(part) for part in version_match.groups()) if version_match else None
checks={
 'crossing state': all(x in h for x in ('DriveNativeRoadblockCrossing','RoadblockTestVehicle','RoadblockTestActor','RoadblockBaselineTires','RoadblockBaselineWheelRisk')),
 'roadblock geometry API': 'GetSpikeStripWorldLocation' in road_h and 'GetSpikeApproachDirection' in road_h,
 'physical route': all(x in cpp for x in ('DriveNativeRoadblockCrossing','GetSpikeStripWorldLocation','GetSpikeApproachDirection','SetActorLocation','SetActorRotation','SetThrottleInput')),
 'no direct damage shortcut': 'ApplyPoliceSpikeDamage' not in cpp,
 'crossing evidence': 'DEMO_SCENARIO_ROADBLOCK_CROSSING' in cpp and 'ROADBLOCK_PHYSICAL_CROSSING' in cpp,
 'handling evidence': 'DEMO_SCENARIO_HANDLING_CONSEQUENCE' in cpp and 'HANDLING_CONSEQUENCE' in cpp and 'GetRuntimeWheelRisk' in cpp,
 'post-spike evidence retained': 'POST_SPIKE_ESCAPE' in cpp,
 '26 core completion retained': 'steps=26' in cpp and 'DEMO_SCENARIO_COMPLETE result=PASS steps=26' in eval_ps,
 'schema v11 retains crossing': 'gtt.demo-scenario.v11' in eval_ps and 'physical_crossing_passed' in eval_ps and 'wheel_risk_after' in eval_ps and 'post_spike_escape_passed' in eval_ps,
 'candidate version not regressed below 0.1.14': candidate_version is not None and candidate_version >= (0,1,14) and 'RUNTIME_SMOKE.json' in workflow and 'DEMO_TECHNICAL_GATE.json' in workflow,
 'origin docs retained': '0.0.91' in playtest and '0.0.91' in changelog,
}
failed=[name for name,ok in checks.items() if not ok]
if failed: raise SystemExit('Physical roadblock crossing verification failed: '+', '.join(failed))
label='.'.join(str(part) for part in candidate_version)
print(f'Physical Native roadblock crossing contract retained under additive candidate {label} ({len(checks)}/{len(checks)} checks).')
