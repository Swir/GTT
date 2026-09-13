from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / "Source/GTT/Public/Vehicles/GTTChaosPowertrainSetupLibrary.h").read_text(encoding="utf-8")
impl = (root / "Source/GTT/Private/Vehicles/GTTChaosPowertrainSetupLibrary.cpp").read_text(encoding="utf-8")
bridge_h = (root / "Source/GTT/Public/Vehicles/GTTChaosVehicleBridgeComponent.h").read_text(encoding="utf-8")
bridge_cpp = (root / "Source/GTT/Private/Vehicles/GTTChaosVehicleBridgeComponent.cpp").read_text(encoding="utf-8")
roadmap = (root / "Docs/ROADMAP.md").read_text(encoding="utf-8")

for symbol in [
    "ConfigureCanonicalPowertrain",
    "ValidateCanonicalPowertrain",
    "EngineSetup.MaxTorque",
    "EngineSetup.MaxRPM",
    "EngineSetup.EngineIdleRPM",
    "TransmissionSetup.FinalRatio",
    "TransmissionSetup.ForwardGearRatios",
    "TransmissionSetup.ReverseGearRatios",
    "DifferentialSetup.DifferentialType",
]:
    assert symbol in header + impl, f"missing canonical powertrain contract: {symbol}"

assert "EVehicleDifferential::FrontWheelDrive" in impl
assert "EVehicleDifferential::RearWheelDrive" in impl
assert "EVehicleDifferential::AllWheelDrive" in impl
assert "Spec.EngineMaxTorqueNm" in impl
assert "Spec.EngineMaxRpm" in impl
assert "Spec.EngineIdleRpm" in impl
assert "Spec.FinalDriveRatio" in impl
assert "Spec.ForwardGearRatios" in impl
assert "Spec.ReverseGearRatio" in impl
assert "Spec.DriveLayout" in impl

assert "ConfigureNativeMovementFromCanonicalSpec" in bridge_h
assert "ValidateNativePowertrain" in bridge_h
assert "bNativePowertrainValid" in bridge_h
assert "UGTTChaosPowertrainSetupLibrary::ConfigureCanonicalPowertrain" in bridge_cpp
assert "UGTTChaosPowertrainSetupLibrary::ValidateCanonicalPowertrain" in bridge_cpp
assert "bNativeMovementReady = NativeMovement != nullptr && bHasSpec && bRigValid && bWheelSetupValid && bPowertrainValid" in bridge_cpp
assert bridge_cpp.index("ValidateNativePowertrain()") < bridge_cpp.index("DisableLegacyDynamicsIfNeeded();")
assert "POWERTRAIN %s" in bridge_cpp

# Runtime acceptance remains intentionally open until authored assets + real UE runtime validation exist.
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap

checks = re.findall(r"^- \[[x ]\] ", roadmap, flags=re.MULTILINE)
done = re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE)
assert len(checks) == 130, f"roadmap total changed unexpectedly: {len(checks)}"
assert len(done) == 125, f"roadmap completed changed unexpectedly: {len(done)}"
assert "DONE-125%2F130" in roadmap and "ROADMAP-96.2%25" in roadmap
assert "███████████████████░ 96.2%" in roadmap

print("Native Chaos powertrain contract verified: engine + transmission + differential + takeover gate; roadmap remains 125/130.")
