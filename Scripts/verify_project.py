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
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h", "Source/GTT/Public/Vehicles/GTTTractorPawn.h",
    "Source/GTT/Public/Vehicles/GTTOldCarPawn.h", "Source/GTT/Public/Vehicles/GTTFarmVanPawn.h",
    "Source/GTT/Public/Wanted/GTTWantedComponent.h", "Source/GTT/Public/Police/GTTPoliceDirector.h",
    "Source/GTT/Public/Ranger/GTTRangerDirector.h", "Source/GTT/Public/Ranger/GTTRangerAIController.h",
    "Source/GTT/Public/Traffic/GTTTrafficCarPawn.h", "Source/GTT/Public/Traffic/GTTTrafficDirector.h",
    "Source/GTT/Public/NPC/GTTCitizenPawn.h", "Source/GTT/Public/Activities/GTTFishingSpot.h", "Source/GTT/Public/Activities/GTTFarmJobTerminal.h",
    "Source/GTT/Public/World/GTTMissionSafeZone.h", "Source/GTT/Public/World/GTTPrototypeWorld.h", "Source/GTT/Public/World/GTTServiceTerminal.h",
    "Source/GTT/Public/World/GTTGarageTerminal.h", "Source/GTT/Public/World/GTTDayNightCycle.h",
    "Source/GTT/Public/Save/GTTSaveGame.h", "Scripts/package_windows.ps1",
]

EXPECTED_SOURCE_TOKENS = {
    "Source/GTT/Public/Save/GTTSaveGame.h": ["FGTTStoredVehicleData", "SaveVersion = 2", "OwnedVehicles", "VehicleId"],
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h": [
        "RegisterBreakablePart", "GetEngineTemperatureC", "GetDetachedPartCount", "GetFaultStatusText",
        "DamageSmokePuffA", "CriticalEngineTemperatureC", "LowConditionFaultChancePerSecond"
    ],
    "Source/GTT/Private/Vehicles/GTTVehicleBase.cpp": [
        "UpdateBreakableParts", "RestoreBreakableParts", "UpdateDamageSmoke", "TriggerMechanicalStall",
        "SetSimulatePhysics(true)", "ENGINE OVERHEAT", "ENGINE STALL", "EngineTemperatureC"
    ],
    "Source/GTT/Private/Vehicles/GTTTractorPawn.cpp": [
        "Rusty Fieldmaster 60", "RustyFieldmaster60", "LeftFenderMesh", "RightFenderMesh", "RegisterBreakablePart", "exhaust stack"
    ],
    "Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp": [
        "Rattleback 82", "Rattleback82", "LeftDoorMesh", "RightDoorMesh", "FrontBumperMesh", "RegisterBreakablePart", "right rear wheel"
    ],
    "Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp": [
        "Mulebox 1200", "Mulebox1200", "SlidingDoorMesh", "RearDoorLeftMesh", "RearDoorRightMesh", "RegisterBreakablePart"
    ],
    "Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp": [
        "LineTraceSingleByChannel", "ObstacleProbeDistance", "BEEP!", "HornCooldownRemaining", "StuckRecoverySeconds", "AddImpulse"
    ],
    "Source/GTT/Private/Traffic/GTTTrafficDirector.cpp": ["ClockwiseRoute", "CounterClockwiseRoute", "Algo::Reverse"],
    "Source/GTT/Private/Economy/GTTPlayerEconomyComponent.cpp": ["AddFish", "SellAllFish", "ChargeFine", "ConfiscateAllFish", "RestoreState"],
    "Source/GTT/Private/NPC/GTTCitizenPawn.cpp": ["TryWitnessVehicleTheft", "GetScheduleCenter", "DayNightCycle", "called the police"],
    "Source/GTT/Private/Activities/GTTFishingSpot.cpp": ["ReportWildlifeCrime", "River Perch", "Village Carp", "Old Pike"],
    "Source/GTT/Private/Activities/GTTFarmJobTerminal.cpp": ["StartFarmJob", "CompleteFarmJob"],
    "Source/GTT/Private/Ranger/GTTRangerDirector.cpp": ["GetWildlifeAlertLevel", "SpawnActor<AGTTRangerPawn>"],
    "Source/GTT/Private/Ranger/GTTRangerAIController.cpp": ["TryRangerCitation", "MoveToActor"],
    "Source/GTT/Private/World/GTTGarageTerminal.cpp": ["RegistrationCost", "TryRegisterVehicle", "VehicleSearchRadius"],
    "Source/GTT/Private/World/GTTDayNightCycle.cpp": ["RealSecondsPerGameDay", "RestoreTime", "UpdateLighting", "DAY %d"],
    "Source/GTT/Private/Police/GTTPoliceAIController.cpp": ["MoveToActor(", "TryArrestPlayer", "ArrestRadius"],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": [
        "WANTED [", "WARDEN [", "GARAGE %d/%d", "TEMP %.0fC", "VEHICLE DAMAGE", "DETACHED PARTS"
    ],
    "Source/GTT/Private/World/GTTPrototypeWorld.cpp": [
        "4-SLOT GARAGE", "SpawnActor<AGTTOldCarPawn>", "SpawnActor<AGTTFarmVanPawn>", "SpawnActor<AGTTTrafficDirector>",
        "GAME WARDEN OUTPOST", "PRIVATE LAKE - NO FISHING"
    ],
    "Source/GTT/Private/Core/GTTGameMode.cpp": [
        "SaveGameToSlot", "LoadGameFromSlot", "OwnedVehicles", "TryRegisterVehicle", "GarageCapacity",
        "ReportWildlifeCrime", "TryRangerCitation", "GetWildlifeAlertLevel"
    ],
    "Source/GTT/Private/Characters/GTTCharacter.cpp": ["QuickSave", "QuickLoad", "SaveProgress", "LoadProgress"],
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
    for token in ['ActionName="QuickSave"', 'Key=F5', 'ActionName="QuickLoad"', 'Key=F9']:
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

    ids = {
        "RustyFieldmaster60": (ROOT / "Source/GTT/Private/Vehicles/GTTTractorPawn.cpp").read_text(encoding="utf-8"),
        "Rattleback82": (ROOT / "Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp").read_text(encoding="utf-8"),
        "Mulebox1200": (ROOT / "Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp").read_text(encoding="utf-8"),
    }
    for vehicle_id, source in ids.items():
        if source.count(vehicle_id) != 1:
            fail(f"Persistent vehicle ID should appear exactly once in its vehicle source: {vehicle_id}")

    breakable_counts = {
        "tractor": (ROOT / "Source/GTT/Private/Vehicles/GTTTractorPawn.cpp").read_text(encoding="utf-8").count("RegisterBreakablePart("),
        "old car": (ROOT / "Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp").read_text(encoding="utf-8").count("RegisterBreakablePart("),
        "farm van": (ROOT / "Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp").read_text(encoding="utf-8").count("RegisterBreakablePart("),
    }
    for vehicle, count in breakable_counts.items():
        if count < 4:
            fail(f"{vehicle} should expose at least four staged breakable parts, found {count}")

    forbidden = ["Binaries", "Intermediate", "DerivedDataCache", "Saved"]
    present = [n for n in forbidden if (ROOT / n).exists()]
    if present:
        fail("Generated Unreal directories should not be committed: " + ", ".join(present))

    print("[OK] GTT 0.0.8 vehicle damage, mechanical faults, traffic avoidance and existing gameplay hooks look structurally sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
