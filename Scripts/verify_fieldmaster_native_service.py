from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
service = (ROOT / "Source/GTT/Private/World/GTTServiceTerminal.cpp").read_text(encoding="utf-8")
tuning = (ROOT / "Source/GTT/Private/World/GTTTuningTerminal.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.46.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.46.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")

for token in [
    "GTTFieldmasterNativePawn.h",
    "FindActiveNativeFieldmaster",
    "IsLegacyTakeoverActive",
    "FindFieldmasterMirror",
    "GetMigrationSnapshot",
    "RepairVehicle(100000.0f)",
    "RefuelVehicle(100000.0f)",
    "ImportLegacyGameplayState",
    "Workshop service rollback",
]:
    assert token in service, f"native workshop integration missing {token}"

for token in [
    "GTTFieldmasterNativePawn.h",
    "FindActiveNativeFieldmaster",
    "FindFieldmasterMirror",
    "RefreshNativeFromMirror",
    "RepairTires",
    "InstallEngineUpgrade",
    "InstallTireUpgrade",
    "Engine tune rollback",
    "Tire upgrade rollback",
    "GameMode->SaveProgress()",
]:
    assert token in tuning, f"native tuning integration missing {token}"

# The legacy Fieldmaster mirror must not remain independently serviceable while takeover is active.
assert "GetPersistentVehicleId() == FieldmasterVehicleId" in service
assert "GetPersistentVehicleId() == FieldmasterVehicleId" in tuning
assert "bNativeTakeoverActive" in service
assert "bNativeTakeoverActive" in tuning

# SWIR Roadmap Style Lock v1 + exact current progress. This milestone must not fake runtime acceptance.
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "ROADMAP-96.2%25" in roadmap
assert "DONE-125%2F130" in roadmap
assert "| **125** | **5** | **130** | **96.2%** |" in roadmap
assert "███████████████████░ 96.2%" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap
assert "- [ ] Full Win64 CI/build runner" in roadmap

assert "Native Fieldmaster Workshop & Tuning Integration" in playtest
assert "mirror sync" in playtest.lower()
assert "DEMO / Win64 gate" in playtest
assert "Native Fieldmaster Workshop & Tuning Integration" in changelog
assert "125/130 (96.2%)" in changelog
assert "Verify Fieldmaster native workshop and tuning" in workflow

print("Fieldmaster native workshop/tuning sanity passed; roadmap remains 125/130 (96.2%)")
