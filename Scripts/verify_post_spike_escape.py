from pathlib import Path
root=Path(__file__).resolve().parents[1]
scenario=(root/'Source/GTT/Private/Core/GTTDemoSmokeScenarioSubsystem.cpp').read_text(encoding='utf-8')
header=(root/'Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h').read_text(encoding='utf-8')
eval_ps=(root/'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
workflow=(root/'.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
playtest=(root/'Docs/PLAYTEST_0.0.91.md').read_text(encoding='utf-8')
changelog=(root/'CHANGELOG.d/0.0.91.md').read_text(encoding='utf-8')
checks={
 'runtime limits exposed': all(x in header for x in ('GetRuntimeThrottleLimit','GetRuntimeSteeringLimit','GetRuntimeBrakeAssist')),
 'scenario v8': 'DEMO_SCENARIO_BEGIN version=8 mode=post-spike-escape-dynamics' in scenario,
 'handling authority proof': all(x in scenario for x in ('throttle_limit_before','throttle_limit_after','steering_limit_before','steering_limit_after')),
 'continued damaged driving': 'POST_SPIKE_ESCAPE' in scenario and 'FMath::Sin(Phase*2.2f)*0.65f' in scenario and 'Phase>=3.f' in scenario,
 '26 step completion': 'DEMO_SCENARIO_COMPLETE result=PASS steps=26' in scenario,
 'evaluator schema': 'gtt.demo-scenario.v8' in eval_ps and "'POST_SPIKE_ESCAPE'" in eval_ps,
 'evaluator hard gates control authority': 'Native handling consequence did not prove reduced control authority' in eval_ps,
 'evaluator hard gates continued motion': 'Native vehicle did not continue a measurable damaged escape' in eval_ps,
 'win64 0.0.91 route': "default: '0.0.91'" in workflow and 'post-spike escape dynamics' in workflow,
 'docs': 'Post-Spike Escape Dynamics' in changelog and 'Post-Spike Escape Dynamics' in playtest,
}
failed=[name for name,ok in checks.items() if not ok]
if failed: raise SystemExit('Post-spike escape verification failed: '+', '.join(failed))
print(f'Post-spike escape dynamics verified ({len(checks)}/{len(checks)} checks).')
