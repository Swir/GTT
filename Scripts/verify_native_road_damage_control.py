#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]

def require(condition, message):
    if not condition:
        print(f"[FAIL] {message}")
        sys.exit(1)

header = (ROOT / "Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h").read_text(encoding="utf-8")
source = (ROOT / "Source/GTT/Private/Vehicles/GTTRoadVehicleNativePawn.cpp").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.67.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.67.md").read_text(encoding="utf-8")

for token in [
    "NotifyHit(",
    "UpdateNativeWheelRuntime",
    "ApplyNativeImpactDamage",
    "StopNativeDriveForBreakdown",
    "GetRuntimeWheelRisk",
    "GetRuntimeWheelContacts",
]:
    require(token in header or token in source, f"missing Native road runtime contract token: {token}")

for token in [
    "GetWheelState(WheelIndex)",
    "NATIVE_ROAD_WHEEL_STATE_EVIDENCE",
    "NATIVE_ROAD_IMPACT_DAMAGE",
    "NATIVE_ROAD_BREAKDOWN",
    "RuntimeThrottleLimit",
    "RuntimeSteeringLimit",
    "RuntimeBrakeAssist",
    "MigrationSnapshot.ConditionPercent = FMath::Clamp",
    "MigrationSnapshot.TireIntegrity = FMath::Clamp",
    "CargoInertia",
    "SyncLegacyMirror();",
]:
    require(token in source, f"missing road-fleet damage/traction implementation token: {token}")

require('Movement->SetBrakeInput(1.0f);' in source, "breakdown must actively stop the Native Chaos vehicle")
require("Rattleback82" in source and "Mulebox1200" in source, "both road-fleet vehicles must share the implementation")
require("Verify Native road fleet collision and wheel-state control" in workflow, "project sanity must execute the 0.0.67 verifier")
require("python Scripts/verify_native_road_damage_control.py" in workflow, "workflow command for 0.0.67 verifier missing")
require("0.0.67" in playtest and "collision" in playtest.lower() and "wheel" in playtest.lower(), "0.0.67 playtest coverage incomplete")
require("0.0.67" in changelog and "Win64" in changelog, "0.0.67 changelog must retain honest Win64 limitation")

for required in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "<!-- ROADMAP-PROGRESS:START -->", "<!-- ROADMAP-PROGRESS:END -->",
    'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"', "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg"):
    require(required in roadmap, f"roadmap SVG-only dashboard element missing: {required}")

tasks = re.findall(r"^\s*-\s+\[([xX ])\]\s+", roadmap, flags=re.MULTILINE)
done = sum(1 for state in tasks if state.lower() == "x")
total = len(tasks)
require(total > 0, "roadmap checklist not found")
remaining = total - done
progress = round(done / total * 100.0, 1)
require(f"DONE-{done}%2F{total}" in roadmap, f"DONE badge stale: expected {done}/{total}")
require(f"ROADMAP-{progress:.1f}%25" in roadmap, f"ROADMAP badge stale: expected {progress:.1f}%")
require(f"| **{done}** | **{remaining}** | **{total}** | **{progress:.1f}%** |" in roadmap, "roadmap table is stale")
require(roadmap.count("../assets/readme/progress-mini.svg") == 1, "roadmap must embed exactly one canonical progress-mini.svg")
require(not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE), "legacy text/Unicode roadmap progress meter must not return")

print(f"[OK] Native road fleet collision/wheel-state control contract verified; roadmap {done}/{total} ({progress:.1f}%) with SVG-only progress.")
