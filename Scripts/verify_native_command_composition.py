#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
DRIVE_H = ROOT / "Source/GTT/Public/Vehicles/GTTNativeDriveDynamicsSubsystem.h"
DRIVE_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTNativeDriveDynamicsSubsystem.cpp"
AXLE_H = ROOT / "Source/GTT/Public/Vehicles/GTTNativeAxleTractionSubsystem.h"
AXLE_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTNativeAxleTractionSubsystem.cpp"
WORKFLOW = ROOT / ".github/workflows/project-sanity.yml"
ROADMAP = ROOT / "Docs/ROADMAP.md"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.0.77.md"
CHANGELOG = ROOT / "CHANGELOG.d/0.0.77.md"

paths = (DRIVE_H, DRIVE_CPP, AXLE_H, AXLE_CPP, WORKFLOW, ROADMAP, PLAYTEST, CHANGELOG)
errors = []
for path in paths:
    if not path.exists():
        errors.append(f"missing required file: {path.relative_to(ROOT)}")

if errors:
    print("\n".join(f"ERROR: {e}" for e in errors))
    sys.exit(1)

drive_h = DRIVE_H.read_text(encoding="utf-8")
drive_cpp = DRIVE_CPP.read_text(encoding="utf-8")
axle_h = AXLE_H.read_text(encoding="utf-8")
axle_cpp = AXLE_CPP.read_text(encoding="utf-8")
workflow = WORKFLOW.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")
playtest = PLAYTEST.read_text(encoding="utf-8")
changelog = CHANGELOG.read_text(encoding="utf-8")

required_drive = [
    "UGTTNativeAxleTractionSubsystem",
    "SampleSnapshot(Movement, TireIntegrity, TireUpgradeLevel)",
    "Movement->GetThrottleInput()",
    "Movement->GetBrakeInput()",
    "Movement->GetSteeringInput()",
    "FMath::Max3(FinalBrake, DrivetrainBrake, AxleBrake)",
    "if (AxleSnapshot.bTorqueCut) FinalThrottle = 0.0f",
    "Movement->SetThrottleInput(FinalThrottle)",
    "Movement->SetBrakeInput(FinalBrake)",
    "Movement->SetSteeringInput(FinalSteering)",
    "NATIVE_COMMAND_COMPOSITION_EVIDENCE",
    "NATIVE_COMMAND_COMPOSITION_LIMIT",
]
for token in required_drive:
    if token not in drive_cpp:
        errors.append(f"final command composer missing token: {token}")

required_state = [
    "bAxleTorqueCut",
    "bSuspensionRuntimeReady",
    "FinalThrottle",
    "FinalBrake",
    "FinalSteering",
]
for token in required_state:
    if token not in drive_h:
        errors.append(f"command authority state missing token: {token}")

required_axle = [
    "SampleSnapshot(",
    "bSuspensionRuntimeReady",
    "SuspensionSamples",
    "MinSuspensionTravel",
    "MaxSuspensionTravel",
]
for token in required_axle:
    if token not in axle_h + axle_cpp:
        errors.append(f"axle/suspension evidence missing token: {token}")

for forbidden in (
    "Movement->SetThrottleInput(0.0f)",
    "Movement->SetBrakeInput(Snapshot.BrakeAssist)",
):
    if forbidden in axle_cpp:
        errors.append(f"axle subsystem still owns competing movement input: {forbidden}")

if "NormalizedSuspensionLength" not in axle_cpp or "Snapshot.SuspensionSamples == 4" not in axle_cpp:
    errors.append("live four-wheel suspension runtime evidence is incomplete")
if "NATIVE_AXLE_TRACTION_RECOMMENDATION" not in axle_cpp:
    errors.append("axle recommendation telemetry missing")

if "python Scripts/verify_native_command_composition.py" not in workflow:
    errors.append("Project sanity does not execute verify_native_command_composition.py")

for token in ("strictest brake", "direction interlock", "suspension", "Fieldmaster", "Rattleback", "Mulebox"):
    if token.lower() not in playtest.lower():
        errors.append(f"playtest missing coverage phrase: {token}")
if "0.0.77" not in changelog or "composition" not in changelog.lower():
    errors.append("0.0.77 changelog does not describe command composition milestone")

style_tokens = (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    '<img alt="CI"',
    '<img alt="Roadmap progress"',
    '<img alt="Completed"',
    '<img alt="Status"',
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
)
for token in style_tokens:
    if token not in roadmap:
        errors.append("roadmap style lock missing: " + token)

checks = re.findall(r"^\s*- \[([xX ])\] ", roadmap, flags=re.MULTILINE)
done = sum(1 for state in checks if state.lower() == "x")
total = len(checks)
remaining = total - done
progress = round(done / total * 100.0, 1)
if (done, total, remaining) != (125, 130, 5):
    errors.append(f"roadmap checklist drift: {done}/{total}, remaining={remaining}")
for token in (
    f"ROADMAP-{progress:.1f}%25",
    f"DONE-{done}%2F{total}",
    f"| **{done}** | **{remaining}** | **{total}** | **{progress:.1f}%** |",
):
    if token not in roadmap:
        errors.append("roadmap dashboard drift: missing " + token)
if "<!-- ROADMAP-PROGRESS:START -->" in roadmap and "<!-- ROADMAP-PROGRESS:END -->" in roadmap:
    progress_block = roadmap.split("<!-- ROADMAP-PROGRESS:START -->", 1)[1].split("<!-- ROADMAP-PROGRESS:END -->", 1)[0]
    if progress_block.count("../assets/readme/progress-mini.svg") != 1:
        errors.append("roadmap progress block must embed exactly one canonical progress-mini.svg")
    if re.search(r"[█▓▒░]{3,}", progress_block):
        errors.append("legacy text/Unicode progress meter must not return to active Roadmap dashboard")

for open_item in (
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Full Win64 CI/build runner",
):
    if open_item not in roadmap:
        errors.append("runtime/build item closed without required evidence: " + open_item)

if errors:
    print("Native command composition verification FAILED")
    for error in errors:
        print(" - " + error)
    sys.exit(1)

print("[OK] Native command composition and suspension runtime evidence verified")
print(" - axle subsystem is evidence-only for movement input ownership")
print(" - final throttle/brake/steering commands compose drivetrain and axle limits")
print(" - strictest brake and torque-cut semantics are enforced")
print(" - four-wheel live suspension evidence is sampled")
print(f" - roadmap remains honest at {done}/{total} ({progress:.1f}%) with SVG-only progress")
