#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / 'Source/GTT/Public/Vehicles/GTTTrailerNativeAcceptanceSubsystem.h').read_text(encoding='utf-8')
cpp = (root / 'Source/GTT/Private/Vehicles/GTTTrailerNativeAcceptanceSubsystem.cpp').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/project-sanity.yml').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')
playtest = (root / 'Docs/PLAYTEST_0.0.72.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.d/0.0.72.md').read_text(encoding='utf-8')
trailer = (root / 'Source/GTT/Private/Vehicles/GTTFarmTrailer.cpp').read_text(encoding='utf-8')

required = [
    (header + cpp, 'UGTTTrailerNativeAcceptanceSubsystem'),
    (cpp, 'GTT.AuthoredTrailerRig'),
    (cpp, 'AuthoredTrailerMesh'),
    (cpp, 'MISSING_PHYSICS_ASSET'),
    (cpp, 'wheel_l'),
    (cpp, 'wheel_r'),
    (cpp, 'tow_eye'),
    (cpp, 'axle_center'),
    (cpp, 'TryGetRearHitchTransform'),
    (cpp, 'NATIVE_TRAILER_ACCEPTANCE_EVIDENCE'),
    (cpp, 'NATIVE_TRAILER_JACKKNIFE_WARNING'),
    (cpp, 'NATIVE_TRAILER_FAILSAFE'),
    (cpp, 'DetachTrailer'),
    (trailer, 'AttachToNativeFieldmaster'),
    (workflow, 'Verify Native trailer authored-rig acceptance and hitch safety'),
    (workflow, 'python Scripts/verify_native_trailer_acceptance.py'),
    (playtest, 'Scenario D — Jackknife safety'),
    (changelog, '0.0.72'),
    (roadmap, '<!-- SWIR-ROADMAP-STANDARD:v1 -->'),
    (roadmap, '📊 Overall progress'),
]
missing = [token for text, token in required if token not in text]
if missing:
    raise SystemExit('Missing required tokens: ' + ', '.join(missing))

checks = re.findall(r'^- \[(x|X| )\]', roadmap, flags=re.MULTILINE)
done = sum(1 for value in checks if value.lower() == 'x')
total = len(checks)
remaining = total - done
if (done, total, remaining) != (125, 130, 5):
    raise SystemExit(f'Roadmap checkbox drift: done={done} total={total} remaining={remaining}; expected 125/130/5')

for token in ['ROADMAP-96.2%25', 'DONE-125%2F130', '███████████████████░ 96.2%', '| **125** | **5** | **130** | **96.2%** |']:
    if token not in roadmap:
        raise SystemExit('Roadmap dashboard drift: missing ' + token)

if '- [ ] Authored skeletal trailer wheel assets and final hitch sockets' not in roadmap:
    raise SystemExit('Trailer authored-assets checkbox must remain open without real UE-authored asset/runtime evidence.')

if 'JackknifeDetachYawDeg = 76.0f' not in header or 'InvalidGraceSeconds = 0.75f' not in header:
    raise SystemExit('Native trailer jackknife/fail-safe thresholds drifted unexpectedly.')

print('[OK] Native trailer authored-rig contract, hitch safety, fail-safe evidence and roadmap lock verified.')
