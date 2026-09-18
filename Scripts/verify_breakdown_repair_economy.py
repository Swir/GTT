#!/usr/bin/env python3
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding='utf-8')
sub_h=read('Source/GTT/Public/Vehicles/GTTBreakdownDecisionSubsystem.h'); sub_cpp=read('Source/GTT/Private/Vehicles/GTTBreakdownDecisionSubsystem.cpp')
service_h=read('Source/GTT/Public/World/GTTServiceTerminal.h'); service_cpp=read('Source/GTT/Private/World/GTTServiceTerminal.cpp')
recovery_cpp=read('Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp'); statics=read('Source/GTT/Private/Core/GTTGameplayStatics.cpp')
sanity=read('.github/workflows/project-sanity.yml'); playtest=read('Docs/PLAYTEST_0.0.95.md'); changelog=read('CHANGELOG.d/0.0.95.md'); roadmap=read('Docs/ROADMAP.md')
legacy_separate_price_copy='Tow $%d; workshop estimate $%d is separate.' in recovery_cpp
choice_separate_price_copy=all(x in recovery_cpp for x in [
    'patch $%d for limp-home, or T / D-Pad Up tow $%d',
    'Full workshop repair ~$%d.',
    'Temporary limp-home service only; body damage and workshop repairs remain.',
    'workshop_repair_still_required=YES',
])
checks={
'authoritative assessment subsystem':'UGTTBreakdownDecisionSubsystem : public UWorldSubsystem' in sub_h,
'decision contract':all(x in sub_h for x in ['EGTTBreakdownRecommendation','FGTTBreakdownAssessment','RepairEstimate','TowEstimate','bCanLimpHome','bTowRecommended']),
'repair estimate real state':all(x in sub_cpp for x in ['ConditionPercent','TireIntegrity','FuelLiters','GetBodyDamageRepairSurcharge']),
'structural dynamics decision':'UGTTStructuralDriveConsequenceSubsystem' in sub_cpp and 'Structural.DamageSeverity' in sub_cpp and 'Structural.bLimpHomeActive' in sub_cpp,
'bounded repair estimate':'1500' in sub_cpp and 'MechanicalLabor' in sub_cpp and 'TireParts' in sub_cpp and 'FuelCharge' in sub_cpp,
'tow distance and damage':'DistanceMeters' in sub_cpp and 'DamageHandling' in sub_cpp and 'StructuralHandling' in sub_cpp,
'workshop quote':'GetNativeRoadRepairQuote' in service_h and 'CalculateRepairEstimate' in service_cpp,
'dynamic workshop charge':'const int32 TotalCost = GetNativeRoadRepairQuote(NativeRoad)' in service_cpp and 'SpendCash(TotalCost' in service_cpp,
'shared tow assessment':'CalculateTowEstimate' in recovery_cpp,
'tow not free repair':'NATIVE_ROADSIDE_TOW_COMPLETE' in recovery_cpp and 'serviced=NO' in recovery_cpp and 'Damage preserved' in recovery_cpp,
'tow preserves state':all(x in recovery_cpp for x in ['BeforeTow.ConditionPercent','AfterTow.ConditionPercent','BodyBeforeTow.FrontHealth','BodyAfterTow.FrontHealth','bDamagePreserved']),
'impound mandatory service':'Police impound + mandatory safety service' in recovery_cpp and 'ApplyNativeWorkshopService()' in recovery_cpp,
'separate price messaging':legacy_separate_price_copy or choice_separate_price_copy,
'native driver services':'AGTTRoadVehicleNativePawn' in statics and 'NativeRoad->GetDriverPawn()' in statics,
'sanity wired':'Verify breakdown decision towing and repair economy' in sanity and 'verify_breakdown_repair_economy.py' in sanity,
'docs':'0.0.95' in playtest and 'tow' in playtest.lower() and '0.0.95' in changelog and 'damage-based' in changelog.lower(),
}
failed=[n for n,ok in checks.items() if not ok]
if failed: raise SystemExit('Breakdown/repair economy verification failed: '+', '.join(failed))
if '<!-- SWIR-ROADMAP-STANDARD:v1 -->' not in roadmap or '## 📊 Overall progress' not in roadmap: raise SystemExit('SWIR roadmap style lock missing')
items=re.findall(r'^- \[(x| )\] ',roadmap,flags=re.MULTILINE); done=sum(v=='x' for v in items); total=len(items); remaining=total-done; percent=round(done*100.0/total,1); filled=round(done*20.0/total); bar='█'*filled+'░'*(20-filled)
for token in (f'ROADMAP-{percent:.1f}%25',f'DONE-{done}%2F{total}',f'{bar} {percent:.1f}%',f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |'):
    if token not in roadmap: raise SystemExit('Roadmap dashboard drift: missing '+token)
print(f'[OK] Breakdown/tow/repair economy verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.')
