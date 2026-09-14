from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
bridge_h = (ROOT / "Source/GTT/Public/Vehicles/GTTChaosVehicleBridgeComponent.h").read_text(encoding="utf-8")
bridge_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTChaosVehicleBridgeComponent.cpp").read_text(encoding="utf-8")
old_car = (ROOT / "Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp").read_text(encoding="utf-8")
van = (ROOT / "Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp").read_text(encoding="utf-8")
tractor = (ROOT / "Source/GTT/Private/Vehicles/GTTTractorPawn.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.64.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.64.md").read_text(encoding="utf-8")

required_header = [
    "FGTTChaosWheelRuntimeSnapshot",
    "GetWheelRuntimeSnapshot",
    "UpdateFleetWheelRuntime",
    "WheelRuntimeSnapshot",
    "SuspensionLength",
    "SpringForce",
    "DriveTorque",
    "BrakeTorque",
]
for token in required_header:
    assert token in bridge_h, f"missing bridge runtime contract token: {token}"

required_cpp = [
    "GetWheelState(WheelIndex)",
    "bIsSlipping",
    "bIsSkidding",
    "NormalizedSuspensionLength",
    "SpringForce",
    "DriveTorque",
    "BrakeTorque",
    "ComputeFleetWheelRisk",
    "NATIVE_FLEET_WHEEL_STATE_EVIDENCE",
    "RuntimeThrottleLimit",
    "RuntimeSteeringLimit",
    "RuntimeBrakeAssist",
    "GetTireIntegrity()",
    "GetTireUpgradeLevel()",
]
for token in required_cpp:
    assert token in bridge_cpp, f"missing fleet runtime implementation token: {token}"

for source, vehicle in [(tractor, "RustyFieldmaster60"), (old_car, "Rattleback82"), (van, "Mulebox1200")]:
    assert "UGTTChaosVehicleBridgeComponent" in source, f"{vehicle} is not wired to shared Chaos bridge"
    assert "CaptureThrottleInput" in source and "CaptureSteeringInput" in source, f"{vehicle} does not feed shared native inputs"

assert "Rattleback 82" in playtest and "Mulebox 1200" in playtest and "Fieldmaster" in playtest
assert "Fleet Native Wheel-State" in changelog

# Runtime-gated roadmap items must remain honest until an actual UE/Win64 run closes them.
for open_item in [
    "Dedicated native Chaos wheeled tractor movement",
    "Full Unreal compile + packaged Win64 smoke test",
    "Dedicated native Chaos drivetrain/suspension/wheel setup",
    "Full Win64 CI/build runner",
]:
    assert f"- [ ] {open_item}" in roadmap, f"runtime-only roadmap item was closed without runtime evidence: {open_item}"

assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
open_count = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
assert checked == 125 and open_count == 5 and checked + open_count == 130, f"roadmap drift: {checked}/{checked + open_count}"
assert "DONE-125%2F130" in roadmap and "ROADMAP-96.2%25" in roadmap
assert "███████████████████░ 96.2%" in roadmap

print("Fleet Native wheel-state control verifier: OK")
