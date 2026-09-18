#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
read = lambda p: (ROOT / p).read_text(encoding="utf-8")
HEADER = read("Source/GTT/Public/World/GTTServiceTerminal.h")
SOURCE = read("Source/GTT/Private/World/GTTServiceTerminal.cpp")
RECOVERY = read("Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp")
GARAGE = read("Source/GTT/Private/World/GTTGarageFleetSubsystem.cpp")
ROADMAP = read("Docs/ROADMAP.md")
README = read("README.md")
PLAYTEST = read("Docs/PLAYTEST_0.1.41.md")
CHANGELOG = read("CHANGELOG.d/0.1.41.md")

checks = {
    "no parallel workshop subsystem": not (ROOT / "Source/GTT/Public/Vehicles/GTTWorkshopRecoverySubsystem.h").exists() and not (ROOT / "Source/GTT/Private/Vehicles/GTTWorkshopRecoverySubsystem.cpp").exists(),
    "existing terminal owns transaction": "PurchaseNativeRoadWorkshopService" in HEADER and "PurchaseNativeRoadWorkshopService" in SOURCE,
    "blueprint quote API": "BlueprintPure" in HEADER and "GetNativeRoadRepairQuote" in HEADER and "GetNativeRoadFuelQuote" in HEADER,
    "blueprint purchase API": "BlueprintCallable" in HEADER and "PurchaseNativeRoadWorkshopService" in HEADER,
    "native owned exact id required": "bOwnedByPlayer" in SOURCE and "ExpectedVehicleId" in SOURCE and "GetPersistentVehicleId() == ExpectedVehicleId" in SOURCE,
    "wanted fail closed": "WANTED_ACTIVE" not in SOURCE and "GetWantedLevel() > 0" in SOURCE and "charged=NO" in SOURCE,
    "physical terminal radius": "VehicleSearchRadius" in SOURCE and "DistSquared(GetActorLocation(), Vehicle->GetActorLocation())" in SOURCE,
    "repair quote uses production decision": "CalculateRepairEstimate(Vehicle, WorkshopServiceCost)" in SOURCE,
    "fuel quote is per liter": "MissingLiters * NativeFuelPricePerLiter" in SOURCE,
    "full service single debit": SOURCE.count("SpendCash(TotalCost") == 1,
    "fuel service single debit": SOURCE.count("SpendCash(FuelCost") == 1,
    "native service reused": "ApplyNativeWorkshopService()" in SOURCE,
    "transaction snapshots": all(token in SOURCE for token in ["const FGTTRoadVehicleMigrationSnapshot Before", "BodyBefore", "DetachedMaskBefore", "CargoLoadBefore"]),
    "repair rollback and refund": all(token in SOURCE for token in ["RestorePersistentMigrationSnapshot(Before)", "RestorePersistentBodyDamage(BodyBefore", "AddCash(TotalCost", "refund=YES"]),
    "refuel rollback and refund": "AddCash(FuelCost" in SOURCE and "NATIVE_WORKSHOP_REFUEL_COMPLETE" in SOURCE,
    "cargo continuity verified": "bCargoPreserved" in SOURCE and "GetCargoLoadFactor" in SOURCE,
    "full restoration verified": all(token in SOURCE for token in ["NativeRoadFullyServiced", "ConditionPercent >= 0.999f", "TireIntegrity >= 0.999f", "DetachedPanelCount == 0", "NeedsNativeWorkshopService"]),
    "persistent save after success": SOURCE.count("GameMode->SaveProgress()") >= 3 and "FlushNativePersistenceMirror" in SOURCE,
    "terminal interaction uses transaction": "PurchaseNativeRoadWorkshopService(NativeRoad, Pawn)" in SOURCE,
    "exact id visible in prompt": "VehicleTag" in SOURCE and "GetPersistentVehicleId().ToString()" in SOURCE,
    "tow remains transport only": "serviced=NO destination=WORKSHOP" in RECOVERY and "Damage preserved; workshop estimate" in RECOVERY,
    "tow exact identity retained": "identity_preserved" in RECOVERY and "ExpectedVehicleId" in RECOVERY,
    "garage remains fleet authority": "BuildFleetSnapshot" in GARAGE and "RepairEstimate" in GARAGE,
    "no farm cargo mutation": "FarmCargo" not in SOURCE and "CompleteFarm" not in SOURCE,
    "milestone playtest": "0.1.41" in PLAYTEST and "Farm Cargo" in PLAYTEST and "68" in PLAYTEST,
    "milestone changelog fragment": "0.1.41" in CHANGELOG and "workshop" in CHANGELOG.lower(),
    "roadmap style marker": "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in ROADMAP,
    "roadmap progress markers": ROADMAP.count("ROADMAP-PROGRESS") >= 2,
    "svg-only roadmap": ROADMAP.count("../assets/readme/progress-mini.svg") == 1,
    "readme v2": "<!-- SWIR-README-STANDARD:v2 -->" in README,
    "readme single card": README.count("assets/readme/progress-card.svg") == 1,
    "search keywords": "## 🔎 Search Keywords" in README,
}

items = re.findall(r"^- \[(x| )\] ", ROADMAP, flags=re.MULTILINE | re.IGNORECASE)
done = sum(value.lower() == "x" for value in items)
total = len(items)
remaining = total - done
progress = round(done * 100.0 / total, 1) if total else None
checks["roadmap truth 125/130"] = (done, remaining, total, progress) == (125, 5, 130, 96.2)
progress_block = ROADMAP.split("<!-- ROADMAP-PROGRESS:START -->", 1)[1].split("<!-- ROADMAP-PROGRESS:END -->", 1)[0]
checks["no legacy unicode meter"] = not re.search(r"[█▓▒░]{3,}", progress_block + "\n" + README)

failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(f"{'PASS' if ok else 'FAIL'} | {name}")
if failed:
    raise SystemExit("GTT 0.1.41 workshop recovery verification failed: " + ", ".join(failed))
print(f"PASS | GTT 0.1.41 existing-workshop recovery integration | checks={len(checks)} | roadmap={done}/{total} ({progress:.1f}%)")
