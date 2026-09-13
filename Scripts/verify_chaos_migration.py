from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")

uproject = read("GTT.uproject")
build = read("Source/GTT/GTT.Build.cs")
spec_h = read("Source/GTT/Public/Vehicles/GTTChaosVehicleSpec.h")
spec_cpp = read("Source/GTT/Private/Vehicles/GTTChaosVehicleSpec.cpp")
tractor = read("Source/GTT/Private/Vehicles/GTTTractorPawn.cpp")
old_car = read("Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp")
van = read("Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp")
doc = read("Docs/CHAOS_VEHICLE_MIGRATION.md")
playtest = read("Docs/PLAYTEST_0.0.30.md")
roadmap = read("Docs/ROADMAP.md")
changelog = read("CHANGELOG.md")
workflow = read(".github/workflows/project-sanity.yml")

assert '"Name": "ChaosVehiclesPlugin"' in uproject and '"Enabled": true' in uproject
assert '"ChaosVehicles"' in build, "ChaosVehicles module dependency missing"
assert "FGTTChaosWheelSpec" in spec_h and "FGTTChaosVehicleSpec" in spec_h
assert "EGTTChaosDriveLayout" in spec_h and "FourWheelDrive" in spec_h

for vehicle_id, source in [
    ("RustyFieldmaster60", tractor),
    ("Rattleback82", old_car),
    ("Mulebox1200", van),
]:
    assert f'PersistentVehicleId = TEXT("{vehicle_id}")' in source, f"vehicle id drift: {vehicle_id}"
    assert f'TEXT("{vehicle_id}")' in spec_cpp, f"Chaos profile missing persistent id {vehicle_id}"

for factory in ["GetFieldmaster60Spec", "GetRattleback82Spec", "GetMulebox1200Spec", "GetSpecForVehicleId"]:
    assert factory in spec_h and factory in spec_cpp, f"missing Chaos profile factory {factory}"

for token in ["EngineMaxTorqueNm", "EngineMaxRpm", "FinalDriveRatio", "ForwardGearRatios", "SuspensionSpringRate", "FrictionForceMultiplier"]:
    assert token in spec_h, f"Chaos contract missing {token}"

assert "AWheeledVehiclePawn" in doc
assert "UChaosWheeledVehicleMovementComponent" in doc
assert "skeletal mesh" in doc.lower() and "physics asset" in doc.lower() and "wheel" in doc.lower()
assert "not complete" in doc.lower() or "not completed" in doc.lower()
assert "Full Unreal Engine 5.8" in playtest and "no packaged EXE verification" in playtest
assert "[0.0.30]" in changelog and "Chaos Migration Foundation" in changelog
assert "Verify Chaos migration foundation" in workflow and "verify_chaos_migration.py" in workflow

# This milestone intentionally does not check off Native Chaos without authored skeletal/physics assets and UE runtime validation.
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
unchecked = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + unchecked
assert (checked, total) == (123, 130), f"roadmap unexpectedly changed: {checked}/{total}"
percent = round(checked / total * 100, 1)
segments = round(checked / total * 20)
bar = "█" * segments + "░" * (20 - segments)
assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"| **{checked}** | **{unchecked}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert f"{bar} {percent:.1f}%" in roadmap

print(f"Chaos migration foundation sanity OK: 3 canonical vehicle specs; roadmap remains {checked}/{total} ({percent:.1f}%)")
