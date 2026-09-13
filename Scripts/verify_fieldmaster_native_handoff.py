from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
input_ini = (ROOT / "Config/DefaultInput.ini").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.44.md").read_text(encoding="utf-8")

for token in [
    "FGTTVehicleMigrationSnapshot",
    "ImportLegacyGameplayState",
    "ApplyMigrationSnapshot",
    "GetMigrationSnapshot",
    "SetupPlayerInputComponent",
    "GetPersistentVehicleId",
]:
    assert token in header, f"Fieldmaster native handoff header missing {token}"

for token in [
    'BindAxis(TEXT("VehicleThrottle")',
    'BindAxis(TEXT("VehicleSteer")',
    "SetThrottleInput",
    "SetSteeringInput",
    "if (!bNativeReady)",
    "GetConditionPercent()",
    "GetFuelLiters()",
    "IsOwnedByPlayer()",
    "GetEngineUpgradeLevel()",
    "GetTireUpgradeLevel()",
    "GetTireIntegrity()",
    "Persistent ID mismatch",
]:
    assert token in cpp, f"Fieldmaster native handoff implementation missing {token}"

assert 'AxisName="VehicleThrottle"' in input_ini
assert 'AxisName="VehicleSteer"' in input_ini
assert "125" in roadmap and "130" in roadmap and "96.2%" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap

for token in [
    "legacy gameplay state",
    "throttle",
    "steering",
    "save/load",
    "fallback",
    "Win64",
]:
    assert token.lower() in playtest.lower(), f"0.0.44 playtest missing {token}"

print("Fieldmaster native gameplay handoff sanity passed")
