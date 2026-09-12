#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

required = {
    "Source/GTT/Public/Save/GTTSaveGame.h": [
        "SaveVersion = 4", "bUnifiedWorldStateInitialized", "UnifiedWorldStateRevision",
        "CombatWeaponTypes", "CombatEquippedWeaponType", "CombatShotgunAmmo",
        "MainStoryStage", "Arc3Stage", "Arc4Stage", "Arc4StartingFactionVictories",
        "bArc4ContrabandPrepared", "FactionVictories", "RustDogsDefeated",
        "StoneCrowsDefeated", "MudJackalsDefeated", "ContrabandUnits", "ContrabandValue",
        "bInsuranceActive", "ImpoundedVehicleId", "PendingImpoundFee",
        "LifetimeFenceRevenue", "SpeedingCitations"
    ],
    "Source/GTT/Public/Save/GTTUnifiedSaveSubsystem.h": [
        "GTT_Prototype_01", "GTT_Combat_01", "GTT_MainStory_01", "GTT_MainStory_Arc3_01",
        "GTT_MainStory_Arc4_01", "GTT_Factions_01", "GTT_RuralEconomy_01",
        "ConsolidateLegacySlots", "HydrateCompatibilityMirrors"
    ],
    "Source/GTT/Private/Save/GTTUnifiedSaveSubsystem.cpp": [
        "Never manufacture a primary save on a brand-new profile", "SaveVersion = 4",
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

# Every domain must have both a legacy -> primary import and a primary -> compatibility-mirror write path.
cpp = (ROOT / "Source/GTT/Private/Save/GTTUnifiedSaveSubsystem.cpp").read_text(encoding="utf-8")
for slot in ["CombatSlotName", "StorySlotName", "Arc3SlotName", "Arc4SlotName", "FactionSlotName", "RuralEconomySlotName"]:
    if cpp.count(slot) < 2:
        raise SystemExit(f"[FAIL] {slot} does not appear in both migration directions")

# Enforce the SWIR roadmap dashboard against the real checklist, including the 20-cell bar.
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
    raise SystemExit("[FAIL] SWIR roadmap standard marker missing")
checks = re.findall(r'^- \[(x| )\]', roadmap, flags=re.M)
done = sum(state == "x" for state in checks)
total = len(checks)
if not total:
    raise SystemExit("[FAIL] roadmap checklist missing")
remaining = total - done
percent = round(done * 100.0 / total, 1)
filled = round(done * 20.0 / total)
bar = "█" * filled + "░" * (20 - filled)
expect = [
    f"DONE-{done}%2F{total}", f"{percent:.1f}%", f"**{done}**", f"**{remaining}**", f"**{total}**",
    f"{bar} {percent:.1f}%"
]
missing = [token for token in expect if token not in roadmap]
if missing:
    raise SystemExit(f"[FAIL] roadmap dashboard/checklist mismatch: {done}/{total} = {percent:.1f}% missing {missing}")

if "- [x] Consolidate combat/story/faction slots into primary sandbox SaveGame" not in roadmap:
    raise SystemExit("[FAIL] unified-save roadmap milestone is not checked")

print(f"[OK] unified world state structurally sane; roadmap {done}/{total} = {percent:.1f}% ({filled}/20 cells).")
