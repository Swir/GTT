#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
stability = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeStabilitySubsystem.cpp").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.63.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.63.md").read_text(encoding="utf-8")
migration = (ROOT / "Docs/CHAOS_VEHICLE_MIGRATION.md").read_text(encoding="utf-8")

required_source = [
    "GetNumWheels() != 4",
    "GetWheelState(WheelIndex)",
    "WheelState.bIsValid",
    "WheelState.bInContact",
    "WheelState.bIsSlipping",
    "WheelState.bIsSkidding",
    "WheelState.NormalizedSuspensionLength",
    "WheelState.SpringForce",
    "WheelState.DriveTorque",
    "WheelState.BrakeTorque",
    "WheelState.SlipMagnitude",
    "NATIVE_CHAOS_WHEEL_STATE_EVIDENCE",
    'TEXT("CHAOS")',
    'TEXT("TRACE_FALLBACK")',
    "TractionRisk = FMath::Max(TractionRisk, ChaosSlipRisk)",
    "ComputeAxleRuntimeGrip(ChaosWheelEvidence, 0)",
    "ComputeAxleRuntimeGrip(ChaosWheelEvidence, 2)",
]
missing = [token for token in required_source if token not in stability]
if missing:
    raise SystemExit(f"Missing Native Chaos wheel-state runtime contract tokens: {missing}")

if "verify_native_chaos_wheel_state.py" not in workflow:
    raise SystemExit("Project sanity does not execute the Native Chaos wheel-state verifier")

if "NATIVE_CHAOS_WHEEL_STATE_EVIDENCE" not in playtest or "GetWheelState" not in playtest:
    raise SystemExit("0.0.63 playtest does not require real Chaos wheel-state evidence")

if "0.0.63" not in changelog or "FWheelStatus" not in changelog:
    raise SystemExit("0.0.63 changelog is missing the FWheelStatus milestone")

if "NATIVE_CHAOS_WHEEL_STATE_EVIDENCE" not in migration:
    raise SystemExit("Chaos migration documentation is missing the runtime wheel-state evidence contract")

for required_dashboard in (
    '<!-- SWIR-ROADMAP-STANDARD:v1 -->', '<!-- ROADMAP-PROGRESS:START -->', '<!-- ROADMAP-PROGRESS:END -->',
    'alt="CI"', 'badge/ROADMAP-', 'badge/DONE-', 'badge/STATUS-', '## 📊 Overall progress',
    '../assets/readme/progress-mini.svg'):
    if required_dashboard not in roadmap:
        raise SystemExit(f"ROADMAP dashboard element missing: {required_dashboard}")

checks = re.findall(r"^- \[(x| )\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE)
done = sum(1 for mark in checks if mark.lower() == "x")
total = len(checks)
if (done, total) != (125, 130):
    raise SystemExit(f"Roadmap checklist changed unexpectedly: {done}/{total}, expected 125/130")

runtime_open = [
    "Dedicated native Chaos wheeled tractor movement",
    "Full Unreal compile + packaged Win64 smoke test",
    "Dedicated native Chaos drivetrain/suspension/wheel setup",
    "Authored skeletal trailer wheel assets and final hitch sockets",
    "Full Win64 CI/build runner",
]
for item in runtime_open:
    if f"- [ ] {item}" not in roadmap:
        raise SystemExit(f"Runtime-only roadmap item was closed without runtime proof: {item}")

if "ROADMAP-96.2%25" not in roadmap or "DONE-125%2F130" not in roadmap:
    raise SystemExit("ROADMAP numeric badges no longer match 125/130")
if "| **125** | **5** | **130** | **96.2%** |" not in roadmap:
    raise SystemExit("ROADMAP dashboard table no longer matches 125/130")
if roadmap.count("../assets/readme/progress-mini.svg") != 1:
    raise SystemExit("ROADMAP must embed exactly one canonical progress-mini.svg")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    raise SystemExit("Legacy text/Unicode roadmap progress meter must not return")

print("Native Chaos wheel-state runtime evidence contract OK")
print("Roadmap remains 125/130 with SVG-only progress; UE 5.8 / packaged Win64 acceptance is still required before closing runtime tasks")
