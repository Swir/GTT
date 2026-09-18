#!/usr/bin/env python3
"""Source-level verifier for GTT 0.1.36 roadside dispatch authority.
Does not claim an Unreal compile/package/runtime test.
"""
from pathlib import Path
import re, sys
ROOT = Path(__file__).resolve().parents[1]
h = (ROOT/'Source/GTT/Public/Vehicles/GTTRoadsideRecoverySubsystem.h').read_text(encoding='utf-8')
c = (ROOT/'Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp').read_text(encoding='utf-8')
r = (ROOT/'Docs/ROADMAP.md').read_text(encoding='utf-8')
readme = (ROOT/'README.md').read_text(encoding='utf-8')
errors=[]
def req(text, needle, label):
    if needle not in text: errors.append(f'missing {label}: {needle}')
for n,l in [
('PendingTowQuote','tow quote state'),('PendingPatchQuote','patch quote state'),
('PendingPersistentVehicleId','pinned vehicle id'),('CancelPendingRoadsideService','cancel API'),
('GetPendingRecoveryQuote','quote query'),('GetPendingRecoverySecondsRemaining','countdown query'),
('GetPendingRecoveryVehicleId','target-id query')]: req(h,n,l)
for n,l in [
('Runtime.PendingTowQuote = TowQuote','tow quote lock'),('Runtime.PendingPatchQuote = PatchQuote','patch quote lock'),
('Runtime.PendingPersistentVehicleId = Vehicle->GetPersistentVehicleId()','target pin'),
('quote_locked=YES target_pinned=YES','dispatch evidence'),('NATIVE_ROADSIDE_DISPATCH_CANCELLED','cancel evidence'),
('NATIVE_ROADSIDE_DISPATCH_CONFLICT','cross-service guard'),('NATIVE_ROADSIDE_DISPATCH_TARGET_MISMATCH','identity mismatch'),
('IsRoadsidePatchPending(NativeVehicle)','repeat-Y cancel'),('IsRoadsideTowPending(NativeVehicle)','repeat-T cancel')]: req(c,n,l)
# Completion must consume request-time quote and reject wrong vehicle before mutation.
tow = c[c.find('bool UGTTRoadsideRecoverySubsystem::CompleteRecovery('):]
req(tow, ': LockedTowQuote;', 'locked tow quote consumption')
if not (0 <= tow.find('SpendCash(Cost') < tow.find('Vehicle->ExitNativeVehicle()')): errors.append('tow must charge locked quote before move')
patch = c[c.find('bool UGTTRoadsideRecoverySubsystem::CompleteEmergencyPatch('):c.find('bool UGTTRoadsideRecoverySubsystem::CompleteRecovery(')]
if not (0 <= patch.find('Vehicle->GetPersistentVehicleId() != ExpectedVehicleId') < patch.find('SpendCash(PatchQuote'))): errors.append('patch id mismatch must be rejected before charge')
req(c,'Police impound remains an automatic, non-cancellable consequence','police impound isolation')
reset = re.search(r'void UGTTRoadsideRecoverySubsystem::ResetPendingService\([^)]*\)\s*\{(.*?)\n\}',c,re.S)
if not reset: errors.append('reset body missing')
else:
    for n in ['PendingTowQuote = 0','PendingPatchQuote = 0','PendingPersistentVehicleId = NAME_None','bTowRequested = false','bPatchRequested = false']: req(reset.group(1),n,'reset state')
# Protected documentation and progress invariants.
for n,l in [('<!-- SWIR-ROADMAP-STANDARD:v1 -->','roadmap marker'),('96.2%','roadmap percentage'),('███████████████████░ 96.2%','roadmap bar')]: req(r,n,l)
req(readme,'<!-- SWIR-README-STANDARD:v2 -->','README v2 marker'); req(readme,'## 🔎 Search Keywords','Search Keywords')
for p in ['assets/readme/progress-card.svg','assets/readme/progress-mini.svg','assets/readme/progress-template.svg','Scripts/generate_progress_svg.py']:
    if not (ROOT/p).exists(): errors.append(f'missing {p}')
if errors:
    print('GTT 0.1.36 roadside dispatch contract: FAIL'); [print(' - '+e) for e in errors]; sys.exit(1)
print('GTT 0.1.36 roadside dispatch contract: PASS')
print('Locked quotes, exact target identity, cancellation/conflict guards, police isolation and SWIR documentation invariants verified.')
print('NOTE: source verifier only; no UE compile/package/runtime claim.')
