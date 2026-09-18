#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "Source/GTT/Public/Vehicles/GTTWorkshopRecoverySubsystem.h").read_text(encoding="utf-8")
SOURCE = (ROOT / "Source/GTT/Private/Vehicles/GTTWorkshopRecoverySubsystem.cpp").read_text(encoding="utf-8")
ROADMAP = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
README = (ROOT / "README.md").read_text(encoding="utf-8")

checks = {
    "world subsystem exists": "UGTTWorkshopRecoverySubsystem : public UTickableWorldSubsystem" in HEADER,
    "blueprint quote struct": "FGTTWorkshopServiceQuote" in HEADER and "BlueprintType" in HEADER,
    "paid service API": "PurchaseFullWorkshopService" in HEADER and "PurchaseFullWorkshopService" in SOURCE,
    "physical workshop radius": "WorkshopVehicleRadiusCm" in SOURCE and "IsVehicleAtWorkshop" in SOURCE,
    "player proximity required": "WorkshopCustomerRadiusCm" in SOURCE and "CUSTOMER_TOO_FAR" in SOURCE,
    "owned exact id required": "bOwnedByPlayer" in SOURCE and "PersistentVehicleId" in SOURCE and "ExpectedVehicleId" in SOURCE,
    "wanted blocks voluntary workshop": "WANTED_ACTIVE" in SOURCE and "GetWantedLevel() > 0" in SOURCE,
    "authoritative repair quote": "CalculateRepairEstimate(Vehicle)" in SOURCE,
    "single economy debit": SOURCE.count("SpendCash(Quote.TotalCost") == 1,
    "native service reused": "ApplyNativeWorkshopService()" in SOURCE,
    "transaction snapshot": all(s in SOURCE for s in ["const FGTTRoadVehicleMigrationSnapshot Before", "BodyBefore", "DetachedMaskBefore", "CargoLoadBefore"]),
    "verification rollback": all(s in SOURCE for s in ["RestorePersistentMigrationSnapshot(Before)", "RestorePersistentBodyDamage(BodyBefore", "AddCash(Quote.TotalCost", "refund=YES"]),
    "exact identity verified": "Vehicle->GetPersistentVehicleId() == ExpectedVehicleId" in SOURCE,
    "cargo preserved": "bCargoPreserved" in SOURCE and "GetCargoLoadFactor" in SOURCE,
    "full mechanical service verified": all(s in SOURCE for s in ["ConditionPercent >= 0.999f", "TireIntegrity >= 0.999f", "FuelCapacityLiters", "DetachedPanelCount == 0"]),
    "persistence mirror flushed": SOURCE.count("FlushNativePersistenceMirror()") >= 2,
    "keyboard workshop action": "WasInputKeyJustPressed(EKeys::H)" in SOURCE,
    "roadside charge explicitly separate": "Tow charge is separate" in SOURCE,
    "no cargo authority mutation": "FarmCargo" not in SOURCE and "CompleteFarm" not in SOURCE and "AddCash(" not in SOURCE.replace("Economy->AddCash(Quote.TotalCost", ""),
    "roadmap marker retained": "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in ROADMAP,
    "roadmap progress marker retained": ROADMAP.count("ROADMAP-PROGRESS") >= 2,
    "svg-only roadmap embed": "../assets/readme/progress-mini.svg" in ROADMAP,
    "readme v2 retained": "<!-- SWIR-README-STANDARD:v2 -->" in README,
    "readme progress card retained": README.count("assets/readme/progress-card.svg") == 1,
    "search keywords retained": "## 🔎 Search Keywords" in README,
}

completed = len(re.findall(r"^- \[x\]", ROADMAP, flags=re.MULTILINE | re.IGNORECASE))
remaining = len(re.findall(r"^- \[ \]", ROADMAP, flags=re.MULTILINE))
total = completed + remaining
progress = round((100.0 * completed / total), 1) if total else None
checks["roadmap truth 125/130"] = (completed, remaining, total, progress) == (125, 5, 130, 96.2)
checks["no legacy unicode meter"] = not re.search(r"[█▓▒░]{3,}", ROADMAP + "\n" + README)

failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(f"{'PASS' if ok else 'FAIL'} | {name}")
if failed:
    raise SystemExit("Workshop recovery integration verification failed: " + ", ".join(failed))
print(f"PASS | GTT 0.1.41 workshop recovery contract | checks={len(checks)} | roadmap={completed}/{total} ({progress:.1f}%)")
