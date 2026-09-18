#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

required = {
    "Source/GTT/Public/Save/GTTSaveGame.h": [
        "SaveVersion = 8", "bUnifiedWorldStateInitialized", "UnifiedWorldStateRevision",
        "CombatWeaponTypes", "CombatEquippedWeaponType", "CombatShotgunAmmo",
        "MainStoryStage", "Arc3Stage", "Arc4Stage", "Arc4StartingFactionVictories",
        "bArc4ContrabandPrepared", "FactionVictories", "RustDogsDefeated",
        "StoneCrowsDefeated", "MudJackalsDefeated", "ContrabandUnits", "ContrabandValue",
        "bInsuranceActive", "ImpoundedVehicleId", "PendingImpoundFee",
        "LifetimeFenceRevenue", "SpeedingCitations", "RoadStructuralDamage",
        "PreferredGarageVehicleId", "PreferredTractorVehicleId", "PreferredRoadVehicleId", "PreferredCargoVehicleId",
        "LogisticsReputation", "LogisticsCleanStreak", "LogisticsCompletedRuns", "LogisticsFailedRuns", "LogisticsLifetimeRevenue"
    ],
    "Source/GTT/Public/Save/GTTUnifiedSaveSubsystem.h": [
        "GTT_Prototype_01", "GTT_Combat_01", "GTT_MainStory_01", "GTT_MainStory_Arc3_01",
        "GTT_MainStory_Arc4_01", "GTT_Factions_01", "GTT_RuralEconomy_01",
        "ConsolidateLegacySlots", "HydrateCompatibilityMirrors"
    ],
    "Source/GTT/Private/Save/GTTUnifiedSaveSubsystem.cpp": [
        "Never manufacture a primary save on a brand-new profile", "FMath::Max(Primary->SaveVersion, 4)",
        "bUnifiedWorldStateInitialized = true", "UnifiedWorldStateRevision",
        "UGTTCombatSave", "UGTTMainStorySave", "UGTTArc3Save", "UGTTArc4Save",
        "UGTTFactionSaveGame", "UGTTRuralEconomySave", "SaveGameToSlot",
        "SetTimer", "5.0f"
    ],
    "Docs/PLAYTEST_0.0.24.md": ["Unified World State", "migration", "compatibility", "GTT_Prototype_01"],
    "CHANGELOG.md": ["[0.0.24]", "Unified World State", "primary sandbox SaveGame"],
    ".github/workflows/project-sanity.yml": ["Verify unified world-state milestone", "verify_unified_save.py"],
}

for rel, tokens in required.items():
    path = ROOT / rel
    if not path.is_file():
        raise SystemExit(f"[FAIL] missing {rel}")
    text = path.read_text(encoding="utf-8")
    missing = [token for token in tokens if token not in text]
    if missing:
        raise SystemExit(f"[FAIL] {rel} missing hooks: {missing}")

cpp = (ROOT / "Source/GTT/Private/Save/GTTUnifiedSaveSubsystem.cpp").read_text(encoding="utf-8")
for slot in ["CombatSlotName", "StorySlotName", "Arc3SlotName", "Arc4SlotName", "FactionSlotName", "RuralEconomySlotName"]:
    if cpp.count(slot) < 2:
        raise SystemExit(f"[FAIL] {slot} does not appear in both migration directions")

roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
):
    if token not in roadmap:
        raise SystemExit(f"[FAIL] roadmap structure missing: {token}")
checks = re.findall(r'^- \[(x| )\]', roadmap, flags=re.M | re.IGNORECASE)
done = sum(state.lower() == "x" for state in checks)
total = len(checks)
if not total:
    raise SystemExit("[FAIL] roadmap checklist missing")
remaining = total - done
percent = round(done * 100.0 / total, 1)
expect = [
    f"DONE-{done}%2F{total}",
    f"{percent:.1f}%",
    f"| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |",
]
missing = [token for token in expect if token not in roadmap]
if missing:
    raise SystemExit(f"[FAIL] roadmap dashboard/checklist mismatch: {done}/{total} = {percent:.1f}% missing {missing}")
if roadmap.count("../assets/readme/progress-mini.svg") != 1:
    raise SystemExit("[FAIL] roadmap must embed exactly one progress-mini.svg")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    raise SystemExit("[FAIL] legacy text/Unicode roadmap progress meter must not return")
if "- [x] Consolidate combat/story/faction slots into primary sandbox SaveGame" not in roadmap:
    raise SystemExit("[FAIL] unified-save roadmap milestone is not checked")

print(f"[OK] unified world state structurally sane with schema v8 forward compatibility; roadmap {done}/{total} = {percent:.1f}% with SVG-only presentation.")
