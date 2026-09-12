#!/usr/bin/env python3
"""Fast repository sanity checks that do not require Unreal Engine to be installed."""
from __future__ import annotations
import json
from pathlib import Path
import sys
ROOT = Path(__file__).resolve().parents[1]
REQUIRED_FILES = [
    "GTT.uproject","Config/DefaultEngine.ini","Config/DefaultGame.ini","Config/DefaultInput.ini",
    "Source/GTT.Target.cs","Source/GTTEditor.Target.cs","Source/GTT/GTT.Build.cs","Source/GTT/GTT.cpp",
    "Source/GTT/Public/Characters/GTTCharacter.h","Source/GTT/Public/Core/GTTGameplayStatics.h","Source/GTT/Public/Core/GTTGameMode.h",
    "Source/GTT/Public/Economy/GTTPlayerEconomyComponent.h","Source/GTT/Public/UI/GTTGameHUD.h",
    "Source/GTT/Public/Vehicles/GTTVehicleBase.h","Source/GTT/Public/Vehicles/GTTTractorPawn.h",
    "Source/GTT/Public/Wanted/GTTWantedComponent.h","Source/GTT/Public/Police/GTTPoliceDirector.h","Source/GTT/Public/Police/GTTPoliceAIController.h","Source/GTT/Public/Police/GTTPolicePawn.h",
    "Source/GTT/Public/NPC/GTTCitizenPawn.h","Source/GTT/Public/Activities/GTTFishingSpot.h","Source/GTT/Public/Activities/GTTFarmJobTerminal.h",
    "Source/GTT/Public/World/GTTMissionSafeZone.h","Source/GTT/Public/World/GTTPrototypeWorld.h","Source/GTT/Public/World/GTTServiceTerminal.h","Source/GTT/Public/World/GTTGarageTerminal.h","Source/GTT/Public/World/GTTDayNightCycle.h",
    "Source/GTT/Public/Save/GTTSaveGame.h","Scripts/package_windows.ps1",
]
EXPECTED_SOURCE_TOKENS = {
    "Source/GTT/Private/Vehicles/GTTVehicleBase.cpp": ["AddForce(","AddTorqueInRadians(","CurrentFuelLiters","MarkOwnedByPlayer","RestorePersistentState","bOwnedByPlayer"],
    "Source/GTT/Private/Vehicles/GTTTractorPawn.cpp": ["Rusty Fieldmaster 60","PersistentVehicleId","FuelCapacityLiters = 55.0f","StartingFuelLiters = 18.0f"],
    "Source/GTT/Private/Economy/GTTPlayerEconomyComponent.cpp": ["AddFish","SellAllFish","ChargeFine","RestoreState"],
    "Source/GTT/Private/NPC/GTTCitizenPawn.cpp": ["TryWitnessVehicleTheft","GetScheduleCenter","DayNightCycle","called the police"],
    "Source/GTT/Private/Activities/GTTFishingSpot.cpp": ["RestrictedFishingHeat","River Perch","Village Carp","Old Pike"],
    "Source/GTT/Private/Activities/GTTFarmJobTerminal.cpp": ["StartFarmJob","CompleteFarmJob"],
    "Source/GTT/Private/World/GTTGarageTerminal.cpp": ["RegistrationCost","MarkOwnedByPlayer","SaveProgress"],
    "Source/GTT/Private/World/GTTDayNightCycle.cpp": ["RealSecondsPerGameDay","RestoreTime","UpdateLighting","DAY %d"],
    "Source/GTT/Private/Police/GTTPoliceAIController.cpp": ["MoveToActor(","TryArrestPlayer","ArrestRadius"],
    "Source/GTT/Private/UI/GTTGameHUD.cpp": ["WANTED [","CASH $","FUEL %.0f%%","F5 save","GetClockText","OWNED"],
    "Source/GTT/Private/World/GTTPrototypeWorld.cpp": ["GARAGE REGISTER / SAVE","LEGAL FARM JOB START","FIELD DELIVERY / LEGAL JOB","SpawnActor<AGTTCitizenPawn>","PRIVATE LAKE - NO FISHING"],
    "Source/GTT/Private/Core/GTTGameMode.cpp": ["SaveGameToSlot","LoadGameFromSlot","TryArrestPlayer","StartFarmJob","CompleteFarmJob","MarkOwnedByPlayer","SpawnActor<AGTTDayNightCycle>"],
    "Source/GTT/Private/Characters/GTTCharacter.cpp": ["QuickSave","QuickLoad","SaveProgress","LoadProgress"],
}
def fail(message: str) -> None:
    print(f"[FAIL] {message}")
    raise SystemExit(1)
def main() -> int:
    missing=[p for p in REQUIRED_FILES if not (ROOT/p).is_file()]
    if missing: fail("Missing required files: "+", ".join(missing))
    try: project=json.loads((ROOT/"GTT.uproject").read_text(encoding="utf-8"))
    except (OSError,json.JSONDecodeError) as exc: fail(f"GTT.uproject is not valid JSON: {exc}")
    if "GTT" not in {m.get("Name") for m in project.get("Modules",[])}: fail("GTT runtime module is not declared")
    enabled={p.get("Name") for p in project.get("Plugins",[]) if p.get("Enabled") is True}
    missing_plugins={"EnhancedInput","ChaosVehiclesPlugin"}-enabled
    if missing_plugins: fail("Required plugins are not enabled: "+", ".join(sorted(missing_plugins)))
    inputs=(ROOT/"Config/DefaultInput.ini").read_text(encoding="utf-8")
    for token in ['ActionName="QuickSave"','Key=F5','ActionName="QuickLoad"','Key=F9']:
        if token not in inputs: fail(f"Input config missing {token}")
    for relative,tokens in EXPECTED_SOURCE_TOKENS.items():
        path=ROOT/relative
        if not path.is_file(): fail(f"Missing gameplay source: {relative}")
        text=path.read_text(encoding="utf-8")
        absent=[t for t in tokens if t not in text]
        if absent: fail(f"{relative} is missing expected gameplay hooks: {absent}")
    forbidden=["Binaries","Intermediate","DerivedDataCache","Saved"]
    present=[n for n in forbidden if (ROOT/n).exists()]
    if present: fail("Generated Unreal directories should not be committed: "+", ".join(present))
    print("[OK] GTT 0.0.5 persistence, ownership, arrest, time and farm-job hooks look structurally sane.")
    return 0
if __name__=="__main__": sys.exit(main())
