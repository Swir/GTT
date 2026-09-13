from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")


bridge_h = read("Source/GTT/Public/Vehicles/GTTChaosVehicleBridgeComponent.h")
bridge_cpp = read("Source/GTT/Private/Vehicles/GTTChaosVehicleBridgeComponent.cpp")
rig_h = read("Source/GTT/Public/Vehicles/GTTChaosRigContract.h")
trailer_cpp = read("Source/GTT/Private/Vehicles/GTTFarmTrailer.cpp")
roadmap = read("Docs/ROADMAP.md")
playtest = read("Docs/PLAYTEST_0.0.36.md")
workflow = read(".github/workflows/project-sanity.yml")

for token in [
    "ResolveRigContract", "ValidateNativeRig", "IsNativeRigContractValid",
    "GetResolvedRigContract", "GetRigValidationSummary", "TryGetNativeHitchTransform",
    "NativeSkeletalBody", "bRigContractValid", "RigValidationSummary",
]:
    assert token in bridge_h + bridge_cpp, f"native rig acceptance bridge missing {token}"

for token in [
    "GetRequiredBoneNames", "GetRequiredSocketNames", "GetBoneIndex",
    "DoesSocketExist", "INDEX_NONE", "MissingBones", "MissingSockets",
]:
    assert token in bridge_cpp + rig_h, f"rig validation missing {token}"

assert "bNativeMovementReady = NativeMovement != nullptr && bHasSpec && bRigValid" in bridge_cpp
assert "DisableLegacyDynamicsIfNeeded" in bridge_cpp
assert bridge_cpp.index("bNativeMovementReady = NativeMovement != nullptr && bHasSpec && bRigValid") < bridge_cpp.index("DisableLegacyDynamicsIfNeeded();")

for token in [
    "FindComponentByClass<UGTTChaosVehicleBridgeComponent>",
    "TryGetNativeHitchTransform", "NativeHitchTransform.GetLocation()",
    "HitchConstraint->SetWorldLocation(HitchLocation)",
]:
    assert token in trailer_cpp, f"trailer native hitch integration missing {token}"

assert "0.0.36" in playtest and "Native Rig Acceptance" in playtest
assert "no packaged exe verification" in playtest.lower()
assert "Verify native rig acceptance gate" in workflow
assert "verify_native_rig_acceptance.py" in workflow

# Runtime contract validation is not a substitute for authored assets + real UE runtime acceptance.
for open_task in [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Full Win64 CI/build runner",
]:
    assert open_task in roadmap, f"acceptance task closed prematurely: {open_task}"

assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
uncheck = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + uncheck
assert (checked, total) == (125, 130), f"0.0.36 must keep roadmap honest: {checked}/{total}"
percent = round(checked / total * 100, 1)
segments = round(checked / total * 20)
bar = "█" * segments + "░" * (20 - segments)
assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"| **{checked}** | **{uncheck}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert f"{bar} {percent:.1f}%" in roadmap

print(
    "Native rig acceptance sanity OK: bone/socket validation gates Chaos takeover; "
    f"trailer consumes validated rear_hitch; roadmap {checked}/{total} ({percent:.1f}%)"
)
