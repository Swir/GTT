#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h"
LEGACY_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTVehicleBase.cpp"
RUNTIME_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawnRuntime.cpp"
DYNAMICS_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTNativeDriveDynamicsSubsystem.cpp"
ROADMAP = ROOT / "Docs/ROADMAP.md"

errors: list[str] = []
for path in (HEADER, LEGACY_CPP, RUNTIME_CPP, DYNAMICS_CPP, ROADMAP):
    if not path.is_file():
        errors.append(f"missing required file: {path.relative_to(ROOT)}")

if errors:
    print("\n".join(f"ERROR: {item}" for item in errors))
    sys.exit(1)

header = HEADER.read_text(encoding="utf-8")
legacy_cpp = LEGACY_CPP.read_text(encoding="utf-8")
runtime_cpp = RUNTIME_CPP.read_text(encoding="utf-8")
dynamics_cpp = DYNAMICS_CPP.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")

# The migration contract and legacy source both define vehicle condition as a 0..1 ratio.
for token in (
    "0.0 = broken, 1.0 = healthy",
    "meta=(ClampMin=\"0.0\", ClampMax=\"1.0\")",
):
    if token not in header:
        errors.append(f"Fieldmaster migration ratio contract missing token: {token}")

if "return MaxCondition > 0.0f ? Condition / MaxCondition : 0.0f;" not in legacy_cpp:
    errors.append("legacy GetConditionPercent() is no longer verified as a 0..1 ratio")

required_dynamics = (
    "constexpr float CriticalConditionRatio = 0.08f;",
    "State.ConditionPercent > CriticalConditionRatio",
    "const float ConditionAlpha = FMath::Clamp(State.ConditionPercent, 0.0f, 1.0f);",
    "State.ConditionPercent <= CriticalConditionRatio",
    "condition_pct=%.1f",
    "State.ConditionPercent * 100.0f",
)
for token in required_dynamics:
    if token not in dynamics_cpp:
        errors.append(f"native dynamics normalized-condition contract missing token: {token}")

for forbidden in (
    "CriticalConditionPercent = 8.0f",
    "State.ConditionPercent / 100.0f",
    "State.ConditionPercent > CriticalConditionPercent",
    "State.ConditionPercent <= CriticalConditionPercent",
):
    if forbidden in dynamics_cpp:
        errors.append(f"stale 0..100 Fieldmaster condition math returned: {forbidden}")

if not re.search(
    r"MigrationSnapshot\.ConditionPercent\s*=\s*FMath::Clamp\(\s*"
    r"MigrationSnapshot\.ConditionPercent\s*-\s*ConditionLoss,\s*0\.0f,\s*1\.0f\)",
    runtime_cpp,
    flags=re.MULTILINE,
):
    errors.append("native impact damage no longer clamps Fieldmaster condition to the 0..1 runtime contract")

checked = len(re.findall(r"^\s*- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^\s*- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + open_items
if (checked, total) != (125, 130):
    errors.append(f"roadmap checklist changed unexpectedly: {checked}/{total}; expected 125/130")
if roadmap.count("../assets/readme/progress-mini.svg") != 1:
    errors.append("roadmap must embed exactly one progress-mini.svg")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    errors.append("legacy text/Unicode roadmap progress meter must not return")

if errors:
    print("Fieldmaster native condition-scale verification FAILED")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print("Fieldmaster native condition-scale verification OK")
print(" - migration and legacy condition are consistently treated as a 0..1 ratio")
print(" - 8% critical threshold is represented as 0.08 in Native Chaos authority")
print(" - healthy Fieldmaster drivetrain authority can no longer be rejected by a 0..100 scale mismatch")
print(f" - roadmap remains honest at {checked}/{total} ({checked / total * 100:.1f}%)")
