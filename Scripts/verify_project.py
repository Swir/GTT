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
    "Source/GTT/Public/Characters/GTTCharacter.h", "Source/GTT/Public/Core/GTTGameMode.h",
    "Source/GTT/Public/Economy/GTTPlayerEconomyComponent.h", "Source/GTT/Public/UI/GTTGameHUD.h",
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h", "Source/GTT/Public/Vehicles/GTTTractorPawn.h",
    "Source/GTT/Public/Vehicles/GTTOldCarPawn.h", "Source/GTT/Public/Vehicles/GTTFarmVanPawn.h",
    "Source/GTT/Public/Wanted/GTTWantedComponent.h", "Source/GTT/Public/Police/GTTPoliceDirector.h",
    "Source/GTT/Public/Police/GTTPolicePursuitVehicle.h", "Source/GTT/Public/Ranger/GTTRangerDirector.h",
    "Source/GTT/Public/Traffic/GTTTrafficDirector.h", "Source/GTT/Public/NPC/GTTCitizenPawn.h",
    "Source/GTT/Public/Activities/GTTFishingSpot.h", "Source/GTT/Public/Activities/GTTForestPoachingSpot.h",
    "Source/GTT/Public/Activities/GTTFarmJobTerminal.h", "Source/GTT/Public/Activities/GTTFarmJobDirector.h",
    "Source/GTT/Public/World/GTTMissionSafeZone.h", "Source/GTT/Public/World/GTTPrototypeWorld.h",
    "Source/GTT/Public/World/GTTServiceTerminal.h", "Source/GTT/Public/World/GTTGarageTerminal.h",
    "Source/GTT/Public/World/GTTTuningTerminal.h", "Source/GTT/Public/World/GTTDayNightCycle.h",
    "Source/GTT/Public/Save/GTTSaveGame.h", "Scripts/package_windows.ps1",
]

EXPECTED_SOURCE_TOKENS = {
    "Source/GTT/Public/Save/GTTSaveGame.h": ["SaveVersion = 3", "EngineUpgradeLevel", "TireUpgradeLevel", "TireIntegrity", "OwnedVehicles"],
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h": ["RecallToTransform", "InstallEngineUpgrade", "InstallTireUpgrade", "RepairTires", "GetTuningSummary", "GetTireIntegrity"],
    "Source/GTT/Private/Vehicles/GTTVehicleBase.cpp": ["EngineTunePower", "TireGrip", "TireLoss", "SetPhysicsLinearVelocity", "ENGINE OVERHEAT", "FLAT TIRE"],
    "Source/GTT/Private/World/GTTGarageTerminal.cpp": ["RecallNextOwnedVehicle", "GARAGE RECALL", "RecallToTransform", "cycle the fleet"],
    "Source/GTT/Private/World/GTTTuningTerminal.cpp": ["ENGINE TUNE", "HEAVY-DUTY TIRES", "InstallEngineUpgrade", "InstallTireUpgrade", "SaveProgress"],
    "Source/GTT/Private/Activities/GTTForestPoachingSpot.cpp": ["ReportWildlifeCrime", "forest hare", "wild boar", "red deer", "WARDEN ALERT"],
    "Source/GTT/Private/Activities/GTTFarmJobDirector.cpp": ["ReachPickup", "DeliverCargo", "DeliveryTimeLimit", "CargoIntegrity", "FAST BONUS", "CargoIntegrity = FMath::Max"],
    "Source/GTT/Private/Activities/GTTFarmJobTerminal.cpp": ["TryStartJob", "TryPickupCargo", "TryCompleteJob"],
    "Source/GTT/Private/Police/GTTPoliceDirector.cpp": ["DesiredVehicles", "VehicleEscalationWantedLevel", "SpawnPursuitVehicle", "SetResponseTier"],
    "Source/GTT/Private/Police/GTTPolicePursuitVehicle.cpp": ["PursuitAcceleration", "AddForce", "AddTorqueInRadians", "TryArrestPlayer", "WantedLevel < 3"],
    "Source/GTT/Private/Core/GTTGameMode.cpp": ["Save->SaveVersion = 3", "Stored.EngineUpgradeLevel", "Stored.TireUpgradeLevel", "Stored.TireIntegrity", "Save->SaveVersion >= 3", "TryRegisterVehicle", "GetWildlifeAlertLevel"],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": ["POLICE RESPONSE", "PURSUIT CARS", "GetObjectiveText", "TUNING | ENGINE L%d/3", "WARDEN ["],
    "Source/GTT/Private/World/GTTPrototypeWorld.cpp": ["AGTTFarmJobDirector", "FEED DEPOT / CARGO PICKUP", "HILL FARM / CARGO DELIVERY", "3+ WANTED: PATROL CARS JOIN PURSUIT", "GTT 0.0.10"],
    "Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp": ["LineTraceSingleByChannel", "BEEP!", "StuckRecoverySeconds"],
    "Source/GTT/Private/Ranger/GTTRangerAIController.cpp": ["TryRangerCitation", "MoveToActor"],
    "Source/GTT/Private/Police/GTTPoliceAIController.cpp": ["TryArrestPlayer", "MoveToActor"],
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

    vehicle_sources = {
        "RustyFieldmaster60": ROOT / "Source/GTT/Private/Vehicles/GTTTractorPawn.cpp",
        "Rattleback82": ROOT / "Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp",
        "Mulebox1200": ROOT / "Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp",
    }
    for vehicle_id, path in vehicle_sources.items():
        source = path.read_text(encoding="utf-8")
        if source.count(vehicle_id) != 1:
            fail(f"Persistent vehicle ID should appear exactly once in its vehicle source: {vehicle_id}")
        if source.count("RegisterBreakablePart(") < 4:
            fail(f"{vehicle_id} should still expose staged breakable body parts")

    forbidden = ["Binaries", "Intermediate", "DerivedDataCache", "Saved"]
    present = [n for n in forbidden if (ROOT / n).exists()]
    if present:
        fail("Generated Unreal directories should not be committed: " + ", ".join(present))

    print("[OK] GTT 0.0.10 police pursuit escalation and staged cargo-job hooks look structurally sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
