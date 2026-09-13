from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
environment = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativeEnvironment.cpp").read_text(encoding="utf-8")
mud = (ROOT / "Source/GTT/Private/World/GTTMudZone.cpp").read_text(encoding="utf-8")
trailer_h = (ROOT / "Source/GTT/Public/Vehicles/GTTFarmTrailer.h").read_text(encoding="utf-8")
trailer = (ROOT / "Source/GTT/Private/Vehicles/GTTFarmTrailer.cpp").read_text(encoding="utf-8")
haul_h = (ROOT / "Source/GTT/Public/Activities/GTTHeavyHaulDirector.h").read_text(encoding="utf-8")
haul = (ROOT / "Source/GTT/Private/Activities/GTTHeavyHaulDirector.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.47.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.47.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")

for token in [
    "ApplyNativeMudResponse",
    "ApplyNativeImpactDamage",
    "TryGetRearHitchTransform",
]:
    assert token in header, f"Native Fieldmaster integration API missing {token}"
    assert token in environment, f"Native Fieldmaster environment implementation missing {token}"

for token in [
    "TireUpgradeLevel",
    "TireIntegrity",
    "SetThrottleInput",
    "AddForce",
    "ConditionPercent",
    "HitchSocket",
    "DoesSocketExist",
    "RTS_World",
]:
    assert token in environment, f"Native environment behavior missing {token}"

for token in [
    "AGTTFieldmasterNativePawn::StaticClass()",
    "IsNativeFieldmasterReady",
    "IsLegacyTakeoverActive",
    "ApplyNativeMudResponse",
]:
    assert token in mud, f"authored mud volume native routing missing {token}"

for token in [
    "AttachToNativeFieldmaster",
    "NativeTowVehicle",
    "GetTowActor",
]:
    assert token in trailer_h, f"trailer native API missing {token}"
    assert token in trailer, f"trailer native implementation missing {token}"

for token in [
    "TryGetRearHitchTransform",
    "SetConstrainedComponents(VehicleMesh",
    "IsSimulatingPhysics",
    "BreakConstraint",
]:
    assert token in trailer, f"native hitch safety missing {token}"

for token in [
    "FindEligibleNativeTowVehicle",
    "ContractNativeTowVehicle",
    "GetContractTowConditionFactor",
]:
    assert token in haul_h, f"heavy-haul native contract API missing {token}"
    assert token in haul, f"heavy-haul native contract implementation missing {token}"

for token in [
    "AttachToNativeFieldmaster",
    "IsHidden()",
    "ConditionPercent / 100.0f",
    "GetCargoIntegrity",
    "GetTrailerIntegrity",
]:
    assert token in haul, f"heavy-haul gameplay connection missing {token}"

# SWIR Roadmap Style Lock v1 + exact current progress. 0.0.47 must not fake runtime acceptance.
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "ROADMAP-96.2%25" in roadmap
assert "DONE-125%2F130" in roadmap
assert "| **125** | **5** | **130** | **96.2%** |" in roadmap
assert "███████████████████░ 96.2%" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
assert "- [ ] Authored skeletal trailer wheel assets and final hitch sockets" in roadmap
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap
assert "- [ ] Full Win64 CI/build runner" in roadmap

assert "Native Fieldmaster Terrain & Heavy-Haul Integration" in playtest
assert "Native trailer hitch" in playtest
assert "DEMO / Win64 gate" in playtest
assert "Native Fieldmaster Terrain & Heavy-Haul Integration" in changelog
assert "125/130 (96.2%)" in changelog
assert "Verify Fieldmaster native terrain and heavy-haul" in workflow

print("Fieldmaster native terrain/heavy-haul sanity passed; roadmap remains 125/130 (96.2%)")
