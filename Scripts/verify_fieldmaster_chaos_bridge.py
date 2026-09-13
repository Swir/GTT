from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")

bridge_h = read("Source/GTT/Public/Vehicles/GTTChaosVehicleBridgeComponent.h")
bridge_cpp = read("Source/GTT/Private/Vehicles/GTTChaosVehicleBridgeComponent.cpp")
tractor_h = read("Source/GTT/Public/Vehicles/GTTTractorPawn.h")
tractor_cpp = read("Source/GTT/Private/Vehicles/GTTTractorPawn.cpp")
spec_cpp = read("Source/GTT/Private/Vehicles/GTTChaosVehicleSpec.cpp")
roadmap = read("Docs/ROADMAP.md")
playtest = read("Docs/PLAYTEST_0.0.33.md")
changelog = read("CHANGELOG.md")
workflow = read(".github/workflows/project-sanity.yml")

for token in [
    "UGTTChaosVehicleBridgeComponent",
    "EGTTChaosBridgeState",
    "CaptureThrottleInput",
    "CaptureSteeringInput",
    "RefreshNativeBinding",
    "IsNativeMovementReady",
    "GetBridgeStatusSummary",
]:
    assert token in bridge_h, f"Chaos bridge header missing {token}"

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
    assert token in bridge_cpp, f"Chaos bridge runtime missing {token}"

assert "ChaosVehicleBridge" in tractor_h
assert "CreateDefaultSubobject<UGTTChaosVehicleBridgeComponent>" in tractor_cpp
assert 'BindAxis(TEXT("VehicleThrottle")' in tractor_cpp
assert 'BindAxis(TEXT("VehicleSteer")' in tractor_cpp
assert "CaptureChaosThrottle" in tractor_cpp and "CaptureChaosSteering" in tractor_cpp
assert 'PersistentVehicleId = TEXT("RustyFieldmaster60")' in tractor_cpp
assert 'TEXT("RustyFieldmaster60")' in spec_cpp

assert "0.0.33" in playtest and "fallback" in playtest.lower()
assert "no packaged EXE verification" in playtest
assert "[0.0.33]" in changelog and "Fieldmaster Chaos Runtime Bridge" in changelog
assert "Verify Fieldmaster Chaos runtime bridge" in workflow
assert "verify_fieldmaster_chaos_bridge.py" in workflow

# Source-only bridge is deliberately not enough to close native-runtime acceptance tasks.
for open_task in [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Full Win64 CI/build runner",
]:
    assert open_task in roadmap, f"runtime acceptance task closed prematurely: {open_task}"

assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "<!-- ROADMAP-PROGRESS:START -->" in roadmap and "<!-- ROADMAP-PROGRESS:END -->" in roadmap
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
uncheked = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + uncheked
assert total == 130, f"roadmap total drifted: {total}"
assert checked == 125, f"0.0.33 must not fake Native Chaos completion: {checked}/{total}"
percent = round(checked / total * 100, 1)
segments = round(checked / total * 20)
bar = "█" * segments + "░" * (20 - segments)
assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"| **{checked}** | **{uncheked}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert f"{bar} {percent:.1f}%" in roadmap

print(f"Fieldmaster Chaos bridge sanity OK: native input/state bridge + protected fallback; roadmap {checked}/{total} ({percent:.1f}%)")
