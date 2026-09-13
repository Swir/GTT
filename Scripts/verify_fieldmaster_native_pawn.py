from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
source = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.42.md").read_text(encoding="utf-8")

required_header = [
    "public AWheeledVehiclePawn",
    "ConfigureAndValidateNativeFieldmaster",
    "IsNativeFieldmasterReady",
    "GetNativeAcceptanceSummary",
]
for token in required_header:
    assert token in header, f"missing native pawn contract: {token}"

required_source = [
    'FieldmasterVehicleId(TEXT("RustyFieldmaster60"))',
    "GetVehicleMovementComponent()",
    "GetRequiredBoneNames",
    "GetRequiredSocketNames",
    "ConfigureCanonicalWheelSetups",
    "ConfigureCanonicalPowertrain",
    "ValidateCanonicalWheelSetups",
    "ValidateCanonicalPowertrain",
    "GetPhysicsAsset()",
]
for token in required_source:
    assert token in source, f"missing native activation behavior: {token}"

assert "bNativeReady = bWheelsValid && bPowertrainValid && bPhysicsAssetPresent" in source
assert "Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap, "do not close runtime acceptance without UE evidence"
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap, "do not close fleet Native Chaos without runtime evidence"
assert "125" in roadmap and "130" in roadmap and "96.2%" in roadmap
assert "AWheeledVehiclePawn" in playtest and "packaged" in playtest.lower()

print("Fieldmaster native pawn source acceptance: OK")
