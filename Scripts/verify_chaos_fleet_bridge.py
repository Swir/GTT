from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")

bridge_h = read("Source/GTT/Public/Vehicles/GTTChaosVehicleBridgeComponent.h")
bridge_cpp = read("Source/GTT/Private/Vehicles/GTTChaosVehicleBridgeComponent.cpp")
spec_cpp = read("Source/GTT/Private/Vehicles/GTTChaosVehicleSpec.cpp")
tractor_h = read("Source/GTT/Public/Vehicles/GTTTractorPawn.h")
tractor_cpp = read("Source/GTT/Private/Vehicles/GTTTractorPawn.cpp")
car_h = read("Source/GTT/Public/Vehicles/GTTOldCarPawn.h")
car_cpp = read("Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp")
van_h = read("Source/GTT/Public/Vehicles/GTTFarmVanPawn.h")
van_cpp = read("Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp")
roadmap = read("Docs/ROADMAP.md")
playtest = read("Docs/PLAYTEST_0.0.34.md")
changelog = read("CHANGELOG.md")
workflow = read(".github/workflows/project-sanity.yml")

for vehicle_id in ["RustyFieldmaster60", "Rattleback82", "Mulebox1200"]:
    assert f'TEXT("{vehicle_id}")' in spec_cpp, f"missing canonical Chaos spec for {vehicle_id}"

for label, header, source, klass in [
    ("Fieldmaster", tractor_h, tractor_cpp, "AGTTTractorPawn"),
    ("Rattleback", car_h, car_cpp, "AGTTOldCarPawn"),
    ("Mulebox", van_h, van_cpp, "AGTTFarmVanPawn"),
]:
    assert "UGTTChaosVehicleBridgeComponent" in header, f"{label} header missing bridge"
    assert "ChaosVehicleBridge" in header, f"{label} header missing bridge property"
    assert "SetupPlayerInputComponent" in header, f"{label} header missing input override"
    assert "CaptureChaosThrottle" in header and "CaptureChaosSteering" in header
    assert "CreateDefaultSubobject<UGTTChaosVehicleBridgeComponent>" in source, f"{label} does not construct bridge"
    assert 'BindAxis(TEXT("VehicleThrottle")' in source, f"{label} missing throttle mirror"
    assert 'BindAxis(TEXT("VehicleSteer")' in source, f"{label} missing steering mirror"
    assert "CaptureThrottleInput" in source and "CaptureSteeringInput" in source
    assert "Super::SetupPlayerInputComponent" in source, f"{label} must preserve legacy/base controls"

for token in [
    "UChaosWheeledVehicleMovementComponent",
    "USkeletalMeshComponent",
    "GetSpecForVehicleId",
    "SetThrottleInput",
    "SetSteeringInput",
    "SetBrakeInput",
    "SetTargetGear",
    "SetComponentTickEnabled(false)",
    "GetConditionPercent",
    "GetEngineUpgradeLevel",
    "GetTireIntegrity",
    "GetTireUpgradeLevel",
    "GetFuelLiters",
    "IsEngineRunning",
]:
    assert token in bridge_cpp, f"shared Chaos bridge missing {token}"

assert "0.0.34" in playtest
for vehicle_name in ["Rusty Fieldmaster 60", "Rattleback 82", "Mulebox 1200"]:
    assert vehicle_name in playtest, f"playtest missing {vehicle_name}"
assert "no packaged exe verification" in playtest.lower()
assert "[0.0.34]" in changelog and "Chaos Fleet Runtime Bridge" in changelog
assert "Verify Chaos fleet runtime bridge" in workflow
assert "verify_chaos_fleet_bridge.py" in workflow

# Fleet bridge readiness is not equivalent to authored wheel/suspension assets or runtime acceptance.
for open_task in [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Full Win64 CI/build runner",
]:
    assert open_task in roadmap, f"runtime acceptance task closed prematurely: {open_task}"

for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
):
    assert token in roadmap, f"roadmap SVG-only presentation missing {token}"
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
uncheked = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + uncheked
assert total == 130, f"roadmap total drifted: {total}"
assert checked == 125, f"0.0.34 must not fake Native Chaos completion: {checked}/{total}"
percent = round(checked / total * 100, 1)
assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"| **{checked}** | **{uncheked}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE), "legacy text/Unicode roadmap progress meter must not return"

print(f"Chaos fleet bridge sanity OK: 3/3 owned vehicles wired with protected fallback; roadmap {checked}/{total} ({percent:.1f}%), SVG-only progress verified")
