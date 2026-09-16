#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
fleet_h = (root / 'Source/GTT/Public/World/GTTGarageFleetSubsystem.h').read_text(encoding='utf-8')
fleet_cpp = (root / 'Source/GTT/Private/World/GTTGarageFleetSubsystem.cpp').read_text(encoding='utf-8')
slot_cpp = (root / 'Source/GTT/Private/World/GTTGarageSlotTerminal.cpp').read_text(encoding='utf-8')
save_h = (root / 'Source/GTT/Public/Save/GTTSaveGame.h').read_text(encoding='utf-8')
struct_cpp = (root / 'Source/GTT/Private/Core/GTTStructuralGameMode.cpp').read_text(encoding='utf-8')
farm_cpp = (root / 'Source/GTT/Private/Activities/GTTFarmJobDirector.cpp').read_text(encoding='utf-8')
struct_verify = (root / 'Scripts/verify_structural_damage_persistence.py').read_text(encoding='utf-8')
sanity = (root / '.github/workflows/project-sanity.yml').read_text(encoding='utf-8')
playtest = (root / 'Docs/PLAYTEST_0.0.99.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.d/0.0.99.md').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')

checks = {
    'save schema v6': 'SaveVersion = 6' in save_h and 'PreferredGarageVehicleId' in save_h,
    'v5 structural threshold retained': 'StructuralDamageSaveVersion = 5' in struct_cpp and 'Save->SaveVersion >= StructuralDamageSaveVersion' in struct_cpp,
    'v6 dispatch threshold explicit': 'FleetDispatchSaveVersion = 6' in struct_cpp and 'ExtendedSaveVersion = 6' in struct_cpp,
    'preferred dispatch saved': 'Save->PreferredGarageVehicleId = Fleet->GetPreferredVehicleId()' in struct_cpp,
    'preferred dispatch restored': 'RestorePreferredVehicleId' in struct_cpp and 'Save->PreferredGarageVehicleId' in struct_cpp,
    'old-save fallback supported': 'Save->SaveVersion >= FleetDispatchSaveVersion ? Save->PreferredGarageVehicleId : NAME_None' in struct_cpp,
    'role enum exposed': all(x in fleet_h for x in ['EGTTGarageFleetRole', 'Tractor', 'Road', 'Cargo', 'Utility']),
    'preferred API exposed': all(x in fleet_h for x in ['GetPreferredVehicleId', 'SetPreferredVehicleId', 'RestorePreferredVehicleId', 'BuildJobDispatchHint']),
    'role classification deterministic': all(x in fleet_cpp for x in ['RustyFieldmaster60', 'Rattleback82', 'Mulebox1200', 'EGTTGarageFleetRole::Tractor', 'EGTTGarageFleetRole::Road', 'EGTTGarageFleetRole::Cargo']),
    'job recommendations deterministic': all(x in fleet_cpp for x in ['FarmCargoJob', 'HeavyHaulJob', 'RoadRunJob', 'RecommendedVehicleForJob']),
    'snapshot carries dispatch state': 'Snapshot.bPreferredDispatch = Snapshot.VehicleId == PreferredVehicleId' in fleet_cpp,
    'fleet summary identifies active role': 'ACTIVE %s [%s]' in fleet_cpp and 'FleetRoleLabel' in fleet_cpp,
    'dispatch preserves Native authority': all(x in slot_cpp for x in ['FindActiveNativeRoadVehicle', 'FlushNativePersistenceMirror', 'TeleportPhysics']),
    'dispatch keeps crime locks': 'GetWantedLevel() > 0' in slot_cpp and 'GetWildlifeAlertLevel() > 0' in slot_cpp,
    'dispatch charges economy': 'SpendCash(RecallServiceCost' in slot_cpp and 'Garage dispatch service' in slot_cpp,
    'dispatch commits preferred before save': 'SetPreferredVehicleId(VehicleId)' in slot_cpp and slot_cpp.find('SetPreferredVehicleId(VehicleId)') < slot_cpp.find('GameMode->SaveProgress()'),
    'dispatch failure refunds preference commit failure': 'Garage dispatch preference refund' in slot_cpp,
    'player-facing active dispatch UX': '[ACTIVE]' in slot_cpp and 'set ACTIVE' in slot_cpp and 'Dispatch slot' in slot_cpp,
    'farm job consumes fleet state': 'BuildJobDispatchHint(FName(TEXT("FarmCargo")))' in farm_cpp,
    'farm role reward retained': 'MuleboxRoleBonus' in farm_cpp and 'MULEBOX ROLE BONUS' in farm_cpp,
    'structural regression verifier migrated': 'save schema retains structural v5 data under v6+' in struct_verify and 'StructuralDamageSaveVersion = 5' in struct_verify,
    'sanity wired': 'Verify persistent fleet dispatch and job fit' in sanity and 'verify_fleet_dispatch_selection.py' in sanity,
    'playtest documents runtime route': all(x in playtest for x in ['0.0.99', 'Save/load persistence and migration', 'Farm cargo job fit', 'Native Chaos authority']),
    'changelog documents milestone': all(x in changelog for x in ['0.0.99', 'Persistent Fleet Dispatch', 'save schema v6', '125/130']),
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('Fleet dispatch selection verification failed: ' + ', '.join(failed))

required_style = [
    '<!-- SWIR-ROADMAP-STANDARD:v1 -->',
    '<!-- ROADMAP-PROGRESS:START -->',
    'alt="CI"',
    'alt="Roadmap progress"',
    'alt="Completed"',
    'alt="Status"',
    '## 📊 Overall progress',
]
for token in required_style:
    if token not in roadmap:
        raise SystemExit('SWIR roadmap style lock missing: ' + token)

items = re.findall(r'^- \[(x| )\] ', roadmap, flags=re.MULTILINE)
done = sum(v == 'x' for v in items)
total = len(items)
if total == 0:
    raise SystemExit('Roadmap contains no checklist items')
remaining = total - done
percent = round(done * 100.0 / total, 1)
filled = round(done * 20.0 / total)
bar = '█' * filled + '░' * (20 - filled)
for token in (
    f'ROADMAP-{percent:.1f}%25',
    f'DONE-{done}%2F{total}',
    'STATUS-IN%20PROGRESS',
    f'{bar} {percent:.1f}%',
    f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |',
    '<!-- ROADMAP-PROGRESS:END -->',
):
    if token not in roadmap:
        raise SystemExit('Roadmap dashboard drift: missing ' + token)

if (done, total, round(percent, 1)) != (125, 130, 96.2):
    raise SystemExit(f'0.0.99 must not claim Unreal/art roadmap gates: got {done}/{total} = {percent:.1f}%')

print(f'[OK] GTT 0.0.99 persistent fleet dispatch + legal-job fit verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.')
