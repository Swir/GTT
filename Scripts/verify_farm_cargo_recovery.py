#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

required = {
    "Source/GTT/Public/Save/GTTSaveGame.h": [
        "SaveVersion = 8", "bFarmCargoContractActive", "FarmCargoStage", "FarmCargoTimeRemaining",
        "FarmCargoIntegrity", "FarmCargoFleetPayoutMultiplier", "FarmCargoMarketMultiplier", "FarmCargoRouteTier",
        "FarmCargoUnitsReserved", "FarmCargoCommodity", "FarmCargoPriority", "bFarmCargoPoliceIncident", "FarmCargoBoundVehicleId",
    ],
    "Source/GTT/Public/Activities/GTTFarmJobDirector.h": ["CaptureActiveCargoToSave", "RestoreActiveCargoFromSave", "AdoptRestoredCargoVehicle"],
    "Source/GTT/Private/Activities/GTTFarmJobPersistence.cpp": [
        "ResetPersistedCargoSnapshot", "bFarmCargoContractActive = true", "invalid_stage",
        "FARM_CARGO_RECOVERY event=RESTORE result=PASS", "AdoptRestoredCargoVehicle", "SetCargoLoadFactor",
    ],
    "Source/GTT/Public/Activities/GTTFarmCargoAuthoritySubsystem.h": ["CaptureToSave", "RestoreFromSave", "TryRebindBoundVehicle", "ResolveVehicleByPersistentId"],
    "Source/GTT/Private/Activities/GTTFarmCargoAuthoritySubsystem.cpp": [
        "ResolveVehicleByPersistentId", "TActorIterator<AGTTRoadVehicleNativePawn>", "TActorIterator<AGTTVehicleBase>",
        "GetPersistentVehicleId() == VehicleId", "FARM_CARGO_RECOVERY event=REBIND result=PASS", "FarmCargoBoundVehicleId",
        "Director->AdoptRestoredCargoVehicle(Resolved)",
    ],
    "Source/GTT/Private/Activities/GTTFarmJobTerminal.cpp": ["SaveCargoCheckpoint", "Persist ReachPickup", "exact physical vehicle ID", "Extended chains now survive a save/reload"],
    "Source/GTT/Private/Core/GTTGameMode.cpp": [
        "LoadGameFromSlot(SaveSlotName, 0)", "FMath::Max(Save->SaveVersion, 8)", "bUnifiedWorldStateInitialized = true",
        "Logistics->CaptureToSave(Save)", "FarmDirector->CaptureActiveCargoToSave(Save)", "CargoAuthority->CaptureToSave(Save)",
        "Logistics->RestoreFromSave(Save)", "FarmDirector->RestoreActiveCargoFromSave(Save)", "CargoAuthority->RestoreFromSave(Save)",
    ],
}

for rel, tokens in required.items():
    path = ROOT / rel
    if not path.is_file():
        raise SystemExit(f"[FAIL] missing {rel}")
    text = path.read_text(encoding="utf-8")
    missing = [token for token in tokens if token not in text]
    if missing:
        raise SystemExit(f"[FAIL] {rel} missing hooks: {missing}")

gamemode = (ROOT / "Source/GTT/Private/Core/GTTGameMode.cpp").read_text(encoding="utf-8")
if "Save->SaveVersion = 3" in gamemode:
    raise SystemExit("[FAIL] legacy GameMode still downgrades primary save to v3")
if gamemode.index("FarmDirector->CaptureActiveCargoToSave(Save)") > gamemode.index("CargoAuthority->CaptureToSave(Save)"):
    raise SystemExit("[FAIL] cargo authority is captured before authoritative route stage")
if gamemode.index("FarmDirector->RestoreActiveCargoFromSave(Save)") > gamemode.index("CargoAuthority->RestoreFromSave(Save)"):
    raise SystemExit("[FAIL] cargo vehicle authority restores before route stage")

authority = (ROOT / "Source/GTT/Private/Activities/GTTFarmCargoAuthoritySubsystem.cpp").read_text(encoding="utf-8")
rebind_match = re.search(r"bool UGTTFarmCargoAuthoritySubsystem::TryRebindBoundVehicle\(\)(.*?)\n}\n", authority, flags=re.S)
if not rebind_match:
    raise SystemExit("[FAIL] TryRebindBoundVehicle body missing")
rebind = rebind_match.group(1)
if "FindNearbyLegacyWorkVehicle" in rebind or "DepotVehicleSearchRadiusCm" in rebind:
    raise SystemExit("[FAIL] recovery may not transfer cargo to a nearest vehicle")
if "ResolveVehicleByPersistentId(BoundCargoVehicleId)" not in rebind:
    raise SystemExit("[FAIL] recovery does not use the saved persistent vehicle ID")

persistence = (ROOT / "Source/GTT/Private/Activities/GTTFarmJobPersistence.cpp").read_text(encoding="utf-8")
if "Save->FarmCargoBoundVehicleId = NAME_None" not in persistence:
    raise SystemExit("[FAIL] idle snapshot does not clear persisted cargo vehicle identity")

roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "<!-- ROADMAP-PROGRESS:START -->", "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress", "../assets/readme/progress-mini.svg",
):
    if token not in roadmap:
        raise SystemExit(f"[FAIL] roadmap structure/progress missing: {token}")
checks = re.findall(r'^- \[(x| )\]', roadmap, flags=re.M | re.IGNORECASE)
if not checks:
    raise SystemExit("[FAIL] roadmap checklist missing")
done = sum(state.lower() == "x" for state in checks)
total = len(checks)
remaining = total - done
percent = round(done * 100.0 / total, 1)
for token in [
    f"ROADMAP-{percent:.1f}%25", f"DONE-{done}%2F{total}",
    f"| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |",
]:
    if token not in roadmap:
        raise SystemExit(f"[FAIL] roadmap dashboard/checklist mismatch: missing {token}")
if roadmap.count("../assets/readme/progress-mini.svg") != 1:
    raise SystemExit("[FAIL] roadmap must embed exactly one progress-mini.svg")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    raise SystemExit("[FAIL] legacy text/Unicode roadmap progress meter must not return")

readme = (ROOT / "README.md").read_text(encoding="utf-8")
if "<!-- SWIR-README-STANDARD:v2 -->" not in readme:
    raise SystemExit("[FAIL] README PRO v2 marker missing or downgraded")
if "## 🔎 Search Keywords" not in readme:
    raise SystemExit("[FAIL] README Search Keywords section missing")

print(f"[OK] Farm Cargo recovery structurally sane: stable vehicle ID + active route checkpoint + non-destructive v8 save; roadmap {done}/{total} = {percent:.1f}% with SVG-only presentation.")
