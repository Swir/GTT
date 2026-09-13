from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "Source/GTT/Public/World/GTTVillagePresentationSubsystem.h"
CPP = ROOT / "Source/GTT/Private/World/GTTVillagePresentationSubsystem.cpp"
ROADMAP = ROOT / "Docs/ROADMAP.md"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.0.37.md"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


header = HEADER.read_text(encoding="utf-8")
cpp = CPP.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")

require("public UWorldSubsystem" in header, "presentation layer must be a world subsystem")
require("EWorldType::Game" in cpp and "EWorldType::PIE" in cpp, "presentation pass must support game and PIE worlds")
require("TActorIterator<AGTTPrototypeWorld>" in cpp, "presentation pass must scope itself to the GTT prototype world")
require("APointLight" in cpp and "SetIntensity(4200.0f)" in cpp, "street lighting must be implemented")
require("SetAttenuationRadius(850.0f)" in cpp, "street-light range must be bounded")
require("AGTTDayNightCycle" in cpp and "IsNight()" in cpp, "street lights must consume the existing day/night system")
require("SetVisibility(bNight)" in cpp, "street lights must actually switch with night state")
require("LabelVisibleDistance = 1900.0f" in cpp, "world labels need an explicit local readability radius")
require("TActorIterator<ATextRenderActor>" in cpp and "SetVisibility(bNearby" in cpp, "distant world-label clutter must be culled")
require("LampPositions" in cpp and cpp.count("FVector(-2700,-1800,0)") == 1, "main village street-light layout is missing")
require("for (int32 Bale = 0; Bale < 8; ++Bale)" in cpp, "Hill Farm dressing must include hay bales")
require("for (int32 Log = 0; Log < 10; ++Log)" in cpp, "North Wood Yard dressing must include log stacks")
require("/Engine/BasicShapes/" in cpp, "presentation geometry must use project-safe engine primitives")
require("<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap, "roadmap style lock marker is missing")
require("**125** | **5** | **130** | **96.2%**" in roadmap, "0.0.37 must not fake roadmap completion")
require("- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap, "Native Chaos tractor acceptance must stay open")
require("- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap, "packaged runtime acceptance must stay open")
require("- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap, "Native Chaos drivetrain acceptance must stay open")
require("- [ ] Full Win64 CI/build runner" in roadmap, "Win64 runner acceptance must stay open")
require(PLAYTEST.exists(), "PLAYTEST_0.0.37.md must document the presentation milestone")

# This source-built visual pass must not introduce external media as a hidden dependency.
for suffix in (".fbx", ".obj", ".gltf", ".glb", ".wav", ".mp3", ".ogg"):
    matches = [p for p in ROOT.rglob(f"*{suffix}") if ".git" not in p.parts]
    if matches:
        require(False, f"unexpected external presentation/media dependency: {matches[0]}")

checkboxes = re.findall(r"^- \[([ xX])\] ", roadmap, flags=re.MULTILINE)
done = sum(1 for value in checkboxes if value.lower() == "x")
require((done, len(checkboxes)) == (125, 130), f"roadmap checkbox math drifted to {done}/{len(checkboxes)}")

print("Village presentation milestone verification passed.")
