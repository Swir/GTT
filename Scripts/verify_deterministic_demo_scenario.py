from pathlib import Path
root=Path(__file__).resolve().parents[1]
h=(root/'Source/GTT/Public/Core/GTTDemoSmokeScenarioSubsystem.h').read_text(encoding='utf-8')
cpp=(root/'Source/GTT/Private/Core/GTTDemoSmokeScenarioSubsystem.cpp').read_text(encoding='utf-8')
smoke=(root/'Scripts/smoke_test_windows.ps1').read_text(encoding='utf-8')
eval_ps=(root/'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
demo=(root/'Scripts/evaluate_demo_candidate.ps1').read_text(encoding='utf-8')
workflow=(root/'.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
steps=['WORLD','HUD','TRAFFIC','NPC','MISSION','COMBAT','FIELDMASTER','FIELDMASTER_MOTION','RATTLEBACK','RATTLEBACK_MOTION','MULEBOX','MULEBOX_MOTION','WANTED_COMPONENT','WANTED_ESCALATION','POLICE_RESPONSE','PURSUIT_ACTIVE','SAVE']
checks={
'world subsystem':'UTickableWorldSubsystem' in h,
'opt-in commandline':'GTTDemoSmokeScenario' in cpp and 'GTTDemoSmokeScenario' in smoke,
'explicit 17-step markers':all(f'TEXT("{s}")' in cpp for s in steps) and 'steps=17' in cpp,
'native fleet readiness':'IsNativeFieldmasterReady()' in cpp and cpp.count('IsNativeReady()') >= 2 and 'IsLegacyTakeoverActive()' in cpp,
'live wheel motion evidence':'GetWheelState' in cpp and 'bInContact' in cpp and 'NormalizedSuspensionLength' in cpp and 'GetVelocity().SizeSquared2D()' in cpp,
'deterministic wanted-3 escalation':'AddHeat(80.f)' in cpp and 'GetWantedLevel() >= 3' in cpp,
'active police response':'GetActiveFootUnitCount() > 0' in cpp,
'active pursuit vehicle':'GetActivePursuitVehicleCount() > 0' in cpp and 'PURSUIT_ACTIVE' in cpp,
'combat evidence':'UGTTCombatComponent' in cpp and 'COMBAT' in cpp,
'save is executed':'SaveProgress()' in cpp,
'evaluator hard gates':all(s in eval_ps for s in steps) and "gtt.demo-scenario.v3" in eval_ps,
'demo gate consumes scenario':"scenario.result -ne 'PASS'" in demo,
'workflow order':workflow.index('Evaluate deterministic native motion and pursuit gameplay scenario') < workflow.index('Evaluate packaged gameplay smoke'),
'artifact retained':'\\DEMO_SCENARIO.json' in workflow,
'version':'0.0.84' in workflow,
}
failed=[k for k,v in checks.items() if not v]
if failed: raise SystemExit('Native-motion/pursuit demo scenario verification failed: '+', '.join(failed))
print(f'Native-motion/pursuit demo scenario verification passed ({len(checks)} checks).')
