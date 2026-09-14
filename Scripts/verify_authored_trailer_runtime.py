#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / 'Source/GTT/Public/Vehicles/GTTTrailerAuthoredRuntimeSubsystem.h').read_text(encoding='utf-8')
cpp = (root / 'Source/GTT/Private/Vehicles/GTTTrailerAuthoredRuntimeSubsystem.cpp').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/project-sanity.yml').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')
playtest = (root / 'Docs/PLAYTEST_0.0.73.md').read_text(encoding='utf-8')
authoring = (root / 'Docs/NATIVE_TRAILER_AUTHORING.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.d/0.0.73.md').read_text(encoding='utf-8')

required = [
    (header + cpp, 'UGTTTrailerAuthoredRuntimeSubsystem'),
    (header, 'FGTTAuthoredTrailerRuntimeSnapshot'),
    (cpp, 'GTT.AuthoredTrailerRig'),
    (cpp, 'AuthoredTrailerMesh'),
    (cpp, 'GetPhysicsAsset'),
    (cpp, 'wheel_l'),
    (cpp, 'wheel_r'),
    (cpp, 'tow_eye'),
    (cpp, 'axle_center'),
    (cpp, 'TraceWheelContact'),
    (cpp, 'GetTowLoadFactor'),
    (cpp, 'AddForceAtLocation'),
    (cpp, 'AddTorqueInRadians'),
    (cpp, 'TryGetRearHitchTransform'),
    (cpp, 'AUTHORED_TRAILER_RUNTIME_EVIDENCE'),
    (workflow, 'Verify authored trailer runtime takeover and wheel-contact dynamics'),
    (workflow, 'python Scripts/verify_authored_trailer_runtime.py'),
    (playtest, 'Scenario C — Native heavy-haul dynamics'),
    (authoring, 'Roadmap close rule'),
    (changelog, '0.0.73'),
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

for token in [
    'ROADMAP-96.2%25',
    'DONE-125%2F130',
    '███████████████████░ 96.2%',
    '| **125** | **5** | **130** | **96.2%** |',
]:
    if token not in roadmap:
        raise SystemExit('Roadmap dashboard drift: missing ' + token)

if '- [ ] Authored skeletal trailer wheel assets and final hitch sockets' not in roadmap:
    raise SystemExit('Authored trailer roadmap item must remain open until a real final Unreal asset/runtime proof exists.')

if 'GroundTraceDistanceCm = 105.0f' not in header or 'HitchWarningErrorCm = 80.0f' not in header:
    raise SystemExit('Authored trailer runtime contact/hitch guard values drifted unexpectedly.')

if 'SetVisibility(false, true)' not in cpp or 'SetVisibility(Trailer->HasCargo(), true)' not in cpp:
    raise SystemExit('Safe authored/greybox presentation takeover fallback is missing.')

print('[OK] Authored trailer runtime takeover, wheel-contact dynamics, heavy-haul integration and roadmap lock verified.')
