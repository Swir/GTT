#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / 'Source/GTT/Public/Vehicles/GTTRoadsideRecoverySubsystem.h').read_text(encoding='utf-8')
cpp = (root / 'Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp').read_text(encoding='utf-8')
road_h = (root / 'Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/project-sanity.yml').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')
playtest = (root / 'Docs/PLAYTEST_0.0.71.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.d/0.0.71.md').read_text(encoding='utf-8')

required = [
    (header + cpp, 'UGTTRoadsideRecoverySubsystem'), (header + cpp, 'EGTTRoadsideRecoveryMode'), (cpp, 'IsRecoveryEligible'),
    (cpp, 'CalculateRoadsideCost'), (cpp, 'CalculateImpoundCost'), (cpp, 'ApplyNativeWorkshopService'), (cpp, 'SpendCash'),
    (cpp, 'ChargeFine'), (cpp, 'ClearWanted'), (cpp, 'NATIVE_ROADSIDE_RECOVERY_ARMED'), (cpp, 'NATIVE_ROADSIDE_RECOVERY_BLOCKED'),
    (cpp, 'NATIVE_ROADSIDE_RECOVERY_DENIED'), (cpp, 'NATIVE_ROADSIDE_TOW_COMPLETE'), (cpp, 'damage_preserved='), (cpp, 'serviced=NO'),
    (cpp, 'NATIVE_POLICE_IMPOUND_ARMED'), (cpp, 'NATIVE_POLICE_IMPOUND'), (road_h, 'GetMigrationSnapshot'),
    (road_h, 'GetBodyDamageRepairSurcharge'), (workflow, 'Verify Native roadside recovery and police impound'),
    (workflow, 'python Scripts/verify_native_roadside_recovery.py'), (playtest, 'Scenario D — Police impound'),
    (playtest, 'NATIVE_ROADSIDE_RECOVERY_DENIED'), (changelog, '0.0.71'), (roadmap, '<!-- SWIR-ROADMAP-STANDARD:v1 -->'), (roadmap, '📊 Overall progress'),
]
missing = [token for text, token in required if token not in text]
if missing: raise SystemExit('Missing required tokens: ' + ', '.join(missing))
for token in ['ConditionPercent <= 0.05f','FuelLiters <= 0.05f','TireIntegrity <= 0.08f','MaxRecoverySpeedKmh']:
    if token not in cpp: raise SystemExit('Recovery eligibility drift: missing ' + token)
if cpp.find('SpendCash(Cost') > cpp.find('ApplyNativeWorkshopService()'): raise SystemExit('Recovery payment must occur before any mandatory-service branch.')
if cpp.find('ChargeFine(Cost') > cpp.find('ApplyNativeWorkshopService()'): raise SystemExit('Police impound must charge before workshop restoration.')
if 'WantedLevel == 1' not in cpp or 'WantedLevel >= 2' not in cpp: raise SystemExit('Wanted-aware roadside block / police impound thresholds are missing.')
checks = re.findall(r'^- \[(x|X| )\]', roadmap, flags=re.MULTILINE); done=sum(1 for value in checks if value.lower()=='x'); total=len(checks); remaining=total-done
if (done,total,remaining)!=(125,130,5): raise SystemExit(f'Roadmap checkbox drift: done={done} total={total} remaining={remaining}; expected 125/130/5')
for token in ['ROADMAP-96.2%25','DONE-125%2F130','███████████████████░ 96.2%','| **125** | **5** | **130** | **96.2%** |']:
    if token not in roadmap: raise SystemExit('Roadmap dashboard drift: missing ' + token)
print('[OK] Native roadside tow, damage preservation, wanted-aware impound, economy/workshop separation and roadmap lock verified.')
