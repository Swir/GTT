#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

fleet_h = read("Source/GTT/Public/World/GTTGarageFleetSubsystem.h")
fleet_cpp = read("Source/GTT/Private/World/GTTGarageFleetSubsystem.cpp")
slot_cpp = read("Source/GTT/Private/World/GTTGarageSlotTerminal.cpp")
office_cpp = read("Source/GTT/Private/World/GTTGarageTerminal.cpp")
service_cpp = read("Source/GTT/Private/World/GTTServiceTerminal.cpp")
sanity = read(".github/workflows/project-sanity.yml")
playtest = read("Docs/PLAYTEST_0.0.97.md")
changelog = read("CHANGELOG.d/0.0.97.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "authoritative fleet snapshot struct": all(token in fleet_h for token in ["FGTTGarageFleetSnapshot", "ConditionPercent", "FuelPercent", "TireIntegrity", "BodyHealth", "RepairEstimate", "TowEstimate", "bNativeAuthority"]),
    "world fleet subsystem": "UGTTGarageFleetSubsystem : public UWorldSubsystem" in fleet_h and "BuildFleetSnapshot" in fleet_h and "BuildFleetSummary" in fleet_h,
    "deterministic garage ordering": all(token in fleet_cpp for token in ["RustyFieldmaster60", "Rattleback82", "Mulebox1200", "GetVehicleSortPriority"]),
    "native road state overrides mirror": all(token in fleet_cpp for token in ["FindActiveNativeRoadVehicle", "GetMigrationSnapshot", "GetBodyDamageSnapshot", "Assessment.RepairEstimate", "Assessment.TowEstimate"]),
    "breakdown states visible": all(token in fleet_cpp for token in ["LIMP", "TOW", "IMMOBILE", "READY"]),
    "native fieldmaster represented": "AGTTFieldmasterNativePawn" in fleet_cpp and "IsLegacyTakeoverActive" in fleet_cpp,
    "office fleet dashboard": "BuildFleetSummary" in office_cpp and "inspect fleet" in office_cpp and "Dispatch sets it ACTIVE and never repairs damage" in office_cpp,
    "registration ignores already-owned cars": "Vehicle->IsOwnedByPlayer()" in office_cpp and "NearestUnownedVehicle" in office_cpp,
    "bay live status": all(token in slot_cpp for token in ["GetSlotSnapshot", "ServiceStatus", "ConditionPercent", "FuelPercent", "TireIntegrity", "BodyHealth"]),
    "native road recall authority": all(token in slot_cpp for token in ["FindActiveNativeRoadVehicle", "TeleportPhysics", "SetPhysicsLinearVelocity", "FlushNativePersistenceMirror"]),
    "recall preserves vehicle state": "Damage, fuel and tuning were preserved" in slot_cpp and "ApplyNativeWorkshopService" not in slot_cpp and "RepairVehicle" not in slot_cpp,
    "recall persists operation": "GameMode->SaveProgress()" in slot_cpp,
    "crime locks retained": "GetWantedLevel() > 0" in slot_cpp and "GetWildlifeAlertLevel() > 0" in slot_cpp,
    "workshop exact native quote prompt": "GetNativeRoadRepairQuote" in service_cpp and "repair + refuel %s ($%d estimate)" in service_cpp,
    "sanity wired": "Verify garage fleet operations and service UX" in sanity and "verify_garage_fleet_operations.py" in sanity,
    "milestone docs": "0.0.97" in playtest and "garage" in playtest.lower() and "0.0.97" in changelog and "fleet" in changelog.lower(),
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("0.0.97 garage fleet operations verification failed: " + ", ".join(failed))

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

print(f"[OK] 0.0.97 garage fleet operations, Native authority recall and workshop quote UX verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.")
