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
    "Source/GTT/Public/Vehicles/GTTVehicleDynamicsComponent.h",
    "Source/GTT/Private/Vehicles/GTTVehicleDynamicsComponent.cpp",
    "Source/GTT/Public/Police/GTTPoliceDirector.h", "Source/GTT/Public/Police/GTTPolicePursuitVehicle.h",
    "Source/GTT/Public/Police/GTTRoadblock.h",
    "Source/GTT/Public/Activities/GTTFarmJobDirector.h", "Source/GTT/Public/Activities/GTTFarmJobTerminal.h",
    "Source/GTT/Public/Activities/GTTRuralWorkDirector.h", "Source/GTT/Public/Activities/GTTRuralWorkTerminal.h",
    "Source/GTT/Public/Activities/GTTFieldCheckpoint.h",
    "Source/GTT/Public/Activities/GTTRecoveryDirector.h", "Source/GTT/Public/Activities/GTTRecoveryTerminal.h",
    "Source/GTT/Public/Activities/GTTRecoveryTargetVehicle.h",
    "Source/GTT/Public/Missions/GTTMainStoryDirector.h", "Source/GTT/Public/Missions/GTTMainStoryTerminal.h",
    "Source/GTT/Public/Missions/GTTNightFavorDirector.h", "Source/GTT/Public/Missions/GTTNightFavorTerminal.h",
    "Source/GTT/Public/World/GTTDayNightCycle.h", "Source/GTT/Public/World/GTTPrototypeWorld.h",
    "Source/GTT/Public/World/GTTVillageEventDirector.h", "Source/GTT/Public/World/GTTVillageEventMarker.h",
    "Source/GTT/Public/World/GTTMudZone.h", "Source/GTT/Public/World/GTTRecoveryWorldSubsystem.h",
    "Source/GTT/Public/World/GTTMainStoryWorldSubsystem.h", "Source/GTT/Public/World/GTTRoadGraph.h",
    "Source/GTT/Private/World/GTTRoadGraph.cpp",
    "Source/GTT/Public/World/GTTGarageTerminal.h", "Source/GTT/Public/World/GTTGarageSlotTerminal.h",
    "Source/GTT/Public/Save/GTTSaveGame.h", "Source/GTT/Public/Save/GTTMainStorySave.h", "Scripts/package_windows.ps1",
]

EXPECTED_SOURCE_TOKENS = {
    "Source/GTT/Public/Save/GTTSaveGame.h": ["SaveVersion = 3", "OwnedVehicles"],
    "Source/GTT/Public/Save/GTTMainStorySave.h": ["StorySaveVersion", "StoryStage"],
    "Source/GTT/Public/Vehicles/GTTVehicleDynamicsComponent.h": [
        "WheelBaseCm", "SuspensionRestLengthCm", "SpringStrength", "DamperStrength", "ForwardGearTopSpeedsKmh", "OffroadGripBias"
    ],
    "Source/GTT/Private/Vehicles/GTTVehicleDynamicsComponent.cpp": [
        "LineTraceSingleByChannel", "AddForceAtLocation", "GetPhysicsLinearVelocityAtPoint", "GroundContactCount",
        "UpdateGear", "MaxSpeedKmh", "ApplyTerrainModifier", "GetDynamicsSummary"
    ],
    "Source/GTT/Private/Vehicles/GTTVehicleBase.cpp": [
        "VehicleDynamics", "ConfigureDynamics", "RefreshDynamicsPower", "SetDriverInputs", "ApplyTerrainDynamicsModifier"
    ],
    "Source/GTT/Private/Vehicles/GTTTractorPawn.cpp": [
        "MaxSpeedKmh = 58.0f", "OffroadGripBias = 0.42f", "ForwardGearTopSpeedsKmh", "ConfigureDynamics"
    ],
    "Source/GTT/Private/Vehicles/GTTOldCarPawn.cpp": [
        "MaxSpeedKmh = 128.0f", "LateralGrip = 9.8f", "ForwardGearTopSpeedsKmh", "ConfigureDynamics"
    ],
    "Source/GTT/Private/Vehicles/GTTFarmVanPawn.cpp": [
        "MaxSpeedKmh = 104.0f", "SuspensionRestLengthCm = 52.0f", "ForwardGearTopSpeedsKmh", "ConfigureDynamics"
    ],
    "Source/GTT/Private/World/GTTRoadGraph.cpp": [
        "BuildGraph", "BuildRoute", "FindClosestNode", "WardenOutpost", "ForestDeep", "HillFarmNorth", "Link(OutNodes"
    ],
    "Source/GTT/Private/Traffic/GTTTrafficDirector.cpp": [
        "FGTTRoadGraph::GetVillageLoop", "FGTTRoadGraph::BuildRoute", "NorthWood", "HillFarm"
    ],
    "Source/GTT/Private/Radio/GTTRadioComponent.cpp": ["GRAVEL FM", "BARNBEAT 96", "RUST & DIESEL", "NIGHT SHIFT"],
    "Source/GTT/Private/Police/GTTPoliceDirector.cpp": [
        "DesiredRoadblocks", "RoadblockEscalationWantedLevel", "SpawnRoadblock", "BuildRuntimeRoadNetwork",
        "FGTTRoadGraph::GetNodes", "SelectInterceptionRoadNode", "InterceptPredictionSeconds", "LastInterceptionNodeIndex"
    ],
    "Source/GTT/Private/Police/GTTRoadblock.cpp": ["SPIKE STRIP", "ApplyTireDamage"],
    "Source/GTT/Private/World/GTTGarageTerminal.cpp": ["FleetSlotCount", "AGTTGarageSlotTerminal", "Use GARAGE SLOT 1-4"],
    "Source/GTT/Private/World/GTTGarageSlotTerminal.cpp": [
        "ResolveSlotVehicle", "RustyFieldmaster60", "Rattleback82", "Mulebox1200", "RecallServiceCost", "RecallToTransform"
    ],
    "Source/GTT/Private/World/GTTVillageEventDirector.cpp": ["18.5f", "2.5f", "COMMUNITY HALL PARTY", "SpawnNightEvent"],
    "Source/GTT/Private/Activities/GTTFarmJobDirector.cpp": ["FindNearbyWorkVehicle", "Park a working vehicle", "DeliveryTimeLimit", "CargoIntegrity"],
    "Source/GTT/Private/Activities/GTTRuralWorkDirector.cpp": [
        "TryStartTimber", "TryPickupTimber", "TryDeliverTimber", "TimberFastBonus", "TryStartMowing", "TryMowingPass", "RequiredMowingPasses", "AGTTTractorPawn"
    ],
    "Source/GTT/Private/Activities/GTTFieldCheckpoint.cpp": ["OnComponentBeginOverlap", "AGTTTractorPawn", "TryMowingPass"],
    "Source/GTT/Private/Activities/GTTRecoveryDirector.cpp": [
        "TryStartRecovery", "TryHookRecoveryVehicle", "TryFinishRecovery", "SetConstrainedComponents", "TOW LINE SNAPPED", "BaseReward", "FastBonus"
    ],
    "Source/GTT/Private/Activities/GTTRecoveryTargetVehicle.cpp": ["Disabled Mulebox", "Engine is dead", "PersistentVehicleId = NAME_None"],
    "Source/GTT/Private/World/GTTMudZone.cpp": [
        "GetOverlappingActors", "ApplyTerrainDynamicsModifier", "ApplyTireDamage", "DragStrength", "0.52f"
    ],
    "Source/GTT/Private/World/GTTRecoveryWorldSubsystem.cpp": [
        "AGTTRecoveryDirector", "EGTTRecoveryTerminalType::Workshop", "EGTTRecoveryTerminalType::Hook", "HillFarmMud", "ForestTrackMud"
    ],
    "Source/GTT/Private/Missions/GTTMainStoryDirector.cpp": [
        "COUNTY LEDGER", "BACKROAD DEAL", "BackroadPickupHeat", "EscapePolice", "GetOwnedVehicleCount() < 2",
        "TIMBER GHOSTS", "TryWardenBriefing", "TryForestCache", "ReportWildlifeCrime", "EscapeRanger",
        "TryHillFarmEvidence", "HasUsableOwnedTractor", "FGTTRoadGraph::BuildRoute", "StorySaveVersion = 2",
        "SaveGameToSlot", "LoadGameFromSlot", "18.5f", "2.5f"
    ],
    "Source/GTT/Private/Missions/GTTMainStoryTerminal.cpp": [
        "FarmOffice", "NorthWood", "VillageShop", "Tavern", "EastRoad", "Workshop", "WardenOutpost", "ForestCache", "HillFarm"
    ],
    "Source/GTT/Private/World/GTTMainStoryWorldSubsystem.cpp": [
        "AGTTMainStoryDirector", "FarmOffice", "NorthWood", "VillageShop", "EastRoad", "Workshop", "WardenOutpost", "ForestCache", "HillFarm"
    ],
    "Source/GTT/Private/Missions/GTTNightFavorDirector.cpp": [
        "18.5f", "2.5f", "CollectParts", "ReachNeighbor", "ReturnToTavern", "CompletionReward", "SaveProgress"
    ],
    "Source/GTT/Private/Missions/GTTNightFavorTerminal.cpp": ["Tavern", "Workshop", "Neighbor", "TryFinish"],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": [
        "POLICE RESPONSE", "ROADBLOCKS", "RuralWork", "MainStory", "VEHICLE DYNAMICS", "GetDynamicsSummary", "NightFavor", "R radio"
    ],
    "Source/GTT/Private/World/GTTPrototypeWorld.cpp": [
        "AGTTRuralWorkDirector", "AGTTNightFavorDirector", "NORTH WOOD YARD", "MOWING CONTRACT", "FIELD GATE", "NIGHT SHIFT FAVOR"
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

    build = (ROOT / "Source/GTT/GTT.Build.cs").read_text(encoding="utf-8")
    if '"ChaosVehicles"' not in build:
        fail("GTT runtime module must link ChaosVehicles")

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

    dynamics_header = (ROOT / "Source/GTT/Public/Vehicles/GTTVehicleDynamicsComponent.h").read_text(encoding="utf-8")
    for token in ["GetCurrentGear", "GetGroundContactCount", "GetAverageSuspensionCompression", "ApplyTerrainModifier"]:
        if token not in dynamics_header:
            fail(f"Vehicle dynamics component missing API: {token}")

    base_header = (ROOT / "Source/GTT/Public/Vehicles/GTTVehicleBase.h").read_text(encoding="utf-8")
    for token in ["DynamicsComponent", "GetDynamicsSummary", "ApplyTerrainDynamicsModifier", "ConfigureDynamics"]:
        if token not in base_header:
            fail(f"Vehicle base missing dynamics integration: {token}")

    story_header = (ROOT / "Source/GTT/Public/Missions/GTTMainStoryDirector.h").read_text(encoding="utf-8")
    for token in [
        "NorthWoodPickup", "ShopDelivery", "TavernMeet", "EastRoadPickup", "EscapePolice", "WorkshopDelivery", "FinalFarmMeet",
        "Arc1Completed", "WardenBriefing", "ForestCache", "EscapeRanger", "HillFarmEvidence", "Arc2FinalFarm", "Completed", "GTT_MainStory_01"
    ]:
        if token not in story_header:
            fail(f"Main story director header missing stage/save contract: {token}")

    road_graph_header = (ROOT / "Source/GTT/Public/World/GTTRoadGraph.h").read_text(encoding="utf-8")
    for token in ["FGTTRoadNode", "GetVillageLoop", "BuildRoute", "FindClosestNode", "GetNodeLabel"]:
        if token not in road_graph_header:
            fail(f"Shared road graph missing API: {token}")

    police_header = (ROOT / "Source/GTT/Public/Police/GTTPoliceDirector.h").read_text(encoding="utf-8")
    for token in ["GetRoadNodeCount", "GetLastInterceptionNodeLabel", "MinimumInterceptLeadDistance", "SelectPursuitInterceptTransform"]:
        if token not in police_header:
            fail(f"Police director header missing road interception API: {token}")

    garage_slot_header = (ROOT / "Source/GTT/Public/World/GTTGarageSlotTerminal.h").read_text(encoding="utf-8")
    for token in ["SetSlotIndex", "GetSlotIndex", "RecallServiceCost"]:
        if token not in garage_slot_header:
            fail(f"Garage slot selector missing API: {token}")

    recovery_header = (ROOT / "Source/GTT/Public/Activities/GTTRecoveryDirector.h").read_text(encoding="utf-8")
    for token in ["ReachBreakdown", "HookVehicle", "TowToWorkshop", "GetTowCableLoad"]:
        if token not in recovery_header:
            fail(f"Recovery director header missing stage/API: {token}")

    forbidden = ["Binaries", "Intermediate", "DerivedDataCache", "Saved"]
    present = [n for n in forbidden if (ROOT / n).exists()]
    if present:
        fail("Generated Unreal directories should not be committed: " + ", ".join(present))

    print("[OK] GTT 0.0.17 vehicle dynamics, four-point suspension, differentiated gearing, mud traction and existing sandbox/story hooks look structurally sane.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
