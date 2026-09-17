#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
movement = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterChaosMovementComponent.cpp").read_text(encoding="utf-8")
authority_h = (ROOT / "Source/GTT/Public/Vehicles/GTTNativeDriveDynamicsSubsystem.h").read_text(encoding="utf-8")
authority_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeDriveDynamicsSubsystem.cpp").read_text(encoding="utf-8")
powertrain = (ROOT / "Source/GTT/Private/Vehicles/GTTChaosPowertrainSetupLibrary.cpp").read_text(encoding="utf-8")
telemetry = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeRuntimeTelemetrySubsystem.cpp").read_text(encoding="utf-8")
evaluator = (ROOT / "Scripts/evaluate_native_chaos_runtime.ps1").read_text(encoding="utf-8")
win64_workflow = (ROOT / ".github/workflows/win64-package-evidence.yml").read_text(encoding="utf-8")
project_sanity = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
dedicated = (ROOT / ".github/workflows/automatic-drivetrain-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.1.16.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.1.16.md").read_text(encoding="utf-8")

errors = []

# Fieldmaster-specific input shaping must not select direction anymore. Doing so from the
# 0.1.13 component would race the shared drivetrain authority and pin automatic forward gear 1.
if "SetTargetGear(" in movement:
    errors.append("Fieldmaster movement still writes target gears instead of delegating direction authority")
for token in [
    "UGTTNativeDriveDynamicsSubsystem",
    "SetThrottleInput(EffectiveThrottle)",
    "SetSteeringInput(EffectiveSteering)",
    "SetBrakeInput",
    "automatic forward gears",
]:
    if token not in movement:
        errors.append(f"Fieldmaster movement missing 0.1.16 delegation token: {token}")

for token in [
    "LastObservedGear",
    "AutomaticForwardGearChangeCount",
    "DirectionShiftCommitCount",
    "GearCommandCount",
]:
    if token not in authority_h:
        errors.append(f"drivetrain authority state missing: {token}")

for token in [
    "DirectionShiftReleaseSpeedKmh = 3.5f",
    "GearMatchesDirection",
    "NATIVE_AUTOMATIC_GEAR_SHIFT",
    "NATIVE_AUTOMATIC_GEARBOX_EVIDENCE",
    "NATIVE_DIRECTION_SHIFT_COMMIT",
    "bDirectionInterlock = true",
    "FinalThrottle = 0.0f",
    "DirectionInterlockBrakeMin",
    "Movement->SetTargetGear(Authority.StableDirection, true)",
    "bDirectionShiftCommitted || !GearMatchesDirection",
]:
    if token not in authority_cpp:
        errors.append(f"shared drivetrain implementation missing: {token}")

# One centralized target-gear write is allowed. More than one usually means the old per-tick
# forward-gear forcing path or the interlock path has started writing gears again.
gear_writes = authority_cpp.count("Movement->SetTargetGear(Authority.StableDirection, true)")
if gear_writes != 1:
    errors.append(f"shared drivetrain must have exactly one centralized target-gear write, found {gear_writes}")

# During the high-speed interlock branch the old gear must be retained while braking.
interlock_start = authority_cpp.find("Authority.bDirectionInterlock = true;")
interlock_end = authority_cpp.find("else\n        {", interlock_start)
if interlock_start < 0 or interlock_end < 0:
    errors.append("could not identify direction-interlock branch")
elif "SetTargetGear(" in authority_cpp[interlock_start:interlock_end]:
    errors.append("direction interlock still changes target gear before speed is safe")

for token in [
    "bUseAutomaticGears = true",
    "bUseAutoReverse = false",
    "ForwardGearRatios",
    "ChangeUpRPM",
    "ChangeDownRPM",
]:
    if token not in powertrain:
        errors.append(f"Chaos powertrain automatic gearbox contract missing: {token}")

for token in [
    "signed_speed_kmh",
    "automatic_gears=%s",
    "forward_gears=%d",
    "TransmissionSetup.bUseAutomaticGears",
    "TransmissionSetup.ForwardGearRatios.Num()",
]:
    if token not in telemetry:
        errors.append(f"runtime telemetry missing automatic drivetrain evidence: {token}")

for token in [
    "automatic_gear_samples",
    "configured_forward_gears",
    "max_forward_gear_observed",
    "unsafe_direction_shift_commits",
    "max_direction_shift_commit_speed_kmh",
    "automatic_gears' 'YES",
    "forward_gears",
    "NATIVE_DIRECTION_SHIFT_COMMIT",
    "3.75",
]:
    if token not in evaluator:
        errors.append(f"native runtime evaluator missing drivetrain safety token: {token}")

if "evaluate_native_chaos_runtime.ps1" not in win64_workflow or "NATIVE_CHAOS_RUNTIME.json" not in win64_workflow:
    errors.append("Win64 evidence workflow no longer runs the Native Chaos runtime evaluator")
if "python Scripts/verify_automatic_drivetrain_runtime.py" not in project_sanity:
    errors.append("Project sanity does not execute the 0.1.16 drivetrain verifier")
if "python Scripts/verify_automatic_drivetrain_runtime.py" not in dedicated:
    errors.append("dedicated 0.1.16 workflow does not execute its verifier")
for token in ["verify_native_drivetrain_authority.py", "verify_fieldmaster_dedicated_chaos_movement.py", "verify_native_runtime_telemetry.py"]:
    if token not in dedicated:
        errors.append(f"dedicated 0.1.16 workflow missing regression verifier: {token}")

# Runtime/hardware acceptance remains open until the real UE 5.8 Win64 evidence exists.
for checkbox in [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Win64 CI/build runner",
]:
    if checkbox not in roadmap:
        errors.append(f"runtime/hardware gate was closed without packaged evidence: {checkbox}")
if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
    errors.append("SWIR roadmap standard marker missing")
if "📊 Overall progress" not in roadmap:
    errors.append("SWIR roadmap progress heading missing")
checked = len(re.findall(r"^\s*- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^\s*- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + open_items
if (checked, open_items, total) != (125, 5, 130):
    errors.append(f"roadmap changed unexpectedly: {checked}/{total} with {open_items} open; expected 125/130 with 5 open")
if "███████████████████░ 96.2%" not in roadmap or "| **125** | **5** | **130** | **96.2%** |" not in roadmap:
    errors.append("SWIR roadmap dashboard is stale or inconsistent")

for token in [
    "Automatic upshift freedom",
    "Forward-to-reverse interlock",
    "Safe reverse commit",
    "Rattleback parity",
    "Mulebox parity",
    "unsafe_direction_shift_commits",
    "visual acceptance",
]:
    if token.lower() not in playtest.lower():
        errors.append(f"0.1.16 playtest missing: {token}")
for token in [
    "GTT 0.1.16",
    "automatic-transmission conflict",
    "NATIVE_AUTOMATIC_GEARBOX_EVIDENCE",
    "does **not** claim",
    "125/130 (96.2%)",
]:
    if token.lower() not in changelog.lower():
        errors.append(f"0.1.16 changelog missing: {token}")

if errors:
    print("GTT 0.1.16 automatic drivetrain verification FAILED")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("GTT 0.1.16 automatic drivetrain verification OK")
print(" - Fieldmaster input shaping no longer races final direction authority")
print(" - automatic forward gears are not pinned by per-tick target-gear writes")
print(" - high-speed direction changes brake before committing the new direction")
print(" - packaged runtime gate records automatic gearbox state and rejects unsafe shift commits")
print(f" - roadmap remains honest at {checked}/{total} ({checked / total * 100:.1f}%)")
