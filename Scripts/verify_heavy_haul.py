#!/usr/bin/env python3
"""Structural checks for GTT 0.0.20 articulated trailer/heavy-haul milestone."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

CHECKS = {
    "Source/GTT/Public/Vehicles/GTTFarmTrailer.h": [
        "AttachToVehicle", "DetachTrailer", "GetCargoIntegrity", "GetTrailerIntegrity", "GetHitchLoad"
    ],
    "Source/GTT/Private/Vehicles/GTTFarmTrailer.cpp": [
        "UPhysicsConstraintComponent", "SetConstrainedComponents", "BreakConstraint", "BreakHitchDistance",
        "CargoIntegrity", "SetMassOverrideInKg", "SpeedKmh", "Roll", "Pitch"
    ],
    "Source/GTT/Public/Activities/GTTHeavyHaulDirector.h": [
        "HitchTrailer", "ReachWoodYard", "LoadTimber", "DeliverHillFarm", "BaseReward", "FastBonus"
    ],
    "Source/GTT/Private/Activities/GTTHeavyHaulDirector.cpp": [
        "TryStartContract", "TryHitchTrailer", "TryLoadTimber", "TryDeliverTimber",
        "AGTTTractorPawn", "IsOwnedByPlayer", "GetConditionPercent", "GetCargoIntegrity",
        "GetTrailerIntegrity", "SaveProgress", "NORTH WOOD", "HILL FARM"
    ],
    "Source/GTT/Private/Activities/GTTHeavyHaulTerminal.cpp": [
        "ContractBoard", "Hitch", "Load", "Deliver", "HEAVY TIMBER"
    ],
    "Source/GTT/Private/World/GTTHeavyHaulWorldSubsystem.cpp": [
        "AGTTHeavyHaulDirector", "EGTTHeavyHaulTerminalType::ContractBoard",
        "EGTTHeavyHaulTerminalType::Hitch", "EGTTHeavyHaulTerminalType::Load",
        "EGTTHeavyHaulTerminalType::Deliver"
    ],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": [
        "GTTHeavyHaulDirector", "HeavyHaul->IsActive()", "HeavyHaul->GetObjectiveText()"
    ],
    "Docs/PLAYTEST_0.0.20.md": [
        "HEAVY HAUL CONTRACT", "TRAILER HITCH", "980 kg", "1680 kg", "cargo integrity"
    ],
}


def fail(message: str) -> None:
    print(f"[FAIL] {message}")
    raise SystemExit(1)


def main() -> int:
    for relative, tokens in CHECKS.items():
        path = ROOT / relative
        if not path.is_file():
            fail(f"Missing heavy-haul source: {relative}")
        text = path.read_text(encoding="utf-8")
        missing = [token for token in tokens if token not in text]
        if missing:
            fail(f"{relative} missing milestone hooks: {missing}")

    trailer = (ROOT / "Source/GTT/Private/Vehicles/GTTFarmTrailer.cpp").read_text(encoding="utf-8")
    if "1680.0f" not in trailer or "980.0f" not in trailer:
        fail("Trailer must change physical mass between empty and loaded states")

    director = (ROOT / "Source/GTT/Private/Activities/GTTHeavyHaulDirector.cpp").read_text(encoding="utf-8")
    if "ContractTimeLimit = 330.0f" not in (ROOT / "Source/GTT/Public/Activities/GTTHeavyHaulDirector.h").read_text(encoding="utf-8"):
        fail("Heavy haul contract must retain a real delivery timer")
    if "CargoFactor" not in director or "TrailerFactor" not in director or "VehicleFactor" not in director:
        fail("Payout must depend on cargo, trailer and tow-vehicle condition")

    changelog = (ROOT / "CHANGELOG.md").read_text(encoding="utf-8")
    roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
    for token in ["## [0.0.20]", "Heavy Timber Haul", "articulated"]:
        if token not in changelog:
            fail(f"CHANGELOG missing 0.0.20 milestone token: {token}")
    for token in ["Articulated rigid-body farm trailer", "Dedicated articulated-trailer heavy timber work contract"]:
        if token not in roadmap:
            fail(f"ROADMAP missing completed heavy-haul item: {token}")

    print("[OK] GTT 0.0.20 articulated trailer, hitch failure, heavy cargo stress, HUD, docs and economy payout hooks look structurally sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
