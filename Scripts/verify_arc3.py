#!/usr/bin/env python3
"""Structural verification for GTT 0.0.19 Main Story Arc 3."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED = [
    "Source/GTT/Public/Missions/GTTArc3Director.h",
    "Source/GTT/Private/Missions/GTTArc3Director.cpp",
    "Source/GTT/Public/Missions/GTTArc3Terminal.h",
    "Source/GTT/Private/Missions/GTTArc3Terminal.cpp",
    "Source/GTT/Public/World/GTTArc3WorldSubsystem.h",
    "Source/GTT/Private/World/GTTArc3WorldSubsystem.cpp",
    "Source/GTT/Public/Save/GTTArc3Save.h",
]

TOKENS = {
    "Source/GTT/Public/Missions/GTTArc3Director.h": [
        "RedBarnApproach", "RedBarnFight", "RedBarnEvidence", "EscapePolice", "CountyDrop", "FinalFarm", "Completed",
        "EvidenceDeliveryReward", "Arc3CompletionReward", "EvidenceRaidHeat", "HostileCount"
    ],
    "Source/GTT/Private/Missions/GTTArc3Director.cpp": [
        "RED BARN RECKONING", "StartBrawlWith", "GetHostilesRemaining", "EvidenceRaidHeat", "AddHeat", "Mulebox1200",
        "GetConditionPercent() < 0.35f", "EvidenceDeliveryReward", "Arc3CompletionReward", "GetOwnedVehicleCount() < 3",
        "EGTTMainStoryStage::Completed", "FGTTRoadGraph::BuildRoute", "SaveGameToSlot"
    ],
    "Source/GTT/Private/Missions/GTTArc3Terminal.cpp": ["TryFarmContact", "TryRedBarn", "TryCountyDrop", "RED BARN / RECKONING"],
    "Source/GTT/Private/World/GTTArc3WorldSubsystem.cpp": [
        "AGTTArc3Director", "EGTTArc3TerminalType::FarmOffice", "EGTTArc3TerminalType::RedBarn", "EGTTArc3TerminalType::CountyDrop",
        "-5200.0f", "-4700.0f"
    ],
    "Source/GTT/Public/Save/GTTArc3Save.h": ["Arc3SaveVersion", "Arc3Stage"],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": ["AGTTArc3Director", "EGTTArc3Stage::Completed", "GetObjectiveText"],
}


def fail(msg: str) -> None:
    print(f"[FAIL] {msg}")
    raise SystemExit(1)


def main() -> int:
    missing = [path for path in REQUIRED if not (ROOT / path).is_file()]
    if missing:
        fail("Missing Arc 3 files: " + ", ".join(missing))

    for relative, tokens in TOKENS.items():
        path = ROOT / relative
        if not path.is_file():
            fail(f"Missing integration file: {relative}")
        text = path.read_text(encoding="utf-8")
        absent = [token for token in tokens if token not in text]
        if absent:
            fail(f"{relative} missing Arc 3 hooks: {absent}")

    print("[OK] GTT 0.0.19 Red Barn combat chapter, police escape, Mulebox evidence haul, persistent Arc 3 state and west-side expansion look structurally sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
