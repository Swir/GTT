#!/usr/bin/env python3
"""Fast structural checks for GTT that do not require Unreal Engine to be installed."""
from __future__ import annotations

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "GTT.uproject",
    "Config/DefaultEngine.ini",
    "Config/DefaultGame.ini",
    "Config/DefaultInput.ini",
    "Source/GTT.Target.cs",
    "Source/GTTEditor.Target.cs",
    "Source/GTT/GTT.Build.cs",
    "Source/GTT/GTT.cpp",
    "Source/GTT/Public/Core/GTTGameMode.h",
    "Source/GTT/Public/Economy/GTTPlayerEconomyComponent.h",
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h",
    "Source/GTT/Public/Vehicles/GTTTractorPawn.h",
    "Source/GTT/Public/Vehicles/GTTOldCarPawn.h",
    "Source/GTT/Public/Vehicles/GTTFarmVanPawn.h",
    "Source/GTT/Public/Traffic/GTTTrafficCarPawn.h",
    "Source/GTT/Public/Traffic/GTTTrafficDirector.h",
    "Source/GTT/Public/Ranger/GTTRangerDirector.h",
    "Source/GTT/Public/Save/GTTSaveGame.h",
    "Source/GTT/Private/UI/GTTGameHUD.cpp",
    "Scripts/package_windows.ps1",
]

EXPECTED_SOURCE_TOKENS = {
    "Source/GTT/Public/Save/GTTSaveGame.h": [
        "FGTTStoredVehicleData", "SaveVersion = 2", "OwnedVehicles", "VehicleId"
    ],
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h": [
        "RegisterBreakablePart", "GetEngineTemperatureC", "GetDetachedPartCount",
        "GetFaultStatusText", "DamageSmokePuffA", "CriticalEngineTemperatureC",
        "LowConditionFaultChancePerSecond"
    ],
    "Source/GTT/Private/Vehicles/GTTVehicleBase.cpp": [
        "UpdateBreakableParts", "RestoreBreakableParts", "UpdateDamageSmoke",
        "TriggerMechanicalStall", "SetSimulatePhysics(true)", "ENGINE OVERHEAT",
        "ENGINE STALL", "EngineTemperatureC"
    ],
    "Source/GTT/Private/Vehicles/GTTTractorPawn.cpp": [
        "Rusty Fieldmaster 60", "RustyFieldmaster60", "LeftFenderMesh",
        "RightFenderMesh", "RegisterBreakablePart", "exhaust stack"
    ],
    "Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp": [
        "Rattleback 82", "Rattleback82", "LeftDoorMesh", "RightDoorMesh",
        "FrontBumperMesh", "RegisterBreakablePart", "right rear wheel"
    ],
    "Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp": [
        "Mulebox 1200", "Mulebox1200", "SlidingDoorMesh", "RearDoorLeftMesh",
        "RearDoorRightMesh", "RegisterBreakablePart"
    ],
    "Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp": [
        "LineTraceSingleByChannel", "ObstacleProbeDistance", "HornCooldownRemaining",
        "StuckRecoverySeconds", "AddImpulse", "Traffic vehicle - driver inside"
    ],
    "Source/GTT/Private/Traffic/GTTTrafficDirector.cpp": [
        "ClockwiseRoute", "CounterClockwiseRoute", "Algo::Reverse"
    ],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": [
        "WANTED [", "WARDEN [", "GARAGE %d/%d", "TEMP %.0fC",
        "VEHICLE DAMAGE", "DETACHED PARTS"
    ],
    "Source/GTT/Private/Core/GTTGameMode.cpp": [
        "SpawnActor<AGTTTrafficDirector>", "SpawnActor<AGTTRangerDirector>",
        "ReportWildlifeCrime", "TryRangerCitation", "OwnedVehicles",
        "TryRegisterVehicle", "SaveGameToSlot", "LoadGameFromSlot"
    ],
    "Source/GTT/Private/Economy/GTTPlayerEconomyComponent.cpp": [
        "ConfiscateAllFish", "ChargeFine", "SellAllFish"
    ],
    "Source/GTT/Private/Activities/GTTFishingSpot.cpp": [
        "ReportWildlifeCrime", "River Perch", "Village Carp", "Old Pike"
    ],
}


def fail(message: str) -> None:
    print(f"[FAIL] {message}")
    raise SystemExit(1)


def read_text(relative_path: str) -> str:
    return (ROOT / relative_path).read_text(encoding="utf-8")


def main() -> int:
    missing = [path for path in REQUIRED_FILES if not (ROOT / path).is_file()]
    if missing:
        fail("Missing required files: " + ", ".join(missing))

    try:
        project = json.loads(read_text("GTT.uproject"))
    except (OSError, json.JSONDecodeError) as exc:
        fail(f"GTT.uproject is not valid JSON: {exc}")

    if "GTT" not in {module.get("Name") for module in project.get("Modules", [])}:
        fail("GTT runtime module is not declared")

    enabled_plugins = {
        plugin.get("Name")
        for plugin in project.get("Plugins", [])
        if plugin.get("Enabled") is True
    }
    missing_plugins = {"EnhancedInput", "ChaosVehiclesPlugin"} - enabled_plugins
    if missing_plugins:
        fail("Required plugins are not enabled: " + ", ".join(sorted(missing_plugins)))

    inputs = read_text("Config/DefaultInput.ini")
    for token in ['ActionName="QuickSave"', 'Key=F5', 'ActionName="QuickLoad"', 'Key=F9']:
        if token not in inputs:
            fail(f"Input config missing {token}")

    for relative_path, tokens in EXPECTED_SOURCE_TOKENS.items():
        source = read_text(relative_path)
        absent = [token for token in tokens if token not in source]
        if absent:
            fail(f"{relative_path} is missing expected hooks: {absent}")

    persistent_sources = {
        "RustyFieldmaster60": read_text("Source/GTT/Private/Vehicles/GTTTractorPawn.cpp"),
        "Rattleback82": read_text("Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp"),
        "Mulebox1200": read_text("Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp"),
    }
    for vehicle_id, source in persistent_sources.items():
        if source.count(vehicle_id) != 1:
            fail(f"Persistent vehicle ID should appear exactly once in its vehicle source: {vehicle_id}")

    breakable_sources = {
        "tractor": read_text("Source/GTT/Private/Vehicles/GTTTractorPawn.cpp"),
        "old car": read_text("Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp"),
        "farm van": read_text("Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp"),
    }
    for vehicle_name, source in breakable_sources.items():
        count = source.count("RegisterBreakablePart(")
        if count < 4:
            fail(f"{vehicle_name} should expose at least four staged breakable parts, found {count}")

    if "TrafficCarCount = 6" not in read_text("Source/GTT/Public/Traffic/GTTTrafficDirector.h"):
        fail("Traffic director should default to six ambient cars for GTT 0.0.8")

    forbidden = ["Binaries", "Intermediate", "DerivedDataCache", "Saved"]
    present = [name for name in forbidden if (ROOT / name).exists()]
    if present:
        fail("Generated Unreal directories should not be committed: " + ", ".join(present))

    print("[OK] GTT 0.0.8 damage, faults, persistence, authorities and smarter traffic look structurally sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
