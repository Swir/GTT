from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

setup_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTChaosNativeSetupLibrary.cpp").read_text(encoding="utf-8")
physics_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTNativePhysicsAcceptanceSubsystem.cpp").read_text(encoding="utf-8")
wheels_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTChaosVehicleWheels.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.62.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.62.md").read_text(encoding="utf-8")

required_setup_tokens = [
    'Vehicles/GTTChaosVehicleSpec.h',
    'ValidateWheelDefaults',
    'WheelRadius',
    'WheelWidth',
    'SuspensionMaxRaise',
    'SuspensionMaxDrop',
    'SpringRate',
    'SuspensionDampingRatio',
    'FrictionForceMultiplier',
    'bAffectedBySteering',
    'bAffectedByEngine',
    'bAffectedByBrake',
    'bAffectedByHandbrake',
    'MaxSteerAngle',
    'bABSEnabled',
    'bTractionControlEnabled',
    'AdditionalOffset.IsNearlyZero()',
    'GetSpecForVehicleId',
    'Invalid native authored wheel setup',
]
for token in required_setup_tokens:
    assert token in setup_cpp, f"missing authored wheel setup contract token: {token}"

required_runtime_tokens = [
    'ValidateCanonicalWheelSetups',
    'authored wheel/suspension setup invalid',
    'NATIVE_WHEEL_SETUP_EVIDENCE',
    'NATIVE_PHYSICS_FALLBACK',
    'FrontWheel ? FrontWheel->SuspensionMaxRaise',
    'RearWheel ? RearWheel->SuspensionMaxDrop',
    'FrontWheel ? FrontWheel->SpringRate',
    'RearWheel ? RearWheel->SuspensionDampingRatio',
]
for token in required_runtime_tokens:
    assert token in physics_cpp, f"missing runtime authored wheel acceptance token: {token}"

required_wheel_authoring = [
    'Wheel.WheelRadius = Spec.RadiusCm',
    'Wheel.SuspensionMaxRaise = Spec.SuspensionMaxRaiseCm',
    'Wheel.SuspensionMaxDrop = Spec.SuspensionMaxDropCm',
    'Wheel.SpringRate = Spec.SuspensionSpringRate',
    'Wheel.SuspensionDampingRatio = Spec.SuspensionDampingRatio',
    'Wheel.FrictionForceMultiplier = Spec.FrictionForceMultiplier',
]
for token in required_wheel_authoring:
    assert token in wheels_cpp, f"canonical wheel classes no longer consume spec field: {token}"

for token in [
    'NATIVE_WHEEL_SETUP_EVIDENCE',
    'Broken authored setup fallback',
    'Unreal Engine 5.8',
    'Win64',
]:
    assert token in playtest, f"playtest missing required acceptance coverage: {token}"

assert '0.0.62' in changelog
assert '125/130' in changelog

# Runtime-only roadmap gates must remain open until a genuine Unreal/Win64 acceptance run proves them.
assert '- [ ] Dedicated native Chaos wheeled tractor movement' in roadmap
assert '- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup' in roadmap
assert '- [ ] Full Unreal compile + packaged Win64 smoke test' in roadmap
assert '- [ ] Full Win64 CI/build runner' in roadmap

checked = len(re.findall(r'^- \[x\] ', roadmap, flags=re.MULTILINE))
unchecked = len(re.findall(r'^- \[ \] ', roadmap, flags=re.MULTILINE))
total = checked + unchecked
assert (checked, total) == (125, 130), f"unexpected roadmap count: {checked}/{total}"
assert '<!-- SWIR-ROADMAP-STANDARD:v1 -->' in roadmap
assert 'ROADMAP-96.2%25' in roadmap
assert 'DONE-125%2F130' in roadmap
assert '███████████████████░ 96.2%' in roadmap

print('Native authored wheel/suspension acceptance contract verified.')
