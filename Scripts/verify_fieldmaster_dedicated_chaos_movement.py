from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
movement_h = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterChaosMovementComponent.h").read_text(encoding="utf-8")
movement_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterChaosMovementComponent.cpp").read_text(encoding="utf-8")
pawn_h = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
pawn_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.1.13.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.1.13.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")

# A dedicated native movement type must exist, not just a generic cast inside the pawn.
for token in [
    "class GTT_API UGTTFieldmasterChaosMovementComponent : public UChaosWheeledVehicleMovementComponent",
    "ConfigureAndValidateFieldmaster",
    "ApplyFieldmasterDriveCommand",
    "HoldFieldmasterStopped",
    "GetEffectiveThrottle",
    "GetEffectiveSteering",
]:
    assert token in movement_h, f"dedicated Fieldmaster movement header missing {token}"

# The component owns canonical Chaos wheels/powertrain and the real Chaos input authority.
for token in [
    "ConfigureCanonicalWheelSetups",
    "ValidateCanonicalWheelSetups",
    "ConfigureCanonicalPowertrain",
    "ValidateCanonicalPowertrain",
    "bMechanicalSimEnabled = true",
    "SetThrottleInput(EffectiveThrottle)",
    "SetSteeringInput(EffectiveSteering)",
    "SetBrakeInput",
    "SetTargetGear",
    "TireIntegrity",
    "TerrainGripFactor",
    "ConditionPercent",
]:
    assert token in movement_cpp, f"dedicated Fieldmaster movement implementation missing {token}"

# The actual tractor pawn must replace AWheeledVehiclePawn's default movement subobject.
for token in [
    "AGTTFieldmasterNativePawn(const FObjectInitializer& ObjectInitializer",
    "GetFieldmasterMovement",
    "RefreshNativeDriveCommand",
    "GetRequestedSteeringInput",
]:
    assert token in pawn_h, f"Fieldmaster pawn contract missing {token}"

for token in [
    "SetDefaultSubobjectClass<UGTTFieldmasterChaosMovementComponent>",
    "AWheeledVehiclePawn::VehicleMovementComponentName",
    "Movement->ConfigureAndValidateFieldmaster",
    "Movement->ApplyFieldmasterDriveCommand",
    "Movement->HoldFieldmasterStopped",
    "GetNativeTerrainGripFactor()",
    "MigrationSnapshot.TireIntegrity",
    "MigrationSnapshot.ConditionPercent",
]:
    assert token in pawn_cpp, f"Fieldmaster pawn is not wired to dedicated movement: {token}"

# Safety: no fuel / invalid ownership takeover must still leave the legacy mirror available.
for token in [
    "TryActivateLegacyTakeover",
    "DeactivateLegacyTakeover",
    "LegacyMirror",
    "bTakeoverActive",
    "MigrationSnapshot.FuelLiters > KINDA_SMALL_NUMBER",
]:
    assert token in pawn_cpp or token in pawn_h, f"native fallback/takeover contract missing {token}"

# Source verification is deliberately not sufficient to claim packaged runtime acceptance.
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
assert "- [ ] Full Win64 CI/build runner" in roadmap

# Enforce SWIR Roadmap Style Lock v1 and exact mathematical dashboard consistency.
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
open_items = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + open_items
assert (checked, open_items, total) == (125, 5, 130), (checked, open_items, total)
progress = round(checked * 100.0 / total, 1)
assert progress == 96.2
assert "ROADMAP-96.2%25" in roadmap
assert "DONE-125%2F130" in roadmap
assert "| **125** | **5** | **130** | **96.2%** |" in roadmap
assert "███████████████████░ 96.2%" in roadmap

assert "Dedicated Native Chaos Tractor Movement" in playtest
assert "Win64" in playtest and "packaged" in playtest.lower()
assert "GTT 0.1.13" in changelog
assert "Verify dedicated Fieldmaster Chaos movement" in workflow

print("GTT 0.1.13 dedicated Fieldmaster Chaos movement source gate passed; packaged runtime acceptance intentionally remains open")
