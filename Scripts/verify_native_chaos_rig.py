from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")

wheels_h = read("Source/GTT/Public/Vehicles/GTTChaosVehicleWheels.h")
wheels_cpp = read("Source/GTT/Private/Vehicles/GTTChaosVehicleWheels.cpp")
rig_h = read("Source/GTT/Public/Vehicles/GTTChaosRigContract.h")
rig_cpp = read("Source/GTT/Private/Vehicles/GTTChaosRigContract.cpp")
spec_h = read("Source/GTT/Public/Vehicles/GTTChaosVehicleSpec.h")
roadmap = read("Docs/ROADMAP.md")
playtest = read("Docs/PLAYTEST_0.0.35.md")
workflow = read(".github/workflows/project-sanity.yml")

wheel_classes = [
    "UGTTFieldmasterFrontWheel", "UGTTFieldmasterRearWheel",
    "UGTTRattlebackFrontWheel", "UGTTRattlebackRearWheel",
    "UGTTMuleboxFrontWheel", "UGTTMuleboxRearWheel",
]
for klass in wheel_classes:
    assert klass in wheels_h, f"missing native wheel class {klass}"
    assert f"{klass}::{klass}()" in wheels_cpp, f"missing constructor for {klass}"

for token in [
    "UChaosVehicleWheel", "WheelRadius", "WheelWidth", "SuspensionMaxRaise",
    "SuspensionMaxDrop", "SpringRate", "SuspensionDampingRatio",
    "FrictionForceMultiplier", "bAffectedBySteering", "bAffectedByEngine",
    "bAffectedByBrake", "bAffectedByHandbrake", "MaxSteerAngle",
]:
    assert token in wheels_h + wheels_cpp, f"native wheel implementation missing {token}"

for getter in ["GetFieldmaster60Spec", "GetRattleback82Spec", "GetMulebox1200Spec"]:
    assert getter in wheels_cpp, f"wheels are not consuming canonical spec: {getter}"
    assert getter in spec_h

for vehicle_id in ["RustyFieldmaster60", "Rattleback82", "Mulebox1200"]:
    assert vehicle_id in rig_cpp, f"rig contract missing {vehicle_id}"

for name in ["root", "wheel_fl", "wheel_fr", "wheel_rl", "wheel_rr", "driver_seat", "driver_exit", "rear_hitch"]:
    assert name in rig_h + rig_cpp, f"rig contract missing canonical name {name}"

assert 'MakeRig(TEXT("RustyFieldmaster60"), true)' in rig_cpp
assert 'MakeRig(TEXT("Rattleback82"), false)' in rig_cpp
assert 'MakeRig(TEXT("Mulebox1200"), true)' in rig_cpp
assert "GetRequiredBoneNames" in rig_h and "GetRequiredSocketNames" in rig_h

assert "0.0.35" in playtest and "Native Rig Architecture" in playtest
assert "no packaged EXE verification" in playtest
assert "Verify Native Chaos rig architecture" in workflow
assert "verify_native_chaos_rig.py" in workflow

# Architecture is not runtime acceptance. Keep every release-critical Native Chaos/Win64 task open.
for open_task in [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Full Win64 CI/build runner",
]:
    assert open_task in roadmap, f"acceptance task closed prematurely: {open_task}"

for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
):
    assert token in roadmap, f"roadmap SVG-only presentation missing {token}"
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
uncheck = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + uncheck
assert (checked, total) == (125, 130), f"0.0.35 architecture must keep roadmap honest: {checked}/{total}"
percent = round(checked / total * 100, 1)
assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"| **{checked}** | **{uncheck}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE), "legacy text/Unicode roadmap progress meter must not return"

print(f"Native Chaos rig architecture sanity OK: 6 wheel classes, 3 rig contracts; roadmap {checked}/{total} ({percent:.1f}%), SVG-only progress verified")
