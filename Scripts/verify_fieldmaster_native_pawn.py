from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
source = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp").read_text(encoding="utf-8")
movement_h = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterChaosMovementComponent.h").read_text(encoding="utf-8")
movement_source = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterChaosMovementComponent.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.42.md").read_text(encoding="utf-8")

required_header = [
    "public AWheeledVehiclePawn",
    "AGTTFieldmasterNativePawn(const FObjectInitializer& ObjectInitializer",
    "ConfigureAndValidateNativeFieldmaster",
    "IsNativeFieldmasterReady",
    "GetNativeAcceptanceSummary",
    "GetFieldmasterMovement",
]
for token in required_header:
    assert token in header, f"missing native pawn contract: {token}"

required_source = [
    'FieldmasterVehicleId(TEXT("RustyFieldmaster60"))',
    "GetVehicleMovementComponent()",
    "GetRequiredBoneNames",
    "GetRequiredSocketNames",
    "SetDefaultSubobjectClass<UGTTFieldmasterChaosMovementComponent>",
    "Movement->ConfigureAndValidateFieldmaster",
    "GetPhysicsAsset()",
]
for token in required_source:
    assert token in source, f"missing native activation behavior: {token}"

for token in [
    "public UChaosWheeledVehicleMovementComponent",
    "ConfigureCanonicalWheelSetups",
    "ConfigureCanonicalPowertrain",
    "ValidateCanonicalWheelSetups",
    "ValidateCanonicalPowertrain",
    "SetThrottleInput",
    "SetSteeringInput",
    "SetTargetGear",
]:
    assert token in movement_h or token in movement_source, f"missing dedicated movement behavior: {token}"

assert "bNativeReady = bMovementValid && bPhysicsAssetPresent" in source
assert "Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap, "do not close runtime acceptance without UE evidence"
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap, "do not close fleet Native Chaos without runtime evidence"
assert "125" in roadmap and "130" in roadmap and "96.2%" in roadmap
assert "AWheeledVehiclePawn" in playtest and "packaged" in playtest.lower()

print("Fieldmaster native pawn source acceptance: OK (dedicated movement subclass wired; runtime roadmap gate remains open)")
