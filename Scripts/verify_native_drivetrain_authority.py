#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTNativeDriveDynamicsSubsystem.cpp"
HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTNativeDriveDynamicsSubsystem.h"
WORKFLOW = ROOT / ".github/workflows/project-sanity.yml"
ROADMAP = ROOT / "Docs/ROADMAP.md"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.0.75.md"
CHANGELOG = ROOT / "CHANGELOG.d/0.0.75.md"

errors = []
for path in (CPP, HEADER, WORKFLOW, ROADMAP, PLAYTEST, CHANGELOG):
    if not path.exists():
        errors.append(f"missing required file: {path.relative_to(ROOT)}")

if errors:
    print("\n".join(f"ERROR: {error}" for error in errors))
    sys.exit(1)

cpp = CPP.read_text(encoding="utf-8")
header = HEADER.read_text(encoding="utf-8")
workflow = WORKFLOW.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")
playtest = PLAYTEST.read_text(encoding="utf-8")
changelog = CHANGELOG.read_text(encoding="utf-8")

required_cpp = [
    "TActorIterator<AGTTFieldmasterNativePawn>",
    "TActorIterator<AGTTRoadVehicleNativePawn>",
    "ApplyDrivetrainAuthority(",
    "DirectionShiftReleaseSpeedKmh",
    "Movement->SetTargetGear(Authority.StableDirection, true)",
    "Movement->SetThrottleInput(0.0f)",
    "StationaryHoldBrake",
    "NATIVE_DIRECTION_SHIFT_COMMIT",
    "NATIVE_DIRECTION_INTERLOCK",
    "NATIVE_DRIVETRAIN_AUTHORITY_EVIDENCE",
    "GetInputAxisValue(TEXT(\"VehicleThrottle\"))",
    "GetInputAxisValue(TEXT(\"VehicleSteer\"))",
]
for token in required_cpp:
    if token not in cpp:
        errors.append(f"drivetrain implementation missing token: {token}")

required_header = [
    "FGTTNativeDrivetrainAuthorityState",
    "StableDirection",
    "bDirectionInterlock",
    "bEngineBrakeActive",
    "TMap<TWeakObjectPtr<APawn>, FGTTNativeDrivetrainAuthorityState>",
]
for token in required_header:
    if token not in header:
        errors.append(f"drivetrain state contract missing token: {token}")

if "python Scripts/verify_native_drivetrain_authority.py" not in workflow:
    errors.append("Project sanity does not execute verify_native_drivetrain_authority.py")

for token in ("direction interlock", "engine braking", "hill hold", "Rattleback", "Mulebox", "Fieldmaster"):
    if token.lower() not in playtest.lower():
        errors.append(f"playtest missing coverage phrase: {token}")

if "0.0.75" not in changelog or "drivetrain" not in changelog.lower():
    errors.append("0.0.75 changelog does not describe drivetrain milestone")

if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
    errors.append("SWIR roadmap standard marker missing")

# Count only actual checklist rows, not explanatory inline examples.
checked = len(re.findall(r"^\s*- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^\s*- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + open_items
if (checked, total) != (125, 130):
    errors.append(f"roadmap checklist changed unexpectedly: {checked}/{total}; expected 125/130")

if "███████████████████░ 96.2%" not in roadmap:
    errors.append("roadmap 20-segment progress bar is stale or missing")

if errors:
    print("Native drivetrain authority verification FAILED")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Native drivetrain authority verification OK")
print(" - shared Fieldmaster/Rattleback/Mulebox final drivetrain safety authority present")
print(" - unsafe forward/reverse swaps are speed-gated with braking interlock")
print(" - neutral engine braking and low-speed hold are present")
print(" - runtime evidence and Project sanity coverage are wired")
print(f" - roadmap remains honest at {checked}/{total} ({checked / total * 100:.1f}%)")
