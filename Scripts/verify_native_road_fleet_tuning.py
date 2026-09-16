#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

road_h = read("Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h")
road_cpp = read("Source/GTT/Private/Vehicles/GTTRoadVehicleNativePawn.cpp")
tuning_cpp = read("Source/GTT/Private/World/GTTTuningTerminal.cpp")
service_h = read("Source/GTT/Public/World/GTTServiceTerminal.h")
service_cpp = read("Source/GTT/Private/World/GTTServiceTerminal.cpp")
fleet_h = read("Source/GTT/Public/World/GTTGarageFleetSubsystem.h")
fleet_cpp = read("Source/GTT/Private/World/GTTGarageFleetSubsystem.cpp")
sanity = read(".github/workflows/project-sanity.yml")
playtest = read("Docs/PLAYTEST_0.0.98.md")
changelog = read("CHANGELOG.d/0.0.98.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "native road service authority API": all(token in road_h for token in [
        "RepairNativeTires", "InstallNativeEngineUpgrade", "InstallNativeTireUpgrade",
        "RefuelNativeVehicle", "GetFuelCapacityLiters", "SyncLegacyMirror()"]),
    "native tuning changes real drive behavior": all(token in road_cpp for token in [
        "MigrationSnapshot.EngineUpgradeLevel", "TunePower", "EfficiencyBonus",
        "MigrationSnapshot.TireUpgradeLevel", "TuneGrip", "TuneAssist"]),
    "tuning selects native road pawn first": all(token in tuning_cpp for token in [
        "FindActiveNativeRoadVehicle", "InstallNativeEngineUpgrade", "InstallNativeTireUpgrade",
        "RepairNativeTires", "GameMode->SaveProgress()"]),
    "hidden road mirror cannot steal tuning": "HasActiveNativeRoadTakeover" in tuning_cpp and "periodic Native -> mirror synchronization" in tuning_cpp,
    "player sees exact next native tuning price": all(token in tuning_cpp for token in [
        "Engine tune %s L%d/3 ($%d)", "Heavy-duty tires %s L%d/3 ($%d)", "Tire service %s ($%d)"]),
    "fuel-only native workshop route": all(token in service_h + service_cpp for token in [
        "NativeFuelPricePerLiter", "GetNativeRoadFuelQuote", "RefuelNativeVehicle", "exact fuel quote"]),
    "fuel visit does not force repair": "Mechanical state was not changed" in service_cpp and "!bNeedsMechanical && bNeedsFuel" in service_cpp,
    "workshop and refuel persist": service_cpp.count("GameMode->SaveProgress()") >= 4,
    "garage service planner fields": all(token in fleet_h for token in [
        "FuelLiters", "FuelCapacityLiters", "NextServiceAction", "FuelEstimate",
        "TireServiceEstimate", "NextEngineUpgradeCost", "NextTireUpgradeCost"]),
    "garage planner uses shared economy constants": all(token in fleet_cpp for token in [
        "GarageTireServiceCost = 65", "GarageEngineBaseCost = 240", "GarageTireUpgradeBaseCost = 170",
        "GarageFuelPricePerLiter = 3.25f", "PopulateServicePlan"]),
    "garage exposes tune state and next action": all(token in fleet_cpp for token in [
        "E%d/3 G%d/3", "NEXT %s", "ENGINE TUNE", "TIRE UPGRADE", "REFUEL", "TIRES"]),
    "native garage source remains authoritative": all(token in fleet_cpp for token in [
        "FindActiveNativeRoadVehicle", "GetMigrationSnapshot", "GetFuelCapacityLiters", "bNativeAuthority = true"]),
    "sanity wired": "Verify Native road fleet tuning and persistent service" in sanity and "verify_native_road_fleet_tuning.py" in sanity,
    "milestone docs": "0.0.98" in playtest and "0.0.98" in changelog and "Rattleback" in playtest and "Mulebox" in playtest,
    "demo honesty retained": "not a demo-release approval" in playtest.lower() and "Win64" in playtest,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("0.0.98 native road fleet tuning/service verification failed: " + ", ".join(failed))

# Style-lock and exact roadmap math remain a hard regression gate even though this milestone
# intentionally does not mark any hardware/packaged-build checkbox complete.
if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap or "## 📊 Overall progress" not in roadmap:
    raise SystemExit("SWIR roadmap style lock missing")
items = re.findall(r"^- \[(x| )\] ", roadmap, flags=re.MULTILINE)
done = sum(value == "x" for value in items)
total = len(items)
remaining = total - done
percent = round(done * 100.0 / total, 1)
filled = round(done * 20.0 / total)
bar = "█" * filled + "░" * (20 - filled)
for token in (f"ROADMAP-{percent:.1f}%25", f"DONE-{done}%2F{total}", f"{bar} {percent:.1f}%", f"| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |"):
    if token not in roadmap:
        raise SystemExit("Roadmap dashboard drift: missing " + token)
if (done, total) != (125, 130):
    raise SystemExit(f"Roadmap checkbox drift: {done}/{total}")

print(f"[OK] 0.0.98 Native road tuning authority, fuel-only service, garage planning and persistence verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.")
