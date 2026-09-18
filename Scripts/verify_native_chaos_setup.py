from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / "Source/GTT/Public/Vehicles/GTTChaosNativeSetupLibrary.h").read_text(encoding="utf-8")
impl = (root / "Source/GTT/Private/Vehicles/GTTChaosNativeSetupLibrary.cpp").read_text(encoding="utf-8")
bridge_h = (root / "Source/GTT/Public/Vehicles/GTTChaosVehicleBridgeComponent.h").read_text(encoding="utf-8")
bridge_cpp = (root / "Source/GTT/Private/Vehicles/GTTChaosVehicleBridgeComponent.cpp").read_text(encoding="utf-8")
roadmap = (root / "Docs/ROADMAP.md").read_text(encoding="utf-8")

required_ids = ["RustyFieldmaster60", "Rattleback82", "Mulebox1200"]
for vehicle_id in required_ids:
    assert vehicle_id in impl, f"missing native setup mapping for {vehicle_id}"

required_wheels = [
    "UGTTFieldmasterFrontWheel", "UGTTFieldmasterRearWheel",
    "UGTTRattlebackFrontWheel", "UGTTRattlebackRearWheel",
    "UGTTMuleboxFrontWheel", "UGTTMuleboxRearWheel",
]
for wheel in required_wheels:
    assert wheel in impl, f"missing wheel class {wheel}"

for bone in ["FrontLeftWheelBone", "FrontRightWheelBone", "RearLeftWheelBone", "RearRightWheelBone"]:
    assert bone in impl, f"native setup does not consume rig contract bone {bone}"

assert "FChaosWheelSetup" in impl
assert "Movement->WheelSetups.Reset" in impl
assert "Movement->WheelSetups.Add" in impl
assert "bMechanicalSimEnabled = true" in impl
assert "ConfigureCanonicalWheelSetups" in header
assert "ValidateCanonicalWheelSetups" in header

assert "ValidateNativeWheelSetup" in bridge_h
assert "bNativeWheelSetupValid" in bridge_h
assert "UGTTChaosNativeSetupLibrary::ValidateCanonicalWheelSetups" in bridge_cpp
assert "bNativeMovementReady = NativeMovement != nullptr && bHasSpec && bRigValid && bWheelSetupValid" in bridge_cpp
assert bridge_cpp.index("ValidateNativeWheelSetup()") < bridge_cpp.index("DisableLegacyDynamicsIfNeeded();"), "legacy dynamics may be disabled before wheel validation"
assert "WHEELS %s" in bridge_cpp

# Native Chaos tasks remain open until real UE runtime acceptance exists.
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
):
    assert token in roadmap, f"roadmap SVG-only presentation missing {token}"

checks = re.findall(r"^- \[[x ]\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE)
done = re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE)
assert len(checks) == 130, f"roadmap total changed unexpectedly: {len(checks)}"
assert len(done) == 125, f"roadmap completed changed unexpectedly: {len(done)}"
assert "DONE-125%2F130" in roadmap and "ROADMAP-96.2%25" in roadmap
assert "| **125** | **5** | **130** | **96.2%** |" in roadmap
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE), "legacy text/Unicode roadmap progress meter must not return"

print("Native Chaos setup contract verified: fleet wheel classes + rig bones + bridge takeover gate; roadmap remains 125/130 with SVG-only progress.")
