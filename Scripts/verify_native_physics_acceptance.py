from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTNativePhysicsAcceptanceSubsystem.h").read_text(encoding="utf-8")
cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTNativePhysicsAcceptanceSubsystem.cpp").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.54.md").read_text(encoding="utf-8")

for token in ["UTickableWorldSubsystem", "ValidateAuthoredPhysics", "SampleGroundContacts", "FAcceptanceState"]:
    assert token in header, f"missing native physics acceptance header token: {token}"

for token in [
    "NATIVE_PHYSICS_EVIDENCE",
    "NATIVE_PHYSICS_FALLBACK",
    "SkeletalBodySetups.Num()",
    "FindBodyIndex(Rig.RootBone)",
    "GetCollisionEnabled()",
    "FrontTrack",
    "RearTrack",
    "Wheelbase",
    "LineTraceSingleByChannel",
    "contacts=%d/4",
    "DeactivateLegacyTakeover()",
]:
    assert token in cpp, f"missing native physics acceptance implementation token: {token}"

assert "python Scripts/verify_native_physics_acceptance.py" in workflow
assert "0.0.54" in playtest
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "DONE-125%2F130" in roadmap
assert "96.2%" in roadmap
assert roadmap.count("- [ ]") == 5, "runtime acceptance instrumentation must not falsely close UE-runtime roadmap items"

print("Native Physics acceptance milestone verified")
