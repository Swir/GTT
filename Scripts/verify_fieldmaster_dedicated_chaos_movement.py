from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
movement_h = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterChaosMovementComponent.h").read_text(encoding="utf-8")
movement_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterChaosMovementComponent.cpp").read_text(encoding="utf-8")
pawn_h = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
pawn_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp").read_text(encoding="utf-8")
authority_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeDriveDynamicsSubsystem.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.1.13.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.1.13.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")

for token in [
    "class GTT_API UGTTFieldmasterChaosMovementComponent : public UChaosWheeledVehicleMovementComponent",
    "ConfigureAndValidateFieldmaster", "ApplyFieldmasterDriveCommand", "HoldFieldmasterStopped",
    "GetEffectiveThrottle", "GetEffectiveSteering",
]:
    assert token in movement_h, f"dedicated Fieldmaster movement header missing {token}"

for token in [
    "ConfigureCanonicalWheelSetups", "ValidateCanonicalWheelSetups", "ConfigureCanonicalPowertrain",
    "ValidateCanonicalPowertrain", "bMechanicalSimEnabled = true", "SetThrottleInput(EffectiveThrottle)",
    "SetSteeringInput(EffectiveSteering)", "SetBrakeInput", "TireIntegrity", "TerrainGripFactor",
    "ConditionPercent", "UGTTNativeDriveDynamicsSubsystem",
]:
    assert token in movement_cpp, f"dedicated Fieldmaster movement implementation missing {token}"
assert "SetTargetGear(" not in movement_cpp, "Fieldmaster component must not bypass shared drivetrain gear authority"
assert "Movement->SetTargetGear(Authority.StableDirection, true)" in authority_cpp
assert "GearMatchesDirection" in authority_cpp

for token in [
    "AGTTFieldmasterNativePawn(const FObjectInitializer& ObjectInitializer", "GetFieldmasterMovement",
    "RefreshNativeDriveCommand", "GetRequestedSteeringInput",
]:
    assert token in pawn_h, f"Fieldmaster pawn contract missing {token}"

for token in [
    "SetDefaultSubobjectClass<UGTTFieldmasterChaosMovementComponent>", "AWheeledVehiclePawn::VehicleMovementComponentName",
    "Movement->ConfigureAndValidateFieldmaster", "Movement->ApplyFieldmasterDriveCommand", "Movement->HoldFieldmasterStopped",
    "GetNativeTerrainGripFactor()", "MigrationSnapshot.TireIntegrity", "MigrationSnapshot.ConditionPercent",
]:
    assert token in pawn_cpp, f"Fieldmaster pawn is not wired to dedicated movement: {token}"

for token in ["TryActivateLegacyTakeover", "DeactivateLegacyTakeover", "LegacyMirror", "bTakeoverActive", "MigrationSnapshot.FuelLiters > KINDA_SMALL_NUMBER"]:
    assert token in pawn_cpp or token in pawn_h, f"native fallback/takeover contract missing {token}"

for checkbox in [
    "- [ ] Dedicated native Chaos wheeled tractor movement", "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup", "- [ ] Full Win64 CI/build runner",
]:
    assert checkbox in roadmap

for token in [
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "<!-- ROADMAP-PROGRESS:START -->", "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress", "../assets/readme/progress-mini.svg", "ROADMAP-96.2%25", "DONE-125%2F130",
    "| **125** | **5** | **130** | **96.2%** |",
]:
    assert token in roadmap, f"roadmap SVG-only presentation missing {token}"
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + open_items
assert (checked, open_items, total) == (125, 5, 130), (checked, open_items, total)
assert round(checked * 100.0 / total, 1) == 96.2
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, re.MULTILINE), "legacy text/Unicode roadmap progress meter must not return"

assert "Dedicated Native Chaos Tractor Movement" in playtest
assert "Win64" in playtest and "packaged" in playtest.lower()
assert "GTT 0.1.13" in changelog
assert "Verify dedicated Fieldmaster Chaos movement" in workflow

print("GTT 0.1.13 dedicated Fieldmaster Chaos movement source gate passed; shared 0.1.16 drivetrain authority owns gear direction and packaged runtime acceptance remains open; roadmap presentation is SVG-only")
