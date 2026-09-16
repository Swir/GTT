#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

save_h = read("Source/GTT/Public/Save/GTTSaveGame.h")
logistics_h = read("Source/GTT/Public/World/GTTLogisticsReputationSubsystem.h")
logistics_cpp = read("Source/GTT/Private/World/GTTLogisticsReputationSubsystem.cpp")
farm_h = read("Source/GTT/Public/Activities/GTTFarmJobDirector.h")
farm_cpp = read("Source/GTT/Private/Activities/GTTFarmJobDirector.cpp")
terminal_h = read("Source/GTT/Public/Activities/GTTFarmJobTerminal.h")
board_cpp = read("Source/GTT/Private/World/GTTContractBoardTerminal.cpp")
dispatch_h = read("Source/GTT/Public/NPC/GTTLogisticsDispatcherPawn.h")
dispatch_cpp = read("Source/GTT/Private/NPC/GTTLogisticsDispatcherPawn.cpp")
workflow = read(".github/workflows/project-sanity.yml")
changelog = read("CHANGELOG.d/0.1.5.md")
playtest = read("Docs/PLAYTEST_0.1.5.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "save v8 remains additive": "SaveVersion = 8" in save_h and "0.1.4 extends v8 additively" in save_h and "0.1.5 extends" in save_h,
    "market state persisted": all(token in save_h for token in (
        "LogisticsMarketDay", "FeedDepotStock", "HillFarmDemand", "WoodYardDemand", "CargoRotationIndex")),
    "market API exposed": all(token in logistics_h for token in (
        "GetCargoCommodityLabel", "GetCargoStockSummary", "CanAcceptCargoContract", "ReserveCargoContract", "SettleCargoContract")),
    "daily market refresh deterministic": all(token in logistics_cpp for token in (
        "EnsureCargoMarketForCurrentDay", "GetDayNumber()", "CurrentDay + CargoCompletedRuns", "CargoRotationIndex =", "MarketDay = CurrentDay")),
    "original rotating commodities": all(token in logistics_cpp for token in ("ANIMAL FEED", "SEED PALLETS", "FARM PARTS")),
    "depot reservation consumes stock": "FeedDepotStock -= RequiredStock" in logistics_cpp and "RequiredStock = RouteTier >= 2 ? 3 : 2" in logistics_cpp,
    "successful settlement consumes demand": all(token in logistics_cpp for token in (
        "HillFarmDemand = FMath::Max", "WoodYardDemand = FMath::Max", "if (!bSuccess || ReservedUnits <= 0) return")),
    "market remains bounded": "FMath::Clamp(1.0f + TimeDemandBonus + DailyDemandBonus + ReputationBonus + StreakBonus, 1.0f, 1.38f)" in logistics_cpp,
    "market capture restore": all(token in logistics_cpp for token in (
        "Save->FeedDepotStock = FeedDepotStock", "Save->HillFarmDemand = HillFarmDemand", "Save->WoodYardDemand = WoodYardDemand",
        "FeedDepotStock = FMath::Clamp(Save->FeedDepotStock", "HillFarmDemand = FMath::Clamp(Save->HillFarmDemand", "WoodYardDemand = FMath::Clamp(Save->WoodYardDemand")),
    "dispatcher class exists": "AGTTLogisticsDispatcherPawn : public ACharacter" in dispatch_h and "ConfigureDispatcher" in dispatch_h,
    "dispatcher follows rural work shift": "WorkStartHour = 7.0f" in dispatch_h and "WorkEndHour = 17.5f" in dispatch_h and "ResolveScheduleTarget" in dispatch_cpp,
    "dispatcher uses real day night cycle": "AGTTDayNightCycle" in dispatch_h and "GetTimeOfDayHours" in dispatch_cpp,
    "terminal exposes type for worker placement": "GetTerminalType() const" in terminal_h,
    "director spawns all three roles": all(token in farm_cpp for token in (
        "FeedDepotDispatcher", "HillFarmReceiver", "WoodYardForeman", "SpawnActor<AGTTLogisticsDispatcherPawn>")),
    "director checks live market before accept": "CanAcceptCargoContract()" in farm_cpp and "CARGO MARKET HAS NO OPEN LOAD" in farm_cpp,
    "director reserves and saves accepted stock": "ReserveCargoContract(RouteTierAtStart, CargoUnitsReserved" in farm_cpp and "Inventory is authoritative as soon as the player accepts the load" in farm_cpp and "GameMode->SaveProgress()" in farm_cpp,
    "director names reserved commodity": "CargoCommodityAtStart" in farm_h and "CargoUnitsReserved" in farm_h and "%s x%d reserved" in farm_cpp,
    "director settles success and failure": "SettleCargoContract(CargoUnitsReserved, bExtendedRoute, true)" in farm_cpp and "SettleCargoContract(CargoUnitsReserved, RouteTierAtStart >= 2, false)" in farm_cpp,
    "legacy cargo dynamics retained": all(token in farm_cpp for token in (
        "SetCargoLoadFactor(1.0f)", "MarketMultiplierAtStart", "FleetPayoutMultiplier", "HANDOFF BLOCKED", "RecordCargoSuccess", "RecordCargoFailure")),
    "board exposes stock demand": "GetCargoStockSummary" in board_cpp and "CanAcceptCargoContract" in board_cpp and "STOCK/DEMAND FULL" in board_cpp,
    "board preserves prep while closed": "CARGO prep stays available while staff are off shift" in board_cpp,
    "playtest covers persistent living market": all(token in playtest.lower() for token in (
        "staffed depot", "reservation", "destination demand", "daily restock", "save compatibility", "win64/demo acceptance boundary")),
    "changelog documents milestone honesty": all(token in changelog for token in (
        "GTT 0.1.5", "Rural Dispatchers", "125/130 (96.2%)", "No demo Release is authorized")),
    "sanity wired": "Verify rural dispatchers, depot stock, and contract rotation" in workflow and "verify_rural_dispatch_stock.py" in workflow,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Rural dispatch/stock verification failed: " + ", ".join(failed))

required_style = [
    '<!-- SWIR-ROADMAP-STANDARD:v1 -->', '<!-- ROADMAP-PROGRESS:START -->',
    'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"',
    '## 📊 Overall progress', '<!-- ROADMAP-PROGRESS:END -->'
]
for token in required_style:
    if token not in roadmap:
        raise SystemExit("SWIR roadmap style lock missing: " + token)

items = re.findall(r'^- \[(x| )\] ', roadmap, flags=re.MULTILINE)
done = sum(v == 'x' for v in items)
total = len(items)
if not total:
    raise SystemExit("Roadmap checklist missing")
remaining = total - done
percent = round(done * 100.0 / total, 1)
filled = round(done * 20.0 / total)
bar = '█' * filled + '░' * (20 - filled)
for token in (
    f'ROADMAP-{percent:.1f}%25', f'DONE-{done}%2F{total}', 'STATUS-IN%20PROGRESS',
    f'{bar} {percent:.1f}%', f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |'):
    if token not in roadmap:
        raise SystemExit("Roadmap dashboard drift: missing " + token)
if (done, total, percent) != (125, 130, 96.2):
    raise SystemExit(f"0.1.5 source milestone must not claim build/art gates: {done}/{total} = {percent:.1f}%")

print(f"[OK] GTT 0.1.5 rural dispatchers, depot stock/demand and contract rotation verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.")
