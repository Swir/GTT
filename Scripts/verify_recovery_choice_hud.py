#!/usr/bin/env python3
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
read=lambda p:(root/p).read_text(encoding='utf-8')
rec_h=read('Source/GTT/Public/Vehicles/GTTRoadsideRecoverySubsystem.h');rec_cpp=read('Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp')
hud_h=read('Source/GTT/Public/UI/GTTGameHUD.h');hud_cpp=read('Source/GTT/Private/UI/GTTGameHUD.cpp')
evid_h=read('Source/GTT/Public/Vehicles/GTTRecoveryChoiceEvidenceSubsystem.h');evid_cpp=read('Source/GTT/Private/Vehicles/GTTRecoveryChoiceEvidenceSubsystem.cpp')
struct_h=read('Source/GTT/Public/Vehicles/GTTStructuralDriveConsequenceSubsystem.h');eval_ps=read('Scripts/evaluate_recovery_choice.ps1');candidate=read('Scripts/evaluate_demo_candidate.ps1')
workflow=read('.github/workflows/win64-recovery-choice-evidence.yml');sanity=read('.github/workflows/project-sanity.yml');playtest=read('Docs/PLAYTEST_0.0.96.md');changelog=read('CHANGELOG.d/0.0.96.md');roadmap=read('Docs/ROADMAP.md')
checks={
'player request API':'RequestRoadsideTow' in rec_h and 'IsRoadsideTowPending' in rec_h,
'keyboard and controller input':'WasInputKeyJustPressed(EKeys::T)' in rec_cpp and 'EKeys::Gamepad_DPad_Up' in rec_cpp,
'normal roadside requires choice':'if (!Runtime.bTowRequested)' in rec_cpp and 'player_choice=REQUIRED' in rec_cpp and 'player_authorized=YES' in rec_cpp,
'police custody remains automatic':'WantedLevel >= 2' in rec_cpp and 'PoliceImpoundArmSeconds' in rec_cpp and 'NATIVE_POLICE_IMPOUND_ARMED' in rec_cpp,
'tow remains transport only':'NATIVE_ROADSIDE_TOW_COMPLETE' in rec_cpp and 'damage_preserved=' in rec_cpp and 'serviced=NO' in rec_cpp,
'damage HUD native support':'AGTTRoadVehicleNativePawn* NativeRoad' in hud_cpp and 'BuildNativeRoadStatus' in hud_h and 'BuildNativeRoadRecovery' in hud_h,
'HUD real quotes':all(x in hud_cpp for x in ['UGTTBreakdownDecisionSubsystem','Assessment.TowEstimate','Assessment.RepairEstimate','TOW RECOMMENDED','IMMOBILIZED','T CALL TOW']),
'HUD remains compact':'RECOVERY  |' in hud_cpp and 'DAMAGE %.0f%%' in hud_cpp and 'BODY %.0f%%' in hud_cpp,
'evidence sequencing':'IsDemoEvidenceComplete' in struct_h and 'WaitForStructuralDrive' in evid_h and 'Structural->IsDemoEvidenceComplete()' in evid_cpp,
'evidence stages real immobilization':'ApplyPoliceSpikeDamage(0.94f, 0.34f)' in evid_cpp and 'EGTTBreakdownRecommendation::Immobilized' in evid_cpp,
'no-auto-tow runtime proof':'NoAutoTowProofSeconds = 8.0f' in evid_cpp and 'auto_tow=NO' in evid_cpp and 'Recovery->IsRoadsideTowPending' in evid_cpp,
'explicit tow runtime proof':'Recovery->RequestRoadsideTow(Vehicle)' in evid_cpp and 'DEMO_SCENARIO_PLAYER_TOW' in evid_cpp and 'damage_preserved=YES serviced=NO' in evid_cpp,
'separate repair runtime proof':'Interact_Implementation(PlayerPawn)' in evid_cpp and 'RepairPaid != RepairQuote' in evid_cpp and 'DEMO_SCENARIO_SEPARATE_REPAIR' in evid_cpp,
'route complete':'DEMO_SCENARIO_RECOVERY_CHOICE_COMPLETE result=PASS' in evid_cpp,
'packaged evaluator':'gtt.recovery-choice.v1' in eval_ps and 'choice_wait_seconds' in eval_ps and 'RECOVERY_CHOICE.json' in eval_ps,
'candidate hard gate':"$build.version -eq '0.0.96'" in candidate and 'RECOVERY_CHOICE.json missing for 0.0.96.' in candidate and 'tow_preserves_damage_passed' in candidate and 'separate_repair_passed' in candidate,
'0.0.96 Win64 workflow':"default: '0.0.96'" in workflow and 'runs-on: [self-hosted, windows, x64, unreal-5.8]' in workflow and 'evaluate_recovery_choice.ps1' in workflow,
'extended packaged runtime':'-MinimumAliveSeconds 175 -LaunchTimeoutSeconds 205' in workflow and '-MinimumRuntimeSeconds 175' in workflow,
'artifact retains recovery proof':'\\RECOVERY_CHOICE.json' in workflow and 'DEMO_TECHNICAL_GATE.json' in workflow,
'sanity wired':'Verify player-selectable recovery and damage HUD' in sanity and 'verify_recovery_choice_hud.py' in sanity,
'docs':'0.0.96' in playtest and 'manual tow' in playtest.lower() and '0.0.96' in changelog and 'player-selectable' in changelog.lower(),
}
failed=[n for n,ok in checks.items() if not ok]
if failed:raise SystemExit('0.0.96 recovery-choice/HUD verification failed: '+', '.join(failed))
if '<!-- SWIR-ROADMAP-STANDARD:v1 -->' not in roadmap or '## 📊 Overall progress' not in roadmap:raise SystemExit('SWIR roadmap style lock missing')
items=re.findall(r'^- \[(x| )\] ',roadmap,flags=re.MULTILINE);done=sum(v=='x' for v in items);total=len(items);remaining=total-done;percent=round(done*100.0/total,1);filled=round(done*20.0/total);bar='█'*filled+'░'*(20-filled)
for token in (f'ROADMAP-{percent:.1f}%25',f'DONE-{done}%2F{total}',f'{bar} {percent:.1f}%',f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |'):
    if token not in roadmap:raise SystemExit('Roadmap dashboard drift: missing '+token)
if (done,total)!=(125,130):raise SystemExit(f'Roadmap checkbox drift: {done}/{total}')
print(f'[OK] 0.0.96 player-selectable recovery, compact Native damage HUD and packaged evidence verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.')