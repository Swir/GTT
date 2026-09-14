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
incident_header = (ROOT / "Source/GTT/Public/Vehicles/GTTNativeRoadIncidentSubsystem.h").read_text(encoding="utf-8")
incident_source = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeRoadIncidentSubsystem.cpp").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.68.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.68.md").read_text(encoding="utf-8")

for token in [
    "GetLastImpactSpeedKmh",
    "GetNativeImpactCount",
]:
    require(token in header, f"missing Native road incident evidence accessor: {token}")

for token in [
    "UGTTNativeRoadIncidentSubsystem",
    "ScanNativeRoadIncidents",
    "IncidentScanTimer",
    "LastSeenImpactCounts",
]:
    require(token in incident_header or token in incident_source, f"missing road incident subsystem token: {token}")

for token in [
    "TActorIterator<AGTTRoadVehicleNativePawn>",
    "GetNativeImpactCount()",
    "GetLastImpactSpeedKmh()",
    "TActorIterator<AGTTTrafficCarPawn>",
    "ApplyVehicleDamage(VictimBodyDamage)",
    "ApplyTireDamage(VictimTireDamage)",
    "FindComponentByClass<UGTTWantedComponent>()",
    "Wanted->AddHeat(CrimeHeat)",
    "NATIVE_ROAD_TRAFFIC_INCIDENT",
    "NATIVE_ROAD_CRIME_ESCALATION",
]:
    require(token in incident_source, f"missing traffic/crime integration token: {token}")

require("GetCargoLoadFactor() * 2.5f" in incident_source, "loaded Mulebox cargo must influence road-incident heat")
require("TrafficIncidentRadiusCm" in incident_source, "incident attribution must be spatially bounded")
require("CurrentImpactCount <= LastSeenCount" in incident_source, "incident system must deduplicate already processed hits")
require("Verify Native road traffic incidents and wanted escalation" in workflow, "project sanity must execute 0.0.68 verifier")
require("python Scripts/verify_native_road_incidents.py" in workflow, "workflow command for 0.0.68 verifier missing")
require("0.0.68" in playtest and "traffic" in playtest.lower() and "wanted" in playtest.lower(), "0.0.68 playtest coverage incomplete")
require("0.0.68" in changelog and "Win64" in changelog, "0.0.68 changelog must retain honest Win64 limitation")

require("<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap, "SWIR roadmap standard marker missing")
require("<!-- ROADMAP-PROGRESS:START -->" in roadmap and "<!-- ROADMAP-PROGRESS:END -->" in roadmap, "roadmap progress block missing")
for required in ['alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"', "## 📊 Overall progress"]:
    require(required in roadmap, f"roadmap dashboard element missing: {required}")

tasks = re.findall(r"^\s*-\s+\[([xX ])\]\s+", roadmap, flags=re.MULTILINE)
done = sum(1 for state in tasks if state.lower() == "x")
total = len(tasks)
require(total > 0, "roadmap checklist not found")
remaining = total - done
progress = round(done / total * 100.0, 1)
segments = round(progress / 5.0)
expected_bar = "█" * segments + "░" * (20 - segments)
require(f"DONE-{done}%2F{total}" in roadmap, f"DONE badge stale: expected {done}/{total}")
require(f"ROADMAP-{progress:.1f}%25" in roadmap, f"ROADMAP badge stale: expected {progress:.1f}%")
require(f"| **{done}** | **{remaining}** | **{total}** | **{progress:.1f}%** |" in roadmap, "roadmap table is stale")
require(f"{expected_bar} {progress:.1f}%" in roadmap, "20-segment roadmap bar is stale")

print(f"[OK] Native road traffic incident/wanted integration verified; roadmap {done}/{total} ({progress:.1f}%).")
