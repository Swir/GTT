#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
scenario_h = (ROOT / "Source/GTT/Public/Core/GTTDrivetrainEvidenceScenarioSubsystem.h").read_text(encoding="utf-8")
scenario_cpp = (ROOT / "Source/GTT/Private/Core/GTTDrivetrainEvidenceScenarioSubsystem.cpp").read_text(encoding="utf-8")
evaluator = (ROOT / "Scripts/evaluate_drivetrain_scenario.ps1").read_text(encoding="utf-8")
win64 = (ROOT / ".github/workflows/win64-package-evidence.yml").read_text(encoding="utf-8")
project = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
dedicated = (ROOT / ".github/workflows/deterministic-drivetrain-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.1.17.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.1.17.md").read_text(encoding="utf-8")

errors = []

for token in [
    "UGTTDrivetrainEvidenceScenarioSubsystem",
    "ForwardAcceleration",
    "BrakeForReverse",
    "ReverseAcceleration",
    "BrakeForForward",
    "ForwardReturn",
    "bAutomaticUpshiftObserved",
    "bSafeReverseCommitObserved",
    "bSafeForwardCommitObserved",
]:
    if token not in scenario_h:
        errors.append(f"drivetrain evidence subsystem header missing: {token}")

for token in [
    "StartDelaySeconds = 76.0f",
    "GlobalDeadlineSeconds = 122.0f",
    "ShiftReleaseSpeedKmh = 3.5f",
    "ForwardEvidenceSpeedKmh = 6.0f",
    "ReverseEvidenceSpeedKmh = 5.0f",
    "NATIVE_DRIVETRAIN_SCENARIO_BEGIN",
    "phase=AUTOMATIC_UPSHIFT",
    "phase=REVERSE_INTERLOCK",
    "phase=REVERSE_COMMIT",
    "phase=REVERSE_MOTION",
    "phase=FORWARD_COMMIT",
    "phase=FORWARD_MOTION",
    "NATIVE_DRIVETRAIN_SCENARIO_COMPLETE",
    "route=forward-auto-reverse-forward",
]:
    if token not in scenario_cpp:
        errors.append(f"deterministic drivetrain source missing: {token}")

# The late evidence pass must not race the original 0-75 second demo scenario.
start_match = re.search(r"StartDelaySeconds\s*=\s*([0-9.]+)f", scenario_cpp)
deadline_match = re.search(r"GlobalDeadlineSeconds\s*=\s*([0-9.]+)f", scenario_cpp)
if not start_match or float(start_match.group(1)) < 75.0:
    errors.append("drivetrain evidence starts before the core 75-second demo scenario is guaranteed finished")
if not deadline_match or float(deadline_match.group(1)) > 124.0:
    errors.append("drivetrain evidence deadline no longer leaves margin before the 125-second smoke minimum")

# Exactly three explicit target-gear commands are expected: initial forward, safe reverse, safe return.
gear_writes = scenario_cpp.count("Movement->SetTargetGear(")
if gear_writes != 3:
    errors.append(f"deterministic scenario must issue exactly three transition gear commands, found {gear_writes}")
if "Movement->SetTargetGear(-1, true);" not in scenario_cpp or scenario_cpp.count("Movement->SetTargetGear(1, true);") != 2:
    errors.append("deterministic scenario target-gear sequence is not forward -> reverse -> forward")

for token in [
    "gtt.native-drivetrain-scenario.v1",
    "NATIVE_CHAOS_RUNTIME.json",
    "automatic forward upshift was not proven",
    "reverse was committed above the safe release window",
    "forward return was committed above the safe release window",
    "reverse_motion_signed_speed_kmh",
    "max_forward_gear_observed",
    "diagnostic_failure_count",
    "3.75",
]:
    if token not in evaluator:
        errors.append(f"drivetrain runtime evaluator missing: {token}")

for token in [
    "evaluate_drivetrain_scenario.ps1",
    "NATIVE_DRIVETRAIN_SCENARIO.json",
    "MinimumAliveSeconds 125",
]:
    if token not in win64:
        errors.append(f"Win64 evidence workflow missing deterministic drivetrain gate: {token}")
if win64.find("evaluate_native_chaos_runtime.ps1") > win64.find("evaluate_drivetrain_scenario.ps1"):
    errors.append("drivetrain scenario evaluator must run after Native Chaos runtime evaluation")

if "python Scripts/verify_deterministic_drivetrain_scenario.py" not in project:
    errors.append("Project sanity does not run the 0.1.17 deterministic drivetrain verifier")
for token in [
    "python Scripts/verify_deterministic_drivetrain_scenario.py",
    "python Scripts/verify_automatic_drivetrain_runtime.py",
    "python Scripts/verify_native_runtime_telemetry.py",
    "python Scripts/verify_deterministic_demo_scenario.py",
]:
    if token not in dedicated:
        errors.append(f"dedicated 0.1.17 workflow missing verifier: {token}")

# No runtime/hardware Roadmap gate may move without a real UE 5.8 Win64 package and evidence artifact.
for checkbox in [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Win64 CI/build runner",
]:
    if checkbox not in roadmap:
        errors.append(f"runtime/hardware gate closed without real packaged evidence: {checkbox}")
if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap or "📊 Overall progress" not in roadmap:
    errors.append("SWIR Roadmap Style Lock v1 is missing")
checked = len(re.findall(r"^\s*- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^\s*- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + open_items
if (checked, open_items, total) != (125, 5, 130):
    errors.append(f"roadmap changed unexpectedly: {checked}/{total}, open={open_items}")
if "███████████████████░ 96.2%" not in roadmap or "| **125** | **5** | **130** | **96.2%** |" not in roadmap:
    errors.append("Roadmap dashboard is stale or inconsistent with 125/130")

for token in [
    "Automatic forward shift",
    "Reverse interlock",
    "Safe reverse commit",
    "Measured reverse motion",
    "Safe forward return",
    "NATIVE_DRIVETRAIN_SCENARIO.json",
    "visual acceptance",
]:
    if token.lower() not in playtest.lower():
        errors.append(f"0.1.17 playtest missing: {token}")
for token in [
    "GTT 0.1.17",
    "deterministic",
    "forward-auto-reverse-forward",
    "NATIVE_DRIVETRAIN_SCENARIO.json",
    "does **not** claim",
    "125/130 (96.2%)",
]:
    if token.lower() not in changelog.lower():
        errors.append(f"0.1.17 changelog missing: {token}")

if errors:
    print("GTT 0.1.17 deterministic drivetrain scenario verification FAILED")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("GTT 0.1.17 deterministic drivetrain scenario verification OK")
print(" - late packaged scenario avoids racing the existing 0-75 second demo controls")
print(" - automatic upshift, safe reverse commit, reverse motion and safe forward return are evidence-gated")
print(" - Win64 candidate workflow persists NATIVE_DRIVETRAIN_SCENARIO.json")
print(f" - Roadmap remains honest at {checked}/{total} ({checked / total * 100:.1f}%)")
