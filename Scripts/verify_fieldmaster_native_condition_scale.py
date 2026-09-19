#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h"
LEGACY_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTVehicleBase.cpp"
CORE_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp"
ENVIRONMENT_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativeEnvironment.cpp"
OBSOLETE_RUNTIME_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawnRuntime.cpp"
DYNAMICS_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTNativeDriveDynamicsSubsystem.cpp"
ROADMAP = ROOT / "Docs/ROADMAP.md"

errors: list[str] = []
for path in (HEADER, LEGACY_CPP, CORE_CPP, ENVIRONMENT_CPP, DYNAMICS_CPP, ROADMAP):
    if not path.is_file():
        errors.append(f"missing required file: {path.relative_to(ROOT)}")

if OBSOLETE_RUNTIME_CPP.exists():
    errors.append("obsolete duplicate Fieldmaster runtime translation unit must remain removed")

if errors:
    print("\n".join(f"ERROR: {item}" for item in errors))
    sys.exit(1)

header = HEADER.read_text(encoding="utf-8")
legacy_cpp = LEGACY_CPP.read_text(encoding="utf-8")
core_cpp = CORE_CPP.read_text(encoding="utf-8")
environment_cpp = ENVIRONMENT_CPP.read_text(encoding="utf-8")
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
    r"MigrationSnapshot\.ConditionPercent\s*-\s*BodyDamageRatio,\s*0\.0f,\s*1\.0f\)",
    environment_cpp,
    flags=re.MULTILINE,
):
    errors.append("native impact damage no longer clamps Fieldmaster condition to the 0..1 runtime contract")

# Import is sourced from the normalized legacy getter, but ApplyMigrationSnapshot is also public.
# Keep that public boundary fail-safe for old callers that may still submit 0..100 values while
# guaranteeing that all stored native state is canonical 0..1 before drivetrain authority sees it.
required_core_tokens = (
    "Snapshot.ConditionPercent = LegacyVehicle->GetConditionPercent();",
    "const float RawCondition = Snapshot.ConditionPercent;",
    "const float NormalizedCondition = RawCondition > 1.0f ? RawCondition / 100.0f : RawCondition;",
    "MigrationSnapshot.ConditionPercent = FMath::Clamp(NormalizedCondition, 0.0f, 1.0f);",
    "MigrationSnapshot.EngineUpgradeLevel = FMath::Clamp(Snapshot.EngineUpgradeLevel, 0, 3);",
    "MigrationSnapshot.TireUpgradeLevel = FMath::Clamp(Snapshot.TireUpgradeLevel, 0, 3);",
    "MigrationSnapshot.ConditionPercent * 100.0f, MigrationSnapshot.FuelLiters",
)
for token in required_core_tokens:
    if token not in core_cpp:
        errors.append(f"Fieldmaster migration-boundary contract missing token: {token}")

for forbidden in (
    "FMath::Clamp(Snapshot.ConditionPercent, 0.0f, 100.0f)",
    "*FieldmasterVehicleId.ToString(), MigrationSnapshot.ConditionPercent, MigrationSnapshot.FuelLiters",
    "MigrationSnapshot.EngineUpgradeLevel = FMath::Max(0, Snapshot.EngineUpgradeLevel)",
    "MigrationSnapshot.TireUpgradeLevel = FMath::Max(0, Snapshot.TireUpgradeLevel)",
):
    if forbidden in core_cpp:
        errors.append(f"stale Fieldmaster migration-boundary behavior returned: {forbidden}")

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
print(" - migration and legacy condition are consistently stored as a 0..1 ratio")
print(" - public migration ingress safely normalizes legacy 0..100 values before native authority")
print(" - engine/tire upgrade levels are bounded to the supported 0..3 range")
print(" - 8% critical threshold is represented as 0.08 in Native Chaos authority")
print(" - canonical environment translation unit owns normalized impact damage")
print(" - obsolete duplicate runtime translation unit is absent")
print(f" - roadmap remains honest at {checked}/{total} ({checked / total * 100:.1f}%)")
