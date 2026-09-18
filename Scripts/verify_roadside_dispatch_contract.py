#!/usr/bin/env python3
"""Source-level verifier for GTT 0.1.36 roadside dispatch authority.
Does not claim an Unreal compile/package/runtime test.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
h = (ROOT / 'Source/GTT/Public/Vehicles/GTTRoadsideRecoverySubsystem.h').read_text(encoding='utf-8')
c = (ROOT / 'Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp').read_text(encoding='utf-8')
r = (ROOT / 'Docs/ROADMAP.md').read_text(encoding='utf-8')
readme = (ROOT / 'README.md').read_text(encoding='utf-8')
errors = []


def req(text: str, needle: str, label: str) -> None:
    if needle not in text:
        errors.append(f'missing {label}: {needle}')


for n, l in [
    ('PendingTowQuote', 'tow quote state'),
    ('PendingPatchQuote', 'patch quote state'),
    ('PendingPersistentVehicleId', 'pinned vehicle id'),
    ('CancelPendingRoadsideService', 'cancel API'),
    ('GetPendingRecoveryQuote', 'quote query'),
    ('GetPendingRecoverySecondsRemaining', 'countdown query'),
    ('GetPendingRecoveryVehicleId', 'target-id query'),
]:
    req(h, n, l)

for n, l in [
    ('Runtime.PendingTowQuote = TowQuote', 'tow quote lock'),
    ('Runtime.PendingPatchQuote = PatchQuote', 'patch quote lock'),
    ('Runtime.PendingPersistentVehicleId = Vehicle->GetPersistentVehicleId()', 'target pin'),
    ('quote_locked=YES target_pinned=YES', 'dispatch evidence'),
    ('NATIVE_ROADSIDE_DISPATCH_CANCELLED', 'cancel evidence'),
    ('NATIVE_ROADSIDE_DISPATCH_CONFLICT', 'cross-service guard'),
    ('NATIVE_ROADSIDE_DISPATCH_TARGET_MISMATCH', 'identity mismatch'),
    ('IsRoadsidePatchPending(NativeVehicle)', 'repeat-Y cancel'),
    ('IsRoadsideTowPending(NativeVehicle)', 'repeat-T cancel'),
]:
    req(c, n, l)

# Completion must consume request-time quote and reject wrong vehicle before mutation.
tow_start = c.find('bool UGTTRoadsideRecoverySubsystem::CompleteRecovery(')
if tow_start < 0:
    errors.append('tow completion body missing')
    tow = ''
else:
    tow = c[tow_start:]
req(tow, ': LockedTowQuote;', 'locked tow quote consumption')
if tow and not (0 <= tow.find('SpendCash(Cost') < tow.find('Vehicle->ExitNativeVehicle()')):
    errors.append('tow must charge locked quote before move')

patch_start = c.find('bool UGTTRoadsideRecoverySubsystem::CompleteEmergencyPatch(')
patch_end = c.find('bool UGTTRoadsideRecoverySubsystem::CompleteRecovery(')
if patch_start < 0 or patch_end <= patch_start:
    errors.append('patch completion body missing')
    patch = ''
else:
    patch = c[patch_start:patch_end]
if patch and not (
    0 <= patch.find('Vehicle->GetPersistentVehicleId() != ExpectedVehicleId')
    < patch.find('SpendCash(PatchQuote')
):
    errors.append('patch id mismatch must be rejected before charge')

req(c, 'Police impound remains an automatic, non-cancellable consequence', 'police impound isolation')
reset = re.search(r'void UGTTRoadsideRecoverySubsystem::ResetPendingService\([^)]*\)\s*\{(.*?)\n\}', c, re.S)
if not reset:
    errors.append('reset body missing')
else:
    for n in [
        'PendingTowQuote = 0',
        'PendingPatchQuote = 0',
        'PendingPersistentVehicleId = NAME_None',
        'bTowRequested = false',
        'bPatchRequested = false',
    ]:
        req(reset.group(1), n, 'reset state')

# Protected documentation/progress invariants: SVG-only presentation, checklist-derived numbers.
for n, l in [
    ('<!-- SWIR-ROADMAP-STANDARD:v1 -->', 'roadmap marker'),
    ('<!-- ROADMAP-PROGRESS:START -->', 'roadmap progress start'),
    ('<!-- ROADMAP-PROGRESS:END -->', 'roadmap progress end'),
    ('## 📊 Overall progress', 'roadmap progress heading'),
    ('../assets/readme/progress-mini.svg', 'roadmap mini SVG'),
    ('| **125** | **5** | **130** | **96.2%** |', 'roadmap numeric table'),
]:
    req(r, n, l)

checks = re.findall(r'^\s*-\s*\[(x| )\]\s+', r, flags=re.MULTILINE | re.IGNORECASE)
done = sum(item.lower() == 'x' for item in checks)
if (done, len(checks)) != (125, 130):
    errors.append(f'roadmap checklist truth changed unexpectedly: {done}/{len(checks)}')

legacy_meter = re.compile(r'^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}[^\n]*%?', re.MULTILINE)
if legacy_meter.search(r):
    errors.append('legacy text progress meter must not exist in active ROADMAP dashboard')
if legacy_meter.search(readme):
    errors.append('legacy text progress meter must not exist in maintained README')

req(readme, '<!-- SWIR-README-STANDARD:v2 -->', 'README v2 marker')
req(readme, '## 🔎 Search Keywords', 'Search Keywords')
req(readme, 'assets/readme/progress-card.svg', 'README progress card')
req(readme, 'Release readiness: **NOT READY**', 'separate release readiness')

for p in [
    'assets/readme/progress-card.svg',
    'assets/readme/progress-mini.svg',
    'assets/readme/progress-template.svg',
    'Scripts/generate_progress_svg.py',
]:
    if not (ROOT / p).exists():
        errors.append(f'missing {p}')

if errors:
    print('GTT 0.1.36 roadside dispatch contract: FAIL')
    for error in errors:
        print(' - ' + error)
    sys.exit(1)

print('GTT 0.1.36 roadside dispatch contract: PASS')
print('Locked quotes, exact target identity, cancellation/conflict guards, police isolation and SVG-only SWIR documentation invariants verified.')
print('NOTE: source verifier only; no UE compile/package/runtime claim.')
