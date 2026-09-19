#!/usr/bin/env python3
"""Verify GTT 0.1.57 physical trailer suspension/hitch integration."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTFarmTrailerDynamicsSubsystem.h"
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFarmTrailerDynamicsSubsystem.cpp"
TRAILER_HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTFarmTrailer.h"
TRAILER_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFarmTrailer.cpp"
ROADMAP = ROOT / "Docs/ROADMAP.md"

errors = []
for path in (HEADER, CPP, TRAILER_HEADER, TRAILER_CPP, ROADMAP):
    if not path.exists():
        errors.append(f"missing required file: {path.relative_to(ROOT)}")

if errors:
    print("Trailer suspension/hitch verification FAILED")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

header = HEADER.read_text(encoding="utf-8")
cpp = CPP.read_text(encoding="utf-8")
trailer_header = TRAILER_HEADER.read_text(encoding="utf-8")
trailer_cpp = TRAILER_CPP.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")

required_header_tokens = (
    "class GTT_API UGTTFarmTrailerDynamicsSubsystem : public UTickableWorldSubsystem",
    "FGTTFarmTrailerDynamicsSnapshot GetSnapshot",
    "bLeftSuspensionActive",
    "bRightSuspensionActive",
    "HitchBreakForce",
    "HitchBreakTorque",
)
for token in required_header_tokens:
    if token not in header:
        errors.append(f"trailer dynamics header missing token: {token}")

required_cpp_tokens = (
    "SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, SuspensionTravelCm)",
    "SetLinearPositionDrive(false, false, true)",
    "SetLinearVelocityDrive(false, false, true)",
    "SetLinearDriveAccelerationMode(true)",
    "SetLinearDriveParams(SpringStrength, DampingStrength, SuspensionForceLimit)",
    "LoadedSpringMultiplier",
    "LoadedDampingMultiplier",
    "SetLinearBreakable(true, HitchBreakForce)",
    "SetAngularBreakable(true, HitchBreakTorque)",
    "MaximumDynamicHitchWeakening",
    "Trailer->DetachTrailer()",
    "TRAILER_HITCH_PHYSICS_BREAK",
    "TRAILER_NATIVE_DYNAMICS",
)
for token in required_cpp_tokens:
    if token not in cpp:
        errors.append(f"trailer dynamics implementation missing token: {token}")

# The trailer actor remains the single cargo/damage/attachment authority.
for token in (
    "bool IsAttached() const",
    "bool HasCargo() const",
    "float GetTrailerIntegrity() const",
    "float GetHitchLoad() const",
    "void DetachTrailer()",
):
    if token not in trailer_header:
        errors.append(f"trailer gameplay authority API missing token: {token}")

# Preserve the already-connected loaded/unloaded rigid-body mass loop and wheel repair path.
for token in (
    "bCargoLoaded ? 1680.0f : 980.0f",
    "RestoreWheel(LeftWheel, LeftWheelConstraint, LeftWheelHome)",
    "RestoreWheel(RightWheel, RightWheelConstraint, RightWheelHome)",
):
    if token not in trailer_cpp:
        errors.append(f"existing trailer gameplay integration missing token: {token}")

# This subsystem must not become a second vehicle-input authority. Native vehicle inputs remain
# composed by UGTTNativeDriveDynamicsSubsystem.
for forbidden in (
    "SetThrottleInput(",
    "SetBrakeInput(",
    "SetSteeringInput(",
    "SetTargetGear(",
):
    if forbidden in cpp:
        errors.append(f"trailer dynamics illegally writes native drivetrain input: {forbidden}")

checked = len(re.findall(r"^\s*- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^\s*- \[ \] ", roadmap, flags=re.MULTILINE))
if (checked, open_items) != (125, 5):
    errors.append(f"roadmap truth changed unexpectedly: checked={checked}, open={open_items}, expected 125/5")

for open_gate in (
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Win64 CI/build runner",
):
    if open_gate not in roadmap:
        errors.append(f"runtime/art acceptance gate was removed or falsely closed: {open_gate}")

if "**125** | **5** | **130** | **96.2%**" not in roadmap:
    errors.append("roadmap summary no longer matches 125/130 = 96.2%")

if errors:
    print("Trailer suspension/hitch verification FAILED")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Trailer suspension/hitch verification OK")
print(" - wheel constraints gain bounded Z travel with load-sensitive spring/damping")
print(" - physical hitch break force/torque reacts to cargo, damage and live hitch stress")
print(" - physical hitch failure returns through the existing authoritative detach path")
print(" - native drivetrain input authority remains outside the trailer subsystem")
print(" - roadmap truth remains 125/130 (96.2%); runtime/art/Win64 gates stay open")
