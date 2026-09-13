from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
trailer_h = (ROOT / "Source/GTT/Public/Vehicles/GTTFarmTrailer.h").read_text(encoding="utf-8")
trailer_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTFarmTrailer.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.51.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.51.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")

for token in [
    "Drawbar",
    "HitchCoupler",
    "FrontRail",
    "LeftRail",
    "RightRail",
    "Tailgate",
    "LeftFender",
    "RightFender",
    "RearReflectorBar",
    "CargoLogA",
    "CargoLogD",
    "RefreshPresentation",
    "SetCargoVisualsVisible",
    "IsPresentationDamaged",
]:
    assert token in trailer_h, f"trailer presentation API missing {token}"

for token in [
    "ConfigureVisual(Drawbar",
    "ConfigureVisual(HitchCoupler",
    "ConfigureVisual(FrontRail",
    "ConfigureVisual(LeftRail",
    "ConfigureVisual(RightRail",
    "ConfigureVisual(Tailgate",
    "ConfigureVisual(LeftFender",
    "ConfigureVisual(RightFender",
    "ConfigureVisual(RearReflectorBar",
    "ConfigureVisual(CargoLogA",
    "SetCargoVisualsVisible(bCargoLoaded)",
    "LeftFender->SetVisibility(!bLeftWheelLost",
    "RightFender->SetVisibility(!bRightWheelLost",
    "CargoShift",
    "TailgateSag",
    "RefreshPresentation();",
]:
    assert token in trailer_cpp, f"trailer presentation behavior missing {token}"

# Presentation must remain coupled to the existing physical/gameplay trailer rather than a second showcase actor.
assert "TrailerBody->SetSimulatePhysics(true)" in trailer_cpp
assert "ConfigureWheelAxle(LeftWheelConstraint, LeftWheel)" in trailer_cpp
assert "PerformRoadsideRepair" in trailer_cpp
assert "AttachToNativeFieldmaster" in trailer_cpp
assert "CargoIntegrity" in trailer_cpp and "TrailerIntegrity" in trailer_cpp

# Do not falsely claim the authored skeletal trailer roadmap task: this milestone deliberately remains source-built.
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "ROADMAP-96.2%25" in roadmap
assert "DONE-125%2F130" in roadmap
assert "| **125** | **5** | **130** | **96.2%** |" in roadmap
assert "███████████████████░ 96.2%" in roadmap
assert "- [ ] Authored skeletal trailer wheel assets and final hitch sockets" in roadmap
assert "Trailer Presentation & Damage Readability" in playtest
assert "Trailer Presentation & Damage Readability" in changelog
assert "125/130 (96.2%)" in changelog
assert "Verify trailer presentation milestone" in workflow

print("Trailer presentation sanity passed; roadmap remains 125/130 (96.2%)")
