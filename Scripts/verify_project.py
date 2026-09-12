#!/usr/bin/env python3
"""Fast repository sanity checks that do not require Unreal Engine to be installed."""
from __future__ import annotations
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "GTT.uproject", "Config/DefaultEngine.ini", "Config/DefaultGame.ini", "Config/DefaultInput.ini",
    "Source/GTT.Target.cs", "Source/GTTEditor.Target.cs", "Source/GTT/GTT.Build.cs", "Source/GTT/GTT.cpp",
    "Source/GTT/Public/Characters/GTTCharacter.h", "Source/GTT/Public/Core/GTTGameplayStatics.h", "Source/GTT/Public/Core/GTTGameMode.h",
    "Source/GTT/Public/Economy/GTTPlayerEconomyComponent.h", "Source/GTT/Public/UI/GTTGameHUD.h",
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h", "Source/GTT/Public/Vehicles/GTTTractorPawn.h", "Source/GTT/Public/Vehicles/GTTOldCarPawn.h", "Source/GTT/Public/Vehicles/GTTFarmVanPawn.h",
    "Source/GTT/Public/Traffic/GTTTrafficCarPawn.h", "Source/GTT/Public/Traffic/GTTTrafficDirector.h",
    "Source/GTT/Public/Ranger/GTTRangerPawn.h", "Source/GTT/Public/Ranger/GTTRangerAIController.h", "Source/GTT/Public/Ranger/GTTRangerDirector.h",
    "Source/GTT/Public/Wanted/GTTWantedComponent.h", "Source/GTT/Public/Police/GTTPoliceDirector.h", "Source/GTT/Public/Police/GTTPoliceAIController.h", "Source/GTT/Public/Police/GTTPolicePawn.h",
    "Source/GTT/Public/NPC/GTTCitizenPawn.h", "Source/GTT/Public/Activities/GTTFishingSpot.h", "Source/GTT/Public/Activities/GTTFarmJobTerminal.h",
    "Source/GTT/Public/World/GTTMissionSafeZone.h", "Source/GTT/Public/World/GTTPrototypeWorld.h", "Source/GTT/Public/World/GTTServiceTerminal.h", "Source/GTT/Public/World/GTTGarageTerminal.h", "Source/GTT/Public/World/GTTDayNightCycle.h",
    "Source/GTT/Public/Save/GTTSaveGame.h", "Scripts/package_windows.ps1",
]

EXPECTED_SOURCE_TOKENS = {
    "Source/GTT/Public/Save/GTTSaveGame.h": ["FGTTStoredVehicleData", "SaveVersion = 2", "OwnedVehicles", "VehicleId"],
    "Source/GTT/Private/Vehicles/GTTVehicleBase.cpp": ["AddForce(", "AddTorqueInRadians(", "MarkOwnedByPlayer", "RestorePersistentState", "bTheftReported = false"],
    "Source/GTT/Private/Vehicles/GTTTractorPawn.cpp": ["Rusty Fieldmaster 60", "RustyFieldmaster60"],
    "Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp": ["Rattleback 82", "Rattleback82"],
    "Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp": ["Mulebox 1200", "Mulebox1200"],
    "Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp": ["InitializeRoute", "AddForce(", "AddTorqueInRadians(", "Traffic vehicle - driver inside"],
    "Source/GTT/Private/Traffic/GTTTrafficDirector.cpp": ["TrafficCarCount", "SpawnTrafficLoop", "SpawnActor<AGTTTrafficCarPawn>"],
    "Source/GTT/Private/Ranger/GTTRangerAIController.cpp": ["MoveToActor(", "GetWildlifeAlertLevel", "TryRangerCitation"],
    "Source/GTT/Private/Ranger/GTTRangerDirector.cpp": ["DesiredRangers", "SpawnActor<AGTTRangerPawn>", "GetWildlifeAlertLevel"],
    "Source/GTT/Private/Economy/GTTPlayerEconomyComponent.cpp": ["AddFish", "SellAllFish", "ChargeFine", "ConfiscateAllFish", "RestoreState"],
    "Source/GTT/Private/Activities/GTTFishingSpot.cpp": ["ReportWildlifeCrime", "River Perch", "Village Carp", "Old Pike"],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": ["WANTED [", "WARDEN [", "GARAGE %d/%d", "F5 save"],
    "Source/GTT/Private/World/GTTPrototypeWorld.cpp": ["LIVE VILLAGE TRAFFIC LOOP", "GAME WARDEN OUTPOST", "GTT 0.0.7"],
    "Source/GTT/Private/Core/GTTGameMode.cpp": ["AGTTTrafficDirector", "AGTTRangerDirector", "ReportWildlifeCrime", "TryRangerCitation", "WildlifeHeatDecayPerSecond", "ConfiscateAllFish"],
}


def fail(message: str) -> None:
    print(f"[FAIL] {message}")
    raise SystemExit(1)


def main() -> int:
    missing = [p for p in REQUIRED_FILES if not (ROOT / p).is_file()]
    if missing: fail("Missing required files: " + ", ".join(missing))
    try: project = json.loads((ROOT / "GTT.uproject").read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc: fail(f"GTT.uproject is not valid JSON: {exc}")
    if "GTT" not in {m.get("Name") for m in project.get("Modules", [])}: fail("GTT runtime module is not declared")
    enabled = {p.get("Name") for p in project.get("Plugins", []) if p.get("Enabled") is True}
    missing_plugins = {"EnhancedInput", "ChaosVehiclesPlugin"} - enabled
    if missing_plugins: fail("Required plugins are not enabled: " + ", ".join(sorted(missing_plugins)))
    for relative, tokens in EXPECTED_SOURCE_TOKENS.items():
        path = ROOT / relative
        if not path.is_file(): fail(f"Missing gameplay source: {relative}")
        text = path.read_text(encoding="utf-8")
        absent = [t for t in tokens if t not in text]
        if absent: fail(f"{relative} is missing expected gameplay hooks: {absent}")
    forbidden = ["Binaries", "Intermediate", "DerivedDataCache", "Saved"]
    present = [n for n in forbidden if (ROOT / n).exists()]
    if present: fail("Generated Unreal directories should not be committed: " + ", ".join(present))
    print("[OK] GTT 0.0.7 traffic and game-warden authority split looks structurally sane.")
    return 0


if __name__ == "__main__": sys.exit(main())
