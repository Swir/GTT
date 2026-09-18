#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / 'Source/GTT/Public/Vehicles/GTTNativeRolloverSafetySubsystem.h').read_text(encoding='utf-8')
cpp = (root / 'Source/GTT/Private/Vehicles/GTTNativeRolloverSafetySubsystem.cpp').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/project-sanity.yml').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')
playtest = (root / 'Docs/PLAYTEST_0.0.74.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.d/0.0.74.md').read_text(encoding='utf-8')

required = [
    (header + cpp, 'UGTTNativeRolloverSafetySubsystem'),
    (header, 'FGTTNativeRolloverSnapshot'),
    (header, 'TippedAccumulator'),
    (cpp, 'AGTTFieldmasterNativePawn'),
    (cpp, 'AGTTRoadVehicleNativePawn'),
    (cpp, 'GetWheelState'),
    (cpp, 'bIsValid && WheelState.bInContact'),
    (cpp, 'Snapshot.WheelContacts >= 2'),
    (cpp, 'AddTorqueInRadians'),
    (cpp, 'AddForce'),
    (cpp, 'Migration.TireIntegrity'),
    (cpp, 'NATIVE_FLEET_ROLLOVER_EVIDENCE'),
    (cpp, 'NATIVE_FLEET_EMERGENCY_RIGHTING'),
    (workflow, 'Verify Native fleet rollover safety and emergency righting'),
    (workflow, 'python Scripts/verify_native_rollover_safety.py'),
    (playtest, 'Scenario D — Genuine rollover recovery'),
    (changelog, '0.0.74'),
    (roadmap, '<!-- SWIR-ROADMAP-STANDARD:v1 -->'),
    (roadmap, '<!-- ROADMAP-PROGRESS:START -->'),
    (roadmap, '<!-- ROADMAP-PROGRESS:END -->'),
    (roadmap, '📊 Overall progress'),
    (roadmap, '../assets/readme/progress-mini.svg'),
]
missing = [token for text, token in required if token not in text]
if missing:
    raise SystemExit('Missing required tokens: ' + ', '.join(missing))

if 'if (!bTakeoverActive)' not in cpp or 'bDriverPresent' not in cpp:
    raise SystemExit('Rollover safety must be gated by active Native takeover and driver state.')
if 'Snapshot.WheelContacts >= 2' not in cpp:
    raise SystemExit('Anti-roll must require grounded wheel contacts; airborne auto-leveling is forbidden.')
if 'EmergencyCooldownSeconds = 8.0f' not in header or 'EmergencyArmSeconds = 2.25f' not in header:
    raise SystemExit('Emergency righting anti-exploit timings drifted unexpectedly.')
if 'TippedAccumulator = FMath::Min' not in cpp:
    raise SystemExit('Persistent rollover arm timer is missing.')

checks = re.findall(r'^- \[(x|X| )\]', roadmap, flags=re.MULTILINE)
done = sum(1 for value in checks if value.lower() == 'x')
total = len(checks)
remaining = total - done
if (done, total, remaining) != (125, 130, 5):
    raise SystemExit(f'Roadmap checkbox drift: done={done} total={total} remaining={remaining}; expected 125/130/5')

for token in [
    'ROADMAP-96.2%25',
    'DONE-125%2F130',
    '| **125** | **5** | **130** | **96.2%** |',
]:
    if token not in roadmap:
        raise SystemExit('Roadmap dashboard drift: missing ' + token)
progress_block = roadmap.split('<!-- ROADMAP-PROGRESS:START -->', 1)[1].split('<!-- ROADMAP-PROGRESS:END -->', 1)[0]
if progress_block.count('../assets/readme/progress-mini.svg') != 1:
    raise SystemExit('Roadmap progress block must embed exactly one canonical progress-mini.svg.')
if re.search(r'[█▓▒░]{3,}', progress_block):
    raise SystemExit('Legacy text/Unicode progress meter must not return to the active Roadmap dashboard.')

for open_item in [
    '- [ ] Dedicated native Chaos wheeled tractor movement',
    '- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup',
    '- [ ] Authored skeletal trailer wheel assets and final hitch sockets',
    '- [ ] Full Unreal compile + packaged Win64 smoke test',
    '- [ ] Full Win64 CI/build runner',
]:
    if open_item not in roadmap:
        raise SystemExit('Runtime/build roadmap item was closed without required evidence: ' + open_item)

print('[OK] Native fleet rollover safety, grounded anti-roll, emergency righting and SVG-only roadmap lock verified.')
