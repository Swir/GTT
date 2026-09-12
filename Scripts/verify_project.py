#!/usr/bin/env python3
"""Fast repository sanity checks that do not require Unreal Engine to be installed."""
from __future__ import annotations
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "GTT.uproject", "Config/DefaultEngine.ini", "Config/DefaultInput.ini",
    "Source/GTT/GTT.Build.cs", "Source/GTT/GTT.cpp",
    "Source/GTT/Public/Characters/GTTCharacter.h",
    "Source/GTT/Public/Core/GTTGameMode.h", "Source/GTT/Public/Core/GTTGameplayStatics.h",
    "Source/GTT/Public/Economy/GTTPlayerEconomyComponent.h",
    "Source/GTT/Public/Radio/GTTRadioComponent.h",
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h",
    "Source/GTT/Public/Police/GTTPoliceDirector.h", "Source/GTT/Public/Police/GTTPolicePursuitVehicle.h",
    "Source/GTT/Public/Police/GTTRoadblock.h",
    "Source/GTT/Public/World/GTTDayNightCycle.h", "Source/GTT/Public/World/GTTPrototypeWorld.h",
    "Source/GTT/Public/World/GTTVillageEventDirector.h", "Source/GTT/Public/World/GTTVillageEventMarker.h",
    "Source/GTT/Public/Save/GTTSaveGame.h", "Scripts/package_windows.ps1",
]

EXPECTED_SOURCE_TOKENS = {
    "Source/GTT/Public/Save/GTTSaveGame.h": ["SaveVersion = 3", "OwnedVehicles"],
    "Source/GTT/Private/Radio/GTTRadioComponent.cpp": [
        "GRAVEL FM", "BARNBEAT 96", "RUST & DIESEL", "NIGHT SHIFT", "AdvanceTrack", "RADIO OFF"
    ],
    "Source/GTT/Private/Characters/GTTCharacter.cpp": ["RadioComponent", "RadioNext", "CycleRadio"],
    "Source/GTT/Private/Core/GTTGameplayStatics.cpp": ["FindRadioComponentForPawn", "UGTTRadioComponent", "GetDriverPawn"],
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h": ["ApplyTireDamage", "CycleRadio", "GetTireIntegrity"],
    "Source/GTT/Private/Vehicles/GTTVehicleBase.cpp": ["RadioNext", "CycleRadio", "ApplyTireDamage", "TireGrip", "FLAT TIRE"],
    "Source/GTT/Private/Police/GTTPoliceDirector.cpp": [
        "DesiredRoadblocks", "RoadblockEscalationWantedLevel", "SpawnRoadblock", "SelectRoadblockTransform"
    ],
    "Source/GTT/Private/Police/GTTRoadblock.cpp": [
        "POLICE ROADBLOCK", "SPIKE STRIP", "ApplyTireDamage", "ApplyVehicleDamage", "SetResponseTier"
    ],
    "Source/GTT/Private/World/GTTVillageEventDirector.cpp": [
        "18.5f", "2.5f", "COMMUNITY HALL PARTY", "VILLAGE NIGHT", "SpawnNightEvent", "PartyCrowd", "waiting for the next bad idea"
    ],
    "Source/GTT/Private/World/GTTVillageEventMarker.cpp": [
        "BROKEN-DOWN NEIGHBOR", "MIDNIGHT TRACTOR MEET", "SUSPICIOUS BONFIRE RUN", "MYSTERY CRATE", "ReportWildlifeCrime"
    ],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": [
        "POLICE RESPONSE", "ROADBLOCKS", "INTERCEPTION MODE", "GetDisplayLine", "NightDirector", "R radio"
    ],
    "Source/GTT/Private/World/GTTPrototypeWorld.cpp": [
        "AGTTVillageEventDirector", "THE BENT AXLE TAVERN", "4+ WANTED: ROADBLOCKS", "GTT 0.0.11"
    ],
}


def fail(message: str) -> None:
    print(f"[FAIL] {message}")
    raise SystemExit(1)


def main() -> int:
    missing = [p for p in REQUIRED_FILES if not (ROOT / p).is_file()]
    if missing:
        fail("Missing required files: " + ", ".join(missing))

    try:
        project = json.loads((ROOT / "GTT.uproject").read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        fail(f"GTT.uproject is not valid JSON: {exc}")

    if "GTT" not in {m.get("Name") for m in project.get("Modules", [])}:
        fail("GTT runtime module is not declared")

    enabled = {p.get("Name") for p in project.get("Plugins", []) if p.get("Enabled") is True}
    missing_plugins = {"EnhancedInput", "ChaosVehiclesPlugin"} - enabled
    if missing_plugins:
        fail("Required plugins are not enabled: " + ", ".join(sorted(missing_plugins)))

    inputs = (ROOT / "Config/DefaultInput.ini").read_text(encoding="utf-8")
    for token in ['ActionName="RadioNext"', 'Key=R', 'ActionName="QuickSave"', 'Key=F5', 'ActionName="QuickLoad"', 'Key=F9']:
        if token not in inputs:
            fail(f"Input config missing {token}")

    for relative, tokens in EXPECTED_SOURCE_TOKENS.items():
        path = ROOT / relative
        if not path.is_file():
            fail(f"Missing gameplay source: {relative}")
        text = path.read_text(encoding="utf-8")
        absent = [t for t in tokens if t not in text]
        if absent:
            fail(f"{relative} is missing expected gameplay hooks: {absent}")

    roadblock_header = (ROOT / "Source/GTT/Public/Police/GTTPoliceDirector.h").read_text(encoding="utf-8")
    for token in ["GetActiveRoadblockCount", "RoadblockEscalationWantedLevel", "MaxRoadblocks"]:
        if token not in roadblock_header:
            fail(f"Police director header missing roadblock contract: {token}")

    radio_header = (ROOT / "Source/GTT/Public/Radio/GTTRadioComponent.h").read_text(encoding="utf-8")
    for token in ["CycleStation", "GetDisplayLine", "TrackTimeRemaining"]:
        if token not in radio_header:
            fail(f"Radio header missing contract: {token}")

    forbidden = ["Binaries", "Intermediate", "DerivedDataCache", "Saved"]
    present = [n for n in forbidden if (ROOT / n).exists()]
    if present:
        fail("Generated Unreal directories should not be committed: " + ", ".join(present))

    print("[OK] GTT 0.0.11 radio, village nightlife, random events, roadblocks and spike-strip hooks look structurally sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
