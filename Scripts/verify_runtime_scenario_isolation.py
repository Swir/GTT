#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
read = lambda p: (ROOT / p).read_text(encoding="utf-8")
smoke = read("Scripts/smoke_test_windows.ps1")
drivetrain = read("Source/GTT/Private/Core/GTTDrivetrainEvidenceScenarioSubsystem.cpp")
trailer = read("Source/GTT/Private/Core/GTTTrailerEvidenceScenarioSubsystem.cpp")
recovery = read("Source/GTT/Private/Vehicles/GTTRecoveryChoiceEvidenceSubsystem.cpp")
fieldmaster = read("Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp")
road_vehicle = read("Source/GTT/Private/Vehicles/GTTRoadVehicleNativePawn.cpp")
physical = read("Scripts/verify_physical_roadblock_crossing.py")
attestor = read("Scripts/write_win64_candidate_attestation.ps1")
workflow = read(".github/workflows/gtt-v0.1.61-win64-attested-candidate.yml")
roadmap = read("Docs/ROADMAP.md")

errors = []
for token in [
    "Invoke-GTTRuntimePass", "-Name 'CORE'", "-Name 'NATIVE'", "-Name 'SERVICES'",
    "GTT_RUNTIME_CORE.log", "GTT_RUNTIME_NATIVE.log", "GTT_RUNTIME_SERVICES.log",
    "return [pscustomobject][ordered]@{", "Measure-Object -Property survived_seconds -Sum",
    "-UserDir=$userDir", "isolated_user_dirs = $true", "runtime_passes = $passes",
    "GTTDisableDrivetrainScenario", "GTTDisableTrailerScenario", "GTTDisableRecoveryChoiceScenario",
    "GTTDrivetrainRuntimeScenario", "GTTTrailerRuntimeScenario",
    "GTTFarmCargoRuntimeScenario", "GTTWorkshopPriorityPickupRuntimeScenario",
]:
    if token not in smoke:
        errors.append(f"smoke isolation contract missing: {token}")

for token in ["GTTDrivetrainRuntimeScenario", "GTTDisableDrivetrainScenario"]:
    if token not in drivetrain:
        errors.append(f"drivetrain activation contract missing: {token}")
for token in ["GTTTrailerRuntimeScenario", "GTTDisableTrailerScenario"]:
    if token not in trailer:
        errors.append(f"trailer activation contract missing: {token}")
for token in ["GTTRecoveryChoiceRuntimeScenario", "GTTDisableRecoveryChoiceScenario"]:
    if token not in recovery:
        errors.append(f"recovery activation contract missing: {token}")
for source_name, source in [("Fieldmaster", fieldmaster), ("road vehicle", road_vehicle)]:
    for token in ["GTTDemoSmokeScenario", "GTTDrivetrainRuntimeScenario", "GTTTrailerRuntimeScenario"]:
        if token not in source:
            errors.append(f"{source_name} acceptance guard missing: {token}")

for token in ["GTT_RUNTIME_CORE.log", "GTT_RUNTIME_NATIVE.log", "GTT_RUNTIME_SERVICES.log"]:
    if token not in attestor:
        errors.append(f"sealed attestor missing isolated runtime log: {token}")
if "GTT_RUNTIME*.log" not in workflow:
    errors.append("candidate failure diagnostics do not preserve all isolated runtime logs")

if "SetActorTransform" not in physical:
    errors.append("physical roadblock verifier is not aligned with transform-based staging")
if "'SetActorLocation','SetActorRotation'" in physical:
    errors.append("obsolete split location/rotation verifier expectation returned")

core = re.search(r"\$corePass\s*=\s*Invoke-GTTRuntimePass.*?\)\s*\n\$nativePass", smoke, flags=re.S)
native = re.search(r"\$nativePass\s*=\s*Invoke-GTTRuntimePass.*?\)\s*\n\$servicesPass", smoke, flags=re.S)
services = re.search(r"\$servicesPass\s*=\s*Invoke-GTTRuntimePass.*?\)\s*\n\$passes", smoke, flags=re.S)
if not core or not all(x in core.group(0) for x in ["GTTDemoSmokeScenario","GTTDisableDrivetrainScenario","GTTDisableTrailerScenario","GTTDisableRecoveryChoiceScenario"]):
    errors.append("CORE pass is not isolated from independent destructive scenarios")
if not native or not all(x in native.group(0) for x in ["GTTDrivetrainRuntimeScenario","GTTTrailerRuntimeScenario"]):
    errors.append("NATIVE pass does not own drivetrain+trailer evidence")
if native and "GTTDemoSmokeScenario" in native.group(0):
    errors.append("NATIVE pass must not start the destructive core chain")
if not services or "GTTDemoSmokeScenario" in services.group(0):
    errors.append("SERVICES pass must be isolated from the core chain")
if "-RequiredAliveSeconds 180" not in smoke:
    errors.append("CORE/NATIVE 180-second acceptance window missing")
if "-RequiredAliveSeconds $MinimumAliveSeconds" not in smoke:
    errors.append("SERVICES canonical long runtime window missing")

items = re.findall(r"^\s*- \[(x|X| )\] ", roadmap, flags=re.M)
done = sum(1 for value in items if value.lower() == "x")
total = len(items)
if (done, total) != (125, 130) or "96.2%" not in roadmap:
    errors.append(f"roadmap truth drifted during isolation: {done}/{total}")

if errors:
    print("GTT runtime scenario isolation audit FAILED")
    for error in errors:
        print(" -", error)
    raise SystemExit(1)

print("[GTT][AUDIT][PASS] packaged evidence isolated: CORE / NATIVE / SERVICES + clean per-pass UserDir; roadmap 125/130 = 96.2%")
