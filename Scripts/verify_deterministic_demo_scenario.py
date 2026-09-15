from pathlib import Path
root=Path(__file__).resolve().parents[1]
h=(root/'Source/GTT/Public/Core/GTTDemoSmokeScenarioSubsystem.h').read_text(encoding='utf-8')
cpp=(root/'Source/GTT/Private/Core/GTTDemoSmokeScenarioSubsystem.cpp').read_text(encoding='utf-8')
smoke=(root/'Scripts/smoke_test_windows.ps1').read_text(encoding='utf-8')
eval_ps=(root/'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
demo=(root/'Scripts/evaluate_demo_candidate.ps1').read_text(encoding='utf-8')
workflow=(root/'.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
steps=['WORLD','HUD','TRAFFIC','NPC','MISSION','COMBAT','FIELDMASTER','RATTLEBACK','MULEBOX','WANTED_COMPONENT','WANTED_ESCALATION','POLICE_RESPONSE','SAVE']
checks={
'world subsystem':'UTickableWorldSubsystem' in h,
'opt-in commandline':'GTTDemoSmokeScenario' in cpp and 'GTTDemoSmokeScenario' in smoke,
'explicit 13-step markers':all(f'TEXT("{s}")' in cpp for s in steps) and 'steps=13' in cpp,
'native fleet readiness':'IsNativeFieldmasterReady()' in cpp and cpp.count('IsNativeReady()') >= 2 and 'IsLegacyTakeoverActive()' in cpp,
'deterministic crime escalation':'AddHeat(55.0f)' in cpp and 'WANTED_ESCALATION' in cpp,
'police response evidence':'AGTTPoliceDirector' in cpp and 'POLICE_RESPONSE' in cpp,
'combat evidence':'UGTTCombatComponent' in cpp and 'COMBAT' in cpp,
'save is executed':'SaveProgress()' in cpp,
'evaluator hard gates':all(s in eval_ps for s in steps) and "gtt.demo-scenario.v2" in eval_ps,
'demo gate consumes scenario':"scenario.result -ne 'PASS'" in demo,
'workflow order':workflow.index('Evaluate deterministic vehicle and crime gameplay scenario') < workflow.index('Evaluate packaged gameplay smoke'),
'artifact retained':'\\DEMO_SCENARIO.json' in workflow,
'version':'0.0.83' in workflow,
}
failed=[k for k,v in checks.items() if not v]
if failed: raise SystemExit('Vehicle/crime demo scenario verification failed: '+', '.join(failed))
print(f'Vehicle/crime demo scenario verification passed ({len(checks)} checks).')
