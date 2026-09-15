from pathlib import Path

root = Path(__file__).resolve().parents[1]
roadblock = (root / "Source/GTT/Private/Police/GTTRoadblock.cpp").read_text(encoding="utf-8")
native_h = (root / "Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h").read_text(encoding="utf-8")

checks = {
    "native road vehicle included by roadblock": 'Vehicles/GTTRoadVehicleNativePawn.h' in roadblock,
    "roadblock recognizes Native Chaos pawn": 'Cast<AGTTRoadVehicleNativePawn>' in roadblock,
    "shared spike contract exposed": 'ApplyPoliceSpikeDamage' in native_h,
    "native tire integrity is reduced": 'MigrationSnapshot.TireIntegrity -' in native_h,
    "native condition is reduced": 'MigrationSnapshot.ConditionPercent -' in native_h,
    "damage syncs to legacy/save mirror": 'SyncLegacyMirror();' in native_h,
    "roadblock invokes native spike damage": 'NativeVehicle->ApplyPoliceSpikeDamage' in roadblock,
    "telemetry distinguishes native path": 'NATIVE_CHAOS' in roadblock,
    "legacy path remains supported": 'Cast<AGTTVehicleBase>' in roadblock and 'LEGACY' in roadblock,
    "repeat-hit cooldown preserved": 'SpikeRepeatCooldownSeconds' in roadblock,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Native spike parity verification failed: " + ", ".join(failed))
print(f"Native spike parity verified ({len(checks)}/{len(checks)} checks).")
