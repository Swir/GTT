from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / 'Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h').read_text(encoding='utf-8')
source = (root / 'Source/GTT/Private/Vehicles/GTTRoadVehicleNativePawn.cpp').read_text(encoding='utf-8')
sub_h = (root / 'Source/GTT/Public/Vehicles/GTTNativeFleetTakeoverSubsystem.h').read_text(encoding='utf-8')
sub_cpp = (root / 'Source/GTT/Private/Vehicles/GTTNativeFleetTakeoverSubsystem.cpp').read_text(encoding='utf-8')
job_h = (root / 'Source/GTT/Public/Activities/GTTFarmJobDirector.h').read_text(encoding='utf-8')
job_cpp = (root / 'Source/GTT/Private/Activities/GTTFarmJobDirector.cpp').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')

required = {
    'shared native road pawn': 'class GTT_API AGTTRoadVehicleNativePawn' in header,
    'rattleback native pawn': 'AGTTRattlebackNativePawn' in header and 'Rattleback82' in source,
    'mulebox native pawn': 'AGTTMuleboxNativePawn' in header and 'Mulebox1200' in source,
    'canonical wheel setup': 'ConfigureCanonicalWheelSetups' in source and 'ValidateCanonicalWheelSetups' in source,
    'canonical powertrain': 'ConfigureCanonicalPowertrain' in source and 'ValidateCanonicalPowertrain' in source,
    'physics asset acceptance': 'GetPhysicsAsset() != nullptr' in source,
    'legacy state import': 'GetConditionPercent()' in source and 'GetFuelLiters()' in source and 'GetEngineUpgradeLevel()' in source and 'GetTireIntegrity()' in source,
    'correct 0..1 condition ratio': 'FMath::Clamp(LegacyVehicle->GetConditionPercent(), 0.0f, 1.0f)' in source and 'ConditionPercent / 100.0f' not in source,
    'continuous legacy mirror': 'RestorePersistentState(GetActorTransform()' in source,
    'safe runtime fallback': 'NATIVE_ROAD_FALLBACK' in source and 'DeactivateLegacyTakeover();' in source,
    'acceptance-gated boot subsystem': 'UGTTNativeFleetTakeoverSubsystem' in sub_h and 'NATIVE_FLEET_TAKEOVER_BOOT' in sub_cpp,
    'collision-safe complete fleet bootstrap': 'SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn' in sub_cpp and 'spawned=%d/3' in sub_cpp,
    'boot rattleback and mulebox': 'AGTTRattlebackNativePawn::StaticClass()' in sub_cpp and 'AGTTMuleboxNativePawn::StaticClass()' in sub_cpp,
    'native cargo job integration': 'LoadedNativeMulebox' in job_h and 'NATIVE MULEBOX LOADED' in job_cpp,
    'native cargo condition consequence': 'ResolveCargoVehicleConditionRatio' in job_cpp and 'GetMigrationSnapshot().ConditionPercent' in job_cpp,
    'native role bonus preserved': '(LoadedMulebox.IsValid() || LoadedNativeMulebox.IsValid()) ? MuleboxRoleBonus : 0' in job_cpp,
    'native cargo cleanup': 'LoadedNativeMulebox->SetCargoLoadFactor(0.0f)' in job_cpp,
}
missing = [name for name, ok in required.items() if not ok]
if missing:
    raise SystemExit('Native road fleet takeover verification failed: ' + ', '.join(missing))

checks = re.findall(r'^\s*- \[(x| )\] ', roadmap, flags=re.MULTILINE | re.IGNORECASE)
done = sum(1 for state in checks if state.lower() == 'x')
total = len(checks)
if (done, total) != (125, 130):
    raise SystemExit(f'Roadmap checkbox count changed unexpectedly: {done}/{total}')
for token in (
    '<!-- SWIR-ROADMAP-STANDARD:v1 -->', '<!-- ROADMAP-PROGRESS:START -->', '<!-- ROADMAP-PROGRESS:END -->',
    '## 📊 Overall progress', '../assets/readme/progress-mini.svg', 'DONE-125%2F130', 'ROADMAP-96.2%25',
    '| **125** | **5** | **130** | **96.2%** |'):
    if token not in roadmap:
        raise SystemExit('Roadmap SVG-only dashboard is stale or inconsistent: ' + token)
if roadmap.count('../assets/readme/progress-mini.svg') != 1:
    raise SystemExit('Roadmap must embed exactly one canonical progress-mini.svg')
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    raise SystemExit('Legacy text/Unicode roadmap progress meter must not return')

print('Native road fleet takeover foundation verified; ROADMAP remains 125/130 (96.2%) with SVG-only progress.')
