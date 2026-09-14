from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / "Source/GTT/Public/Vehicles/GTTNativeTerrainLoadSubsystem.h").read_text(encoding="utf-8")
cpp = (root / "Source/GTT/Private/Vehicles/GTTNativeTerrainLoadSubsystem.cpp").read_text(encoding="utf-8")
roadmap = (root / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (root / "Docs/PLAYTEST_0.0.61.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.d/0.0.61.md").read_text(encoding="utf-8")

for token in [
    "FWheelLoadEvidence",
    "FrontLeftClearanceCm",
    "FrontRightClearanceCm",
    "RearLeftClearanceCm",
    "RearRightClearanceCm",
    "FrontCrossAxleImbalance",
    "RearCrossAxleImbalance",
    "SideLoadImbalance",
    "GroundedWheelCount",
    "CrossAxleRiskSeconds",
    "SampleWheelLoadEvidence",
]:
    if token not in header:
        raise SystemExit(f"Per-wheel load header missing token: {token}")

for token in [
    "FrontLeftWheelBone",
    "FrontRightWheelBone",
    "RearLeftWheelBone",
    "RearRightWheelBone",
    "GTTNativeWheelLoad",
    "NATIVE_WHEEL_LOAD_EVIDENCE",
    "FrontAxleGrip",
    "RearAxleGrip",
    "CrossAxleRisk",
    "CrossAxleGraceSeconds",
    "Movement->SetThrottleInput",
    "Movement->SetBrakeInput",
    "Movement->SetSteeringInput(0.0f)",
    "GetNativeTerrainGripFactor()",
    "Snapshot.TireIntegrity",
    "Snapshot.TireUpgradeLevel",
    "TowLoad",
]:
    if token not in cpp:
        raise SystemExit(f"Per-wheel load implementation missing token: {token}")

if "4.0f / RawTotal" not in cpp:
    raise SystemExit("Wheel-load normalization no longer keeps a four-wheel baseline")
if "FrontCrossAxleImbalance" not in cpp or "RearCrossAxleImbalance" not in cpp:
    raise SystemExit("Cross-axle imbalance is not calculated for both axles")
if "GroundedWheelCount <= 2" not in cpp:
    raise SystemExit("Severe two-contact steering protection is missing")
if "!bRollbackControl" not in cpp:
    raise SystemExit("Cross-axle control no longer yields to rollback control")
if "WheelLoadControlMaxKmh" not in cpp:
    raise SystemExit("Low/medium-speed ownership boundary missing")

for token in [
    "rut",
    "diagonal",
    "mud",
    "Heavy Timber Haul",
    "contact loss",
    "rollback",
    "26 km/h",
    "NATIVE_WHEEL_LOAD_EVIDENCE",
]:
    if token.lower() not in playtest.lower():
        raise SystemExit(f"0.0.61 playtest missing scenario/evidence: {token}")

if "Native Per-Wheel Load & Cross-Axle Control" not in changelog:
    raise SystemExit("0.0.61 changelog title missing")

if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
    raise SystemExit("SWIR roadmap style lock marker missing")
completed = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
remaining = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
if (completed, remaining, completed + remaining) != (125, 5, 130):
    raise SystemExit(f"Roadmap unexpectedly changed: {completed}/130 complete, {remaining} remaining")
if "DONE-125%2F130" not in roadmap or "96.2%" not in roadmap or "███████████████████░ 96.2%" not in roadmap:
    raise SystemExit("ROADMAP-PROGRESS dashboard is not synchronized with checklist")

print("Native per-wheel load / cross-axle control and roadmap honesty verified.")
