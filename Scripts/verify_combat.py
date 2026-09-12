#!/usr/bin/env python3
"""Structural verification for GTT 0.0.18 combat / rural arsenal milestone."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED = [
    "Source/GTT/Public/Combat/GTTCombatTypes.h",
    "Source/GTT/Private/Combat/GTTCombatTypes.cpp",
    "Source/GTT/Public/Combat/GTTCombatComponent.h",
    "Source/GTT/Private/Combat/GTTCombatComponent.cpp",
    "Source/GTT/Public/Combat/GTTWeaponPickup.h",
    "Source/GTT/Private/Combat/GTTWeaponPickup.cpp",
    "Source/GTT/Public/Combat/GTTCombatWorldSubsystem.h",
    "Source/GTT/Private/Combat/GTTCombatWorldSubsystem.cpp",
    "Source/GTT/Public/Activities/GTTBrawlDirector.h",
    "Source/GTT/Private/Activities/GTTBrawlDirector.cpp",
    "Source/GTT/Public/Activities/GTTBrawlTerminal.h",
    "Source/GTT/Private/Activities/GTTBrawlTerminal.cpp",
]

TOKENS = {
    "Source/GTT/Public/Combat/GTTCombatTypes.h": [
        "Pitchfork", "Axe", "Branch", "Rake", "CowChain", "Shovel", "WorkshopWrench", "FarmShotgun"
    ],
    "Source/GTT/Private/Combat/GTTCombatComponent.cpp": [
        "SweepMultiByChannel", "VRandCone", "AddCrimeHeat", "ApplyIncomingDamage", "HandleDefeat", "IsBrawlParticipant",
        "ApplyVehicleDamage", "ShotgunAmmo"
    ],
    "Source/GTT/Private/Combat/GTTWeaponPickup.cpp": ["AddWeapon", "Pick up", "Destroy"],
    "Source/GTT/Private/Combat/GTTCombatWorldSubsystem.cpp": [
        "Pitchfork", "Rake", "WorkshopWrench", "Axe", "Branch", "Shovel", "CowChain", "FarmShotgun",
        "AGTTBrawlDirector", "AGTTBrawlTerminal"
    ],
    "Source/GTT/Private/NPC/GTTCitizenPawn.cpp": [
        "ApplyCombatHit", "StartBrawlWith", "ApplyIncomingDamage", "bKnockedOut", "bBrawlParticipant", "LaunchCharacter"
    ],
    "Source/GTT/Private/Activities/GTTBrawlDirector.cpp": [
        "18.5f", "2.5f", "StartBrawlWith", "Brawlers", "TimeRemaining", "Reward", "BENT AXLE BRAWL"
    ],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": ["GetCombatStatusText", "GetHealthPercent", "AGTTBrawlDirector", "LMB attack", "Q next weapon", "G drop"],
    "Source/GTT/Private/Characters/GTTCharacter.cpp": ["CombatComponent", "Attack", "WeaponNext", "DropWeapon"],
}


def fail(msg: str) -> None:
    print(f"[FAIL] {msg}")
    raise SystemExit(1)


def main() -> int:
    missing = [path for path in REQUIRED if not (ROOT / path).is_file()]
    if missing:
        fail("Missing combat files: " + ", ".join(missing))

    for relative, tokens in TOKENS.items():
        path = ROOT / relative
        if not path.is_file():
            fail(f"Missing integration file: {relative}")
        text = path.read_text(encoding="utf-8")
        absent = [token for token in tokens if token not in text]
        if absent:
            fail(f"{relative} missing combat hooks: {absent}")

    inputs = (ROOT / "Config/DefaultInput.ini").read_text(encoding="utf-8")
    for token in ['ActionName="Attack"', 'Key=LeftMouseButton', 'ActionName="WeaponNext"', 'Key=Q', 'ActionName="DropWeapon"', 'Key=G']:
        if token not in inputs:
            fail(f"Combat input missing {token}")

    print("[OK] GTT 0.0.18 rural arsenal, player health, civilian reactions, wanted integration and Bent Axle brawl hooks look structurally sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
