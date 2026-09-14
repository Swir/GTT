from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTNativeStabilitySubsystem.h").read_text(encoding="utf-8")
cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeStabilitySubsystem.cpp").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.55.md").read_text(encoding="utf-8")

for token in ["UTickableWorldSubsystem", "FStabilityState", "EvaluateFieldmaster", "ComputeStabilityRisk"]:
    assert token in header, f"missing native stability header token: {token}"

for token in [
    "NATIVE_STABILITY_EVIDENCE",
    "SampleWheelContacts",
    "LowContactGraceSeconds",
    "SevereRollDegrees",
    "SeverePitchDegrees",
    "SetThrottleInput(0.0f)",
    "SetBrakeInput",
    "SetSteeringInput(0.0f)",
    "GetMigrationSnapshot",
    "TireUpgradeLevel",
    "LineTraceSingleByChannel",
    "contacts=%d/4",
]:
    assert token in cpp, f"missing native stability implementation token: {token}"

assert "python Scripts/verify_native_stability.py" in workflow
assert "0.0.55" in playtest
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "DONE-125%2F130" in roadmap
assert "96.2%" in roadmap
assert roadmap.count("- [ ]") == 5, "stability source milestone must not falsely close UE-runtime roadmap items"

print("Native stability and contact-control milestone verified")
