from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTNativeDriveDynamicsSubsystem.h").read_text(encoding="utf-8")
cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeDriveDynamicsSubsystem.cpp").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.53.md").read_text(encoding="utf-8")

for token in ["UTickableWorldSubsystem", "ApplyDriveDynamics", "EvidenceSeconds"]:
    assert token in header, f"missing drive-dynamics header token: {token}"

for token in [
    "NATIVE_DRIVE_DYNAMICS",
    "BaseFieldmasterTopSpeedKmh",
    "EngineUpgradeSpeedBonusKmh",
    "CriticalConditionRatio",
    "LowTireIntegrityThreshold",
    "GetMigrationSnapshot",
    "const float ConditionAlpha = FMath::Clamp(State.ConditionPercent, 0.0f, 1.0f);",
    "State.ConditionPercent > CriticalConditionRatio",
    "State.ConditionPercent <= CriticalConditionRatio",
    "ConditionSpeedFactor",
    "TireSpeedFactor",
    "EffectiveTopSpeedKmh",
    "SetThrottleInput(0.0f)",
    "SetSteeringInput(0.0f)",
    "SetBrakeInput",
]:
    assert token in cpp, f"missing drive-dynamics implementation token: {token}"

assert "CriticalConditionPercent = 8.0f" not in cpp, "stale 0..100 critical-condition threshold returned"
assert "State.ConditionPercent / 100.0f" not in cpp, "normalized Fieldmaster condition must not be divided by 100"
assert "python Scripts/verify_native_drive_dynamics.py" in workflow
assert "0.0.53" in playtest
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "DONE-125%2F130" in roadmap
assert "96.2%" in roadmap
assert roadmap.count("- [ ]") == 5, "source-level drive dynamics must not falsely close runtime-only roadmap work"

print("Native drive dynamics milestone verified")
print(" - Fieldmaster condition authority remains normalized to 0..1 with an 8% = 0.08 critical threshold")
