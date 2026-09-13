from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "Source/GTT/Public/UI/GTTGameHUD.h"
CPP = ROOT / "Source/GTT/Private/UI/GTTGameHUD.cpp"
ROADMAP = ROOT / "Docs/ROADMAP.md"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.0.38.md"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


header = HEADER.read_text(encoding="utf-8")
cpp = CPP.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")

for symbol in ("BuildPrimaryObjective", "BuildContextHint", "BuildVehicleStatus", "BuildVehicleAlert", "DrawHudText"):
    require(symbol in header and symbol in cpp, f"HUD presentation helper missing: {symbol}")

require("Canvas->ClipX" in cpp and "Canvas->ClipY" in cpp, "HUD must anchor presentation to the current viewport")
require("CURRENT OBJECTIVE" in cpp, "primary objective presentation is missing")
require("if (Brawl->IsBrawlActive())" in cpp and "if (HeavyHaul->IsActive())" in cpp, "urgent activity objective priority is missing")
require("if (Farm->IsJobActive())" in cpp and "if (Work->IsWorkActive())" in cpp, "legal-work objective priority is missing")
require("WantedLevel > 0" in cpp, "wanted UI must be contextual")
require("GetWildlifeAlertLevel() > 0" in cpp, "warden UI must be contextual")
require("Radio && Radio->IsRadioOn()" in cpp, "radio row must only appear while radio is on")
require("GetTireIntegrity() < 0.30f" in cpp, "critical tire warning threshold is missing")
require("GetFuelPercent() < 0.15f" in cpp, "low-fuel warning threshold is missing")
require("GetEngineTemperatureC() > 108.0f" in cpp, "engine temperature warning threshold is missing")
require("F exit vehicle" in cpp and "Q next weapon" in cpp, "vehicle/on-foot contextual hints are missing")

# Normal player presentation must no longer expose the old permanent debug-like telemetry wall.
for noisy_literal in ("VEHICLE DYNAMICS |", "TUNING | ENGINE", "CONTROLS | LMB attack"):
    require(noisy_literal not in cpp, f"legacy always-visible HUD clutter remains: {noisy_literal}")

require("<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap, "roadmap style lock marker is missing")
require("**125** | **5** | **130** | **96.2%**" in roadmap, "0.0.38 must not fake roadmap completion")
require("- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap, "Native Chaos tractor acceptance must stay open")
require("- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap, "packaged runtime acceptance must stay open")
require("- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap, "Native Chaos drivetrain acceptance must stay open")
require("- [ ] Authored skeletal trailer wheel assets and final hitch sockets" in roadmap, "trailer authored-asset acceptance must stay open")
require("- [ ] Full Win64 CI/build runner" in roadmap, "Win64 runner acceptance must stay open")
require(PLAYTEST.exists(), "PLAYTEST_0.0.38.md must document the HUD milestone")

checkboxes = re.findall(r"^- \[([ xX])\] ", roadmap, flags=re.MULTILINE)
done = sum(1 for value in checkboxes if value.lower() == "x")
require((done, len(checkboxes)) == (125, 130), f"roadmap checkbox math drifted to {done}/{len(checkboxes)}")

print("Demo HUD presentation verification passed.")
