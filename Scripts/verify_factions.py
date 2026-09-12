#!/usr/bin/env python3
"""Structural checks for GTT 0.0.21 rural faction encounters."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

CHECKS = {
    "Source/GTT/Public/NPC/GTTCitizenPawn.h": [
        "EGTTHostileArchetype", "Scrapper", "Runner", "Bruiser", "Enforcer",
        "ConfigureHostileArchetype", "IsFactionHostile", "GetArchetypeLabel",
    ],
    "Source/GTT/Private/NPC/GTTCitizenPawn.cpp": [
        "MaxHealth=165.0f", "MaxWalkSpeed=365.0f", "RetaliationDamage=13.0f",
        "HostileArchetype==EGTTHostileArchetype::Civilian?2600.0f:4200.0f",
    ],
    "Source/GTT/Public/NPC/GTTRuralFactionDirector.h": [
        "RustDogs", "StoneCrows", "MudJackals", "GetActiveHostileCount",
        "GetFactionVictories", "GetObjectiveText", "GetThreatText",
    ],
    "Source/GTT/Private/NPC/GTTRuralFactionDirector.cpp": [
        "RUST DOGS SCRAP YARD", "STONE CROWS OLD QUARRY", "MUD JACKALS MARSH CAMP",
        "PressureBonus", "FactionVictories/2", "AMBUSH:", "Faction territory cleared",
        "150.0f", "SaveFactionProgress", "LoadFactionProgress",
        "UGTTGameplayStatics::FindEconomyComponentForPawn",
    ],
    "Source/GTT/Public/Save/GTTFactionSaveGame.h": [
        "FactionVictories", "RustDogsDefeated", "StoneCrowsDefeated", "MudJackalsDefeated",
    ],
    "Source/GTT/Public/World/GTTFactionWorldSubsystem.h": ["UWorldSubsystem", "OnWorldBeginPlay"],
    "Source/GTT/Private/World/GTTFactionWorldSubsystem.cpp": [
        "AGTTRuralFactionDirector", "SpawnActor<AGTTRuralFactionDirector>",
    ],
    "Source/GTT/Private/World/GTTRoadGraph.cpp": [
        "ScrapYardRoad", "OldQuarryRoad", "MarshCampRoad",
        "RUST DOGS SCRAP YARD ROAD", "STONE CROWS OLD QUARRY ROAD", "MUD JACKALS MARSH CAMP ROAD",
    ],
}


def fail(message: str) -> None:
    print(f"[FAIL] {message}")
    raise SystemExit(1)


def main() -> int:
    for relative, tokens in CHECKS.items():
        path = ROOT / relative
        if not path.is_file():
            fail(f"Missing faction milestone file: {relative}")
        text = path.read_text(encoding="utf-8")
        missing = [token for token in tokens if token not in text]
        if missing:
            fail(f"{relative} missing faction hooks: {missing}")

    road_text = (ROOT / "Source/GTT/Private/World/GTTRoadGraph.cpp").read_text(encoding="utf-8")
    if road_text.count("FGTTRoadNode") > 0:
        pass
    for link in ["Link(OutNodes,7,20)", "Link(OutNodes,11,21)", "Link(OutNodes,16,22)"]:
        if link not in road_text:
            fail(f"Faction road territory is disconnected: {link}")

    director = (ROOT / "Source/GTT/Private/NPC/GTTRuralFactionDirector.cpp").read_text(encoding="utf-8")
    if "BaseHostiles+PressureBonus" not in director or "BaseReward+FMath::Clamp(FactionVictories" not in director:
        fail("Faction progression must affect both encounter size and payout")

    print("[OK] GTT 0.0.21 hostile archetypes, persistent faction pressure, ambush territories and road expansion look structurally sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
