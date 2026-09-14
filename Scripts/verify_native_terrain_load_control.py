from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / "Source/GTT/Public/Vehicles/GTTNativeTerrainLoadSubsystem.h").read_text(encoding="utf-8")
cpp = (root / "Source/GTT/Private/Vehicles/GTTNativeTerrainLoadSubsystem.cpp").read_text(encoding="utf-8")
roadmap = (root / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (root / "Docs/PLAYTEST_0.0.60.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.d/0.0.60.md").read_text(encoding="utf-8")

for token in [
    "UGTTNativeTerrainLoadSubsystem",
    "FTerrainLoadState",
    "SampleAxleClearance",
    "FindAttachedTrailer",
    "RollbackSeconds",
]:
    if token not in header:
        raise SystemExit(f"Native terrain-load header missing token: {token}")

for token in [
    "GetNativeMudSeverity()",
    "GetNativeTerrainGripFactor()",
    "GetTowLoadFactor()",
    "FrontLeftWheelBone",
    "RearRightWheelBone",
    "rear_load_bias",
    "launch_grip",
    "NATIVE_TERRAIN_LOAD_EVIDENCE",
    "Movement->SetThrottleInput",
    "Movement->SetBrakeInput",
    "RollbackGraceSeconds",
]:
    if token not in cpp:
        raise SystemExit(f"Native terrain-load implementation missing token: {token}")

if "bLaunchControl" not in cpp or "bRollbackControl" not in cpp:
    raise SystemExit("Hill-start and rollback control are not both implemented")
if "TowLoad * 0.42f" not in cpp:
    raise SystemExit("Trailer tongue load no longer feeds rear-load bias")
if "TerrainGrip - FrontUnloadPenalty" not in cpp:
    raise SystemExit("Authored terrain grip no longer feeds launch-grip calculation")

for token in ["mud", "hill", "rollback", "trailer", "NATIVE_TERRAIN_LOAD_EVIDENCE"]:
    if token.lower() not in playtest.lower():
        raise SystemExit(f"0.0.60 playtest missing scenario/evidence: {token}")

if "Native Terrain-Load & Hill Control" not in changelog:
    raise SystemExit("0.0.60 changelog title missing")

if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
    raise SystemExit("SWIR roadmap style lock marker missing")
completed = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
remaining = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
if (completed, remaining, completed + remaining) != (125, 5, 130):
    raise SystemExit(f"Roadmap unexpectedly changed: {completed}/130 complete, {remaining} remaining")
if "DONE-125%2F130" not in roadmap or "96.2%" not in roadmap or "███████████████████░ 96.2%" not in roadmap:
    raise SystemExit("ROADMAP-PROGRESS dashboard is not synchronized with checklist")

print("Native terrain-load / hill control and roadmap honesty verified.")
