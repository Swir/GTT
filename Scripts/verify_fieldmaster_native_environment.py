from pathlib import Path
import re

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
    "GetMigrationSnapshot().ConditionPercent, 0.40f, 1.0f",
    "GetCargoIntegrity",
    "GetTrailerIntegrity",
]:
    assert token in haul, f"heavy-haul gameplay connection missing {token}"
assert "GetMigrationSnapshot().ConditionPercent / 100.0f" not in haul, "heavy-haul must consume normalized Native Fieldmaster health directly"

# Keep the old milestone's gameplay assertions while validating the current
# SWIR Roadmap v1 structure using SVG-only progress presentation.
for token in [
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
    "ROADMAP-96.2%25",
    "DONE-125%2F130",
    "| **125** | **5** | **130** | **96.2%** |",
]:
    assert token in roadmap, f"roadmap SVG-only presentation missing {token}"

checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + open_items
assert (checked, open_items, total) == (125, 5, 130), (checked, open_items, total)
assert round(checked * 100.0 / total, 1) == 96.2
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(
    r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}",
    roadmap,
    flags=re.MULTILINE,
), "legacy text/Unicode roadmap progress meter must not return"

for checkbox in [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Full Win64 CI/build runner",
]:
    assert checkbox in roadmap

assert "Native Fieldmaster Terrain & Heavy-Haul Integration" in playtest
assert "Native trailer hitch" in playtest
assert "DEMO / Win64 gate" in playtest
assert "Native Fieldmaster Terrain & Heavy-Haul Integration" in changelog
assert "125/130 (96.2%)" in changelog
assert "Verify Fieldmaster native terrain and heavy-haul" in workflow

print(
    "Fieldmaster native terrain/heavy-haul sanity passed with normalized native tractor health; "
    "roadmap remains 125/130 (96.2%) with SVG-only progress presentation"
)
