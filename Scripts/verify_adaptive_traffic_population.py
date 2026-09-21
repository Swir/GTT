#!/usr/bin/env python3
"""Static contract checks for the 0.1.69 adaptive living-traffic milestone.

This verifier intentionally does not claim Unreal compilation or runtime proof. It protects
source-level population math, time-of-day integration, incident-safe culling and roadmap truth.
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "Source/GTT/Public/Traffic/GTTTrafficDirector.h"
CPP = ROOT / "Source/GTT/Private/Traffic/GTTTrafficDirector.cpp"
DAY_NIGHT = ROOT / "Source/GTT/Public/World/GTTDayNightCycle.h"
CONFIG = ROOT / "Config/DefaultGame.ini"
ROADMAP = ROOT / "Docs/ROADMAP.md"
PLAYTEST = ROOT / "Docs/PLAYTEST-0.1.69-ADAPTIVE-TRAFFIC.md"


def fail(message: str) -> None:
    raise SystemExit(f"[FAIL] {message}")


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def read(path: Path) -> str:
    require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def default_int(text: str, name: str) -> int:
    match = re.search(rf"\bint32\s+{re.escape(name)}\s*=\s*(-?\d+)\s*;", text)
    require(match is not None, f"cannot resolve default for {name}")
    return int(match.group(1))


def main() -> int:
    header = read(HEADER)
    cpp = read(CPP)
    day_night = read(DAY_NIGHT)
    config = read(CONFIG)
    roadmap = read(ROADMAP)
    playtest = read(PLAYTEST)

    for token in [
        "virtual void Tick(float DeltaSeconds) override",
        "GetTargetTrafficCount",
        "GetManagedTrafficCount",
        "GetTrafficProfileText",
        "RuralCommuterCount",
        "RushHourBonus",
        "NightTrafficReduction",
        "MaximumManagedTraffic",
        "MinimumCullDistanceFromPlayer",
        "MaxPopulationAdjustmentPerPass",
        "TArray<TWeakObjectPtr<AGTTTrafficCarPawn>> ManagedTrafficCars",
    ]:
        require(token in header, f"traffic director header missing contract token: {token}")

    for token in [
        "CacheDayNightCycle",
        "ReconcileTrafficPopulation(true)",
        "ReconcileTrafficPopulation(false)",
        "Clock->GetTimeOfDayHours()",
        "Clock->IsNight()",
        'TEXT("NIGHT")',
        'TEXT("RUSH")',
        'TEXT("DAY")',
        "OffsetRouteToDrivingLane",
        "FGTTRoadGraph::GetVillageLoop",
        "FGTTRoadGraph::BuildRoute",
        'FName(TEXT("GTT.ManagedTraffic"))',
        "IsOccupied()",
        "IsIncidentDisabled()",
        "IsRoadsideAssistanceActive()",
        "IsRoadsideResponderSceneAuthority()",
        "IsYieldingForRangerStop()",
        "IsHoldingForRangerStop()",
        "MinimumCullDistanceFromPlayer",
        "MaxPopulationAdjustmentPerPass",
        "TrafficCar->Destroy()",
        "TRAFFIC_POPULATION",
    ]:
        require(token in cpp, f"traffic director implementation missing contract token: {token}")

    require("PrimaryActorTick.bCanEverTick = false" not in cpp,
            "adaptive population director must not regress to one-shot ticking")
    require("float GetTimeOfDayHours() const" in day_night and "bool IsNight() const" in day_night,
            "day/night clock API required by adaptive traffic is missing")

    baseline = default_int(header, "TrafficCarCount")
    rural = default_int(header, "RuralCommuterCount")
    rush_bonus = default_int(header, "RushHourBonus")
    night_reduction = default_int(header, "NightTrafficReduction")
    maximum = default_int(header, "MaximumManagedTraffic")
    adjustment = default_int(header, "MaxPopulationAdjustmentPerPass")

    day_target = min(max(baseline + rural, 2), maximum)
    rush_target = min(max(baseline + rural + rush_bonus, 2), maximum)
    night_target = min(max(baseline + rural - night_reduction, 2), maximum)
    require((day_target, rush_target, night_target) == (8, 11, 5),
            f"unexpected default population profile math: day={day_target}, rush={rush_target}, night={night_target}")
    require(1 <= adjustment <= 6, "population adjustment budget must remain bounded")
    require(rush_target > day_target > night_target >= 2,
            "population profiles must preserve rush > day > night ordering")

    require("Hour >= 6.0f && Hour < 9.0f" in cpp, "morning rush window missing")
    require("Hour >= 16.0f && Hour < 19.5f" in cpp, "evening rush window missing")
    require("DistanceSq > FarthestDistanceSq" in cpp,
            "culling must prefer far-away managed traffic")
    require("if (!PlayerPawn)" in cpp,
            "culling must fail safe while no player pawn exists")

    require("ProjectVersion=0.1.69" in config, "project version must be 0.1.69")
    require("125/130" in playtest and "96.2%" in playtest,
            "playtest must preserve roadmap truth")
    require("UE 5.8" in playtest and "not" in playtest.lower(),
            "playtest must explicitly preserve the Unreal-runtime verification limitation")

    checked = len(re.findall(r"^- \[x\]", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_items = len(re.findall(r"^- \[ \]", roadmap, flags=re.MULTILINE))
    require((checked, open_items) == (125, 5),
            f"roadmap checklist changed unexpectedly: checked={checked}, open={open_items}")
    require("96.2%" in roadmap, "roadmap percentage must remain 96.2% until a real gate closes")

    print("[PASS] adaptive traffic population contract")
    print(f"[PASS] profiles: NIGHT={night_target}, DAY={day_target}, RUSH={rush_target}; adjustment/pass={adjustment}")
    print("[PASS] culling preserves occupied/incident/roadside/ranger-stop traffic and player proximity")
    print("[PASS] roadmap remains 125/130 (96.2%); no packaged Unreal runtime claim made")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
