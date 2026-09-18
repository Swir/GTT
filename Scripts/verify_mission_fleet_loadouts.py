#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

fleet_h = read("Source/GTT/Public/World/GTTGarageFleetSubsystem.h")
fleet_cpp = read("Source/GTT/Private/World/GTTGarageFleetSubsystem.cpp")
slot_cpp = read("Source/GTT/Private/World/GTTGarageSlotTerminal.cpp")
save_h = read("Source/GTT/Public/Save/GTTSaveGame.h")
struct_cpp = read("Source/GTT/Private/Core/GTTStructuralGameMode.cpp")
farm_cpp = read("Source/GTT/Private/Activities/GTTFarmJobDirector.cpp")
heavy_cpp = read("Source/GTT/Private/Activities/GTTHeavyHaulDirector.cpp")
rural_cpp = read("Source/GTT/Private/Activities/GTTRuralWorkDirector.cpp")
sanity = read(".github/workflows/project-sanity.yml")
playtest = read("Docs/PLAYTEST_0.1.0.md")
changelog = read("CHANGELOG.d/0.1.0.md")
roadmap = read("Docs/ROADMAP.md")
playtest_lower = playtest.lower()
changelog_lower = changelog.lower()

checks = {
    "readiness contract exposed": all(token in fleet_h for token in ["EGTTFleetMissionReadiness", "FGTTFleetMissionAssessment", "AssessJobReadiness", "IsJobFleetReady", "MissionReadinessLabel", "RecommendedRoleForJob"]),
    "per-role loadout API exposed": all(token in fleet_h for token in ["GetRoleLoadoutVehicleId", "SetRoleLoadoutVehicleId", "RestoreRoleLoadouts", "PreferredTractorVehicleId", "PreferredRoadVehicleId", "PreferredCargoVehicleId"]),
    "jobs map to deterministic roles": all(token in fleet_cpp for token in ["FarmCargoJob", "HeavyHaulJob", "TimberHaulJob", "FieldMowingJob", "RoadRunJob", "EGTTGarageFleetRole::Cargo", "EGTTGarageFleetRole::Tractor", "EGTTGarageFleetRole::Road"]),
    "mission thresholds consider actual wear": all(token in fleet_cpp for token in ["GetJobThresholds", "ConditionPercent", "FuelPercent", "TireIntegrity", "BodyHealth", "ServiceRequired", "PrepEstimate"]),
    "mission hint uses exact garage slot": all(token in fleet_cpp for token in ["Assessment.AssignedSlot + 1", "prep~$%d", "MissionReadinessLabel"]),
    "dispatch commits role loadout": "SetRoleLoadoutVehicleId(ClassifyVehicleRole(VehicleId), VehicleId)" in fleet_cpp,
    "garage surfaces role loadout": "[LOADOUT]" in slot_cpp and "mission loadout" in slot_cpp and "LOADOUTS T:%s R:%s C:%s" in fleet_cpp,
    "save schema v8 retains v7 loadouts": "SaveVersion = 8" in save_h and "v7 adds per-role mission loadouts" in save_h,
    "save stores three role ids": all(token in save_h for token in ["PreferredTractorVehicleId", "PreferredRoadVehicleId", "PreferredCargoVehicleId"]),
    "structural save writes role ids": all(token in struct_cpp for token in ["MissionLoadoutSaveVersion = 7", "ExtendedSaveVersion = 8", "Save->PreferredTractorVehicleId", "Save->PreferredRoadVehicleId", "Save->PreferredCargoVehicleId"]),
    "structural load migrates pre-v7": "Save->SaveVersion >= MissionLoadoutSaveVersion" in struct_cpp and "RestoreRoleLoadouts(NAME_None, NAME_None, NAME_None)" in struct_cpp,
    "v5 and v6 compatibility thresholds retained": "StructuralDamageSaveVersion = 5" in struct_cpp and "FleetDispatchSaveVersion = 6" in struct_cpp,
    "farm cargo consumes readiness": "AssessJobReadiness(FName(TEXT(\"FarmCargo\")))" in farm_cpp and "CARGO LOADOUT NEEDS SERVICE" in farm_cpp,
    "heavy haul consumes and blocks unsafe tractor": "AssessJobReadiness(FName(TEXT(\"HeavyHaul\")))" in heavy_cpp and "HEAVY HAUL BLOCKED" in heavy_cpp and "ServiceRequired" in heavy_cpp,
    "timber consumes cargo readiness": "AssessJobReadiness(FName(TEXT(\"TimberHaul\")))" in rural_cpp and "TIMBER LOADOUT CAUTION" in rural_cpp,
    "mowing blocks unsafe tractor": "AssessJobReadiness(FName(TEXT(\"FieldMowing\")))" in rural_cpp and "FIELD WORK BLOCKED" in rural_cpp,
    "crime locks remain in mission loops": "GetWantedLevel() > 0" in farm_cpp and "GetWildlifeAlertLevel() > 0" in farm_cpp and "CanTakeContract" in heavy_cpp and "CanTakeLegalWork" in rural_cpp,
    "existing role rewards retained": "MuleboxRoleBonus" in farm_cpp and "MULEBOX ROLE BONUS" in farm_cpp,
    "sanity wired": "Verify mission-aware fleet loadouts and job readiness" in sanity and "verify_mission_fleet_loadouts.py" in sanity,
    "playtest covers runtime route": all(token in playtest_lower for token in ["0.1.0", "role loadouts", "heavy haul", "mowing", "save/load", "win64"]),
    "changelog documents milestone": all(token in changelog_lower for token in ["0.1.0", "mission-aware fleet", "save schema v7", "125/130"]),
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Mission-aware fleet loadout verification failed: " + ", ".join(failed))

required_style = ['<!-- SWIR-ROADMAP-STANDARD:v1 -->', '<!-- ROADMAP-PROGRESS:START -->', 'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"', '## 📊 Overall progress', '../assets/readme/progress-mini.svg', '<!-- ROADMAP-PROGRESS:END -->']
for token in required_style:
    if token not in roadmap:
        raise SystemExit("SWIR roadmap style lock missing: " + token)
items = re.findall(r'^- \[(x| )\] ', roadmap, flags=re.MULTILINE)
done = sum(v == 'x' for v in items)
total = len(items)
if not total:
    raise SystemExit("Roadmap checklist missing")
remaining = total - done
percent = round(done * 100.0 / total, 1)
for token in (f'ROADMAP-{percent:.1f}%25', f'DONE-{done}%2F{total}', 'STATUS-IN%20PROGRESS', f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |'):
    if token not in roadmap:
        raise SystemExit("Roadmap dashboard drift: missing " + token)
if (done, total, percent) != (125, 130, 96.2):
    raise SystemExit(f"0.1.0 source milestone must not claim build/art gates: {done}/{total} = {percent:.1f}%")
progress_block = roadmap.split('<!-- ROADMAP-PROGRESS:START -->', 1)[1].split('<!-- ROADMAP-PROGRESS:END -->', 1)[0]
if progress_block.count('../assets/readme/progress-mini.svg') != 1:
    raise SystemExit('Roadmap progress block must embed exactly one canonical progress-mini.svg.')
if re.search(r'[█▓▒░]{3,}', progress_block):
    raise SystemExit('Legacy text/Unicode progress meter must not return to the active Roadmap dashboard.')

print(f"[OK] GTT 0.1.0 mission-aware fleet loadouts retained under save v8 ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}% with SVG-only progress.")
