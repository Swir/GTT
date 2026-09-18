from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTFarmTrailer.h").read_text(encoding="utf-8")
source = (ROOT / "Source/GTT/Private/Vehicles/GTTFarmTrailer.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.49.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.49.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")

for token in [
    "LeftWheelConstraint",
    "RightWheelConstraint",
    "HasIntactAxle",
    "ConfigureWheelAxle",
    "NotifyHit",
    "WheelBreakForce",
    "WheelBreakTorque",
]:
    assert token in header, f"trailer axle API missing {token}"

for token in [
    "LeftWheel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)",
    "RightWheel->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)",
    "LeftWheel->SetSimulatePhysics(true)",
    "RightWheel->SetSimulatePhysics(true)",
    "SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free",
    "SetLinearBreakable(true, WheelBreakForce)",
    "SetAngularBreakable(true, WheelBreakTorque)",
    "SetConstrainedComponents(TrailerBody, NAME_None, Wheel, NAME_None)",
    "LeftWheelConstraint->IsBroken()",
    "RightWheelConstraint->IsBroken()",
    "AxlePenalty",
    "AGTTFarmTrailer::NotifyHit",
    "TrailerIntegrity = FMath::Max",
    "CargoIntegrity = FMath::Max",
    "ConfigureWheelAxle(LeftWheelConstraint, LeftWheel)",
    "ConfigureWheelAxle(RightWheelConstraint, RightWheel)",
]:
    assert token in source, f"trailer axle behavior missing {token}"

for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
    "ROADMAP-96.2%25",
    "DONE-125%2F130",
    "| **125** | **5** | **130** | **96.2%** |",
):
    assert token in roadmap, f"roadmap SVG-only presentation missing {token}"
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE), "legacy text/Unicode roadmap progress meter must not return"
assert "- [ ] Authored skeletal trailer wheel assets and final hitch sockets" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap
assert "Trailer Axle & Impact Physics" in playtest
assert "Trailer Axle & Impact Physics" in changelog
assert "125/130 (96.2%)" in changelog
assert "Verify trailer axle and impact physics" in workflow

print("Trailer axle/impact physics sanity passed; roadmap remains 125/130 (96.2%) with SVG-only progress")
