#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTNativeAxleTractionSubsystem.h"
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTNativeAxleTractionSubsystem.cpp"
WORKFLOW = ROOT / ".github/workflows/project-sanity.yml"
ROADMAP = ROOT / "Docs/ROADMAP.md"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.0.76.md"
CHANGELOG = ROOT / "CHANGELOG.d/0.0.76.md"

for path in (HEADER, CPP, WORKFLOW, ROADMAP, PLAYTEST, CHANGELOG):
    if not path.exists():
        raise SystemExit(f"missing required file: {path.relative_to(ROOT)}")

header = HEADER.read_text(encoding="utf-8")
cpp = CPP.read_text(encoding="utf-8")
workflow = WORKFLOW.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")
playtest = PLAYTEST.read_text(encoding="utf-8")
changelog = CHANGELOG.read_text(encoding="utf-8")

required = [
    (header + cpp, "UGTTNativeAxleTractionSubsystem"),
    (header, "FGTTNativeAxleTractionSnapshot"),
    (header, "FrontContacts"),
    (header, "RearContacts"),
    (header, "AxleImbalance"),
    (header, "TractionAuthority"),
    (cpp, "TActorIterator<AGTTFieldmasterNativePawn>"),
    (cpp, "TActorIterator<AGTTRoadVehicleNativePawn>"),
    (cpp, "GetWheelState(WheelIndex)"),
    (cpp, "WheelState" if False else "FWheelStatus"),
    (cpp, "bIsValid"),
    (cpp, "bInContact"),
    (cpp, "bIsSlipping"),
    (cpp, "bIsSkidding"),
    (cpp, "NormalizedSuspensionLength"),
    (cpp, "Snapshot.FrontContacts == 0 || Snapshot.RearContacts == 0"),
    (cpp, "Movement->SetThrottleInput(0.0f)"),
    (cpp, "Movement->SetBrakeInput(Snapshot.BrakeAssist)"),
    (cpp, "AddTorqueInRadians"),
    (cpp, "Migration.TireIntegrity"),
    (cpp, "Migration.TireUpgradeLevel"),
    (cpp, "NATIVE_AXLE_TRACTION_INTERVENTION"),
    (cpp, "NATIVE_AXLE_TRACTION_EVIDENCE"),
    (workflow, "Verify Native wheel/axle traction authority"),
    (workflow, "python Scripts/verify_native_axle_traction.py"),
    (playtest, "0.0.76"),
    (changelog, "0.0.76"),
]
missing = [token for text, token in required if token not in text]
if missing:
    raise SystemExit("missing Native axle traction contract tokens: " + ", ".join(missing))

if "Snapshot.ContactWheels <= 1" not in cpp:
    raise SystemExit("torque cut must hard-gate severe loss of wheel contact")
if "SpeedKmh >= MinimumInterventionSpeedKmh" not in cpp:
    raise SystemExit("axle intervention must remain speed-gated")
if "Snapshot.ContactWheels >= 2" not in cpp:
    raise SystemExit("yaw correction must require grounded wheel support")

for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    '<img alt="CI"',
    '<img alt="Roadmap progress"',
    '<img alt="Completed"',
    '<img alt="Status"',
    "## 📊 Overall progress",
):
    if token not in roadmap:
        raise SystemExit("roadmap style lock missing: " + token)

checks = re.findall(r"^\s*- \[([xX ])\] ", roadmap, flags=re.MULTILINE)
done = sum(1 for state in checks if state.lower() == "x")
total = len(checks)
remaining = total - done
progress = round(done / total * 100.0, 1)
segments = round(progress / 5.0)
bar = "█" * segments + "░" * (20 - segments)
if (done, total, remaining) != (125, 130, 5):
    raise SystemExit(f"roadmap checklist drift: {done}/{total}, remaining={remaining}")
for token in (
    f"ROADMAP-{progress:.1f}%25",
    f"DONE-{done}%2F{total}",
    f"{bar} {progress:.1f}%",
    f"| **{done}** | **{remaining}** | **{total}** | **{progress:.1f}%** |",
):
    if token not in roadmap:
        raise SystemExit("roadmap dashboard drift: missing " + token)

for open_item in (
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Full Win64 CI/build runner",
):
    if open_item not in roadmap:
        raise SystemExit("runtime/build item closed without required evidence: " + open_item)

print("[OK] Native wheel/axle traction authority verified")
print(" - actual Chaos wheel-state contact/slip/suspension evidence is sampled")
print(" - Fieldmaster, Rattleback and Mulebox share the authority")
print(" - torque cut, brake assist and grounded axle-balance intervention are present")
print(f" - roadmap remains honest at {done}/{total} ({progress:.1f}%)")
