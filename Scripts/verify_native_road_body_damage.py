#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
road_h = (root / 'Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h').read_text(encoding='utf-8')
road_cpp = (root / 'Source/GTT/Private/Vehicles/GTTRoadVehicleNativePawn.cpp').read_text(encoding='utf-8')
service_cpp = (root / 'Source/GTT/Private/World/GTTServiceTerminal.cpp').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/project-sanity.yml').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')
playtest = (root / 'Docs/PLAYTEST_0.0.70.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.d/0.0.70.md').read_text(encoding='utf-8')

required = [
    (road_h + road_cpp, 'EGTTRoadDamageZone'),
    (road_h + road_cpp, 'FGTTRoadBodyDamageSnapshot'),
    (road_cpp, 'DetermineImpactZone'),
    (road_cpp, 'NATIVE_ROAD_DAMAGE_ZONE'),
    (road_cpp, 'NATIVE_ROAD_PANEL_DETACH'),
    (road_cpp, 'NATIVE_ROAD_DAMAGE_DYNAMICS'),
    (road_cpp, 'NATIVE_ROAD_WORKSHOP_RESTORE'),
    (road_cpp, 'CoolingStress'),
    (road_cpp, 'DamageThrottleLimit'),
    (road_cpp, 'DamageSteeringLimit'),
    (road_cpp, 'RearBodyRisk'),
    (road_cpp, 'GetBodyDamageRepairSurcharge'),
    (service_cpp, 'FindActiveNativeRoadVehicle'),
    (service_cpp, 'ApplyNativeWorkshopService'),
    (service_cpp, 'HasActiveNativeRoadTakeover'),
    (service_cpp, 'body surcharge'),
    (workflow, 'Verify Native road body damage and workshop integration'),
    (workflow, 'python Scripts/verify_native_road_body_damage.py'),
    (playtest, 'NATIVE_ROAD_PANEL_DETACH'),
    (playtest, 'Workshop'),
    (changelog, '0.0.70'),
    (roadmap, '<!-- SWIR-ROADMAP-STANDARD:v1 -->'),
    (roadmap, '📊 Overall progress'),
]
missing = [token for text, token in required if token not in text]
if missing:
    raise SystemExit('Missing required tokens: ' + ', '.join(missing))

checks = re.findall(r'\[(x|X| )\]', roadmap)
done = sum(1 for value in checks if value.lower() == 'x')
total = len(checks)
remaining = total - done
if (done, total, remaining) != (125, 130, 5):
    raise SystemExit(f'Roadmap checkbox drift: done={done} total={total} remaining={remaining}; expected 125/130/5')

expected_dashboard_tokens = [
    '125/130',
    '96.2%',
    '███████████████████░ 96.2%',
]
missing_dashboard = [token for token in expected_dashboard_tokens if token not in roadmap]
if missing_dashboard:
    raise SystemExit('Roadmap dashboard drift: missing ' + ', '.join(missing_dashboard))

if road_cpp.find('UpdateNativeWheelRuntime(DeltaSeconds);') > road_cpp.find('UpdateDamageConsequences(DeltaSeconds);'):
    raise SystemExit('Damage consequences must run after wheel-state refresh so both control layers compose predictably.')

if service_cpp.find('FindActiveNativeRoadVehicle') > service_cpp.find('FindNearestVehicle();'):
    raise SystemExit('Native road workshop handling must precede generic legacy vehicle servicing.')

print('[OK] Native road body zones, detachable debris, handling/cooling consequences, workshop integration and roadmap lock verified.')
