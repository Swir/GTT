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
    (header + cpp, 'UGTTRoadsideRecoverySubsystem'),
    (header + cpp, 'EGTTRoadsideRecoveryMode'),
    (cpp, 'IsRecoveryEligible'),
    (cpp, 'CalculateRoadsideCost'),
    (cpp, 'CalculateImpoundCost'),
    (cpp, 'ApplyNativeWorkshopService'),
    (cpp, 'SpendCash'),
    (cpp, 'ChargeFine'),
    (cpp, 'ClearWanted'),
    (cpp, 'NATIVE_ROADSIDE_RECOVERY_ARMED'),
    (cpp, 'NATIVE_ROADSIDE_RECOVERY_BLOCKED'),
    (cpp, 'NATIVE_ROADSIDE_RECOVERY_DENIED'),
    (cpp, 'NATIVE_ROADSIDE_RECOVERY_COMPLETE'),
    (cpp, 'NATIVE_POLICE_IMPOUND_ARMED'),
    (cpp, 'NATIVE_POLICE_IMPOUND'),
    (road_h, 'GetMigrationSnapshot'),
    (road_h, 'GetBodyDamageRepairSurcharge'),
    (workflow, 'Verify Native roadside recovery and police impound'),
    (workflow, 'python Scripts/verify_native_roadside_recovery.py'),
    (playtest, 'Scenario D — Police impound'),
    (playtest, 'NATIVE_ROADSIDE_RECOVERY_DENIED'),
    (changelog, '0.0.71'),
    (roadmap, '<!-- SWIR-ROADMAP-STANDARD:v1 -->'),
    (roadmap, '📊 Overall progress'),
]
missing = [token for text, token in required if token not in text]
if missing:
    raise SystemExit('Missing required tokens: ' + ', '.join(missing))

# Guard the gameplay contract: only genuine stranded states can arm recovery.
for token in ['ConditionPercent <= 0.05f', 'FuelLiters <= 0.05f', 'TireIntegrity <= 0.08f', 'MaxRecoverySpeedKmh']:
    if token not in cpp:
        raise SystemExit('Recovery eligibility drift: missing ' + token)

if cpp.find('SpendCash(Cost') > cpp.find('ApplyNativeWorkshopService()'):
    raise SystemExit('Roadside recovery must charge before workshop restoration.')
if cpp.find('ChargeFine(Cost') > cpp.find('ApplyNativeWorkshopService()'):
    raise SystemExit('Police impound must charge before workshop restoration.')
if 'WantedLevel == 1' not in cpp or 'WantedLevel >= 2' not in cpp:
    raise SystemExit('Wanted-aware roadside block / police impound thresholds are missing.')

# Count only actual roadmap checklist task lines; do not count the explanatory example.
checks = re.findall(r'^- \[(x|X| )\]', roadmap, flags=re.MULTILINE)
done = sum(1 for value in checks if value.lower() == 'x')
total = len(checks)
remaining = total - done
if (done, total, remaining) != (125, 130, 5):
    raise SystemExit(f'Roadmap checkbox drift: done={done} total={total} remaining={remaining}; expected 125/130/5')

expected_dashboard_tokens = [
    'ROADMAP-96.2%25',
    'DONE-125%2F130',
    '███████████████████░ 96.2%',
    '| **125** | **5** | **130** | **96.2%** |',
]
missing_dashboard = [token for token in expected_dashboard_tokens if token not in roadmap]
if missing_dashboard:
    raise SystemExit('Roadmap dashboard drift: missing ' + ', '.join(missing_dashboard))

print('[OK] Native roadside recovery, wanted-aware impound, economy/workshop integration and roadmap lock verified.')
