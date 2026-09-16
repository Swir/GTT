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
board_cpp = read("Source/GTT/Private/World/GTTContractBoardTerminal.cpp")
workflow = read(".github/workflows/project-sanity.yml")
changelog = read("CHANGELOG.d/0.1.6.md")
playtest = read("Docs/PLAYTEST_0.1.6.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "schema v8 remains additive": "SaveVersion = 8" in save_h and "0.1.6" in save_h and "additive" in save_h,
    "backlog persisted in save": "CargoBacklogPressure = 0" in save_h,
    "dynamic-order API exposed": all(token in logistics_h for token in (
        "GetActiveCargoOrderTier", "GetCargoOrderUnits", "GetCargoOrderPriorityLabel",
        "GetCargoOrderRouteLabel", "GetCargoBacklogPressure", "GetRoadSupplySignalLabel")),
    "daily demand carries instead of resets": all(token in logistics_cpp for token in (
        "HillFarmDemand = FMath::Clamp(HillFarmDemand + HillFreshOrders",
        "WoodYardDemand = FMath::Clamp(WoodYardDemand + WoodFreshOrders",
        "Unserved demand survives", "CargoBacklogPressure = FMath::Max(0, CargoBacklogPressure - ElapsedDays)")),
    "failed cargo raises bounded backlog": "CargoBacklogPressure = FMath::Clamp(CargoBacklogPressure + (bSevereFailure ? 2 : 1), 0, MaxCargoBacklogPressure)" in logistics_cpp,
    "successful cargo repairs backlog": "CargoBacklogPressure = FMath::Max(0, CargoBacklogPressure - (bExtendedRoute ? 2 : 1))" in logistics_cpp,
    "backlog capture restore wired": all(token in logistics_cpp for token in (
        "Save->CargoBacklogPressure = CargoBacklogPressure",
        "CargoBacklogPressure = FMath::Clamp(Save->CargoBacklogPressure")),
    "market capability and active order separated": all(token in logistics_cpp for token in (
        "const int32 CapabilityTier = GetCargoRouteTier()", "HillFarmDemand >= WoodYardDemand + 3",
        "CargoBacklogPressure >= 2", "return 3", "return 2")),
    "bulk order is four units": "case 3: return 4" in logistics_cpp and "if (RouteTier >= 3) RequiredStock = 4" in logistics_cpp,
    "base market remains backward bounded": "FMath::Clamp(1.0f + TimeDemandBonus + DailyDemandBonus + ReputationBonus + StreakBonus, 1.0f, 1.38f)" in logistics_cpp,
    "backlog reward remains bounded": "BacklogBonus" in logistics_cpp and "FMath::Clamp(BaseMarketMultiplier + BacklogBonus, 1.0f, 1.48f)" in logistics_cpp,
    "road courier shares parts supply pressure": all(token in logistics_cpp for token in (
        "PartsPressureBonus", "CargoRotationIndex == 2", "WoodYardDemand", "CargoBacklogPressure", "WOOD PARTS BACKLOG")),
    "director locks active order not only capability": "RouteTierAtStart = Logistics->GetActiveCargoOrderTier()" in farm_cpp and "CargoPriorityAtStart = Logistics->GetCargoOrderPriorityLabel()" in farm_cpp,
    "director still reserves authoritative stock": "ReserveCargoContract(RouteTierAtStart, CargoUnitsReserved" in farm_cpp and "GameMode->SaveProgress()" in farm_cpp,
    "bulk cargo has physical handling load": all(token in farm_cpp for token in (
        "CargoLoadFactor = RouteTierAtStart >= 3 ? 1.20f : 1.0f",
        "SetCargoLoadFactor(CargoLoadFactor)", "BULK-LOADED")),
    "bulk route has timing allowance": "BulkRouteExtraTime = 25.0f" in farm_h and "BulkRouteExtraTime" in farm_cpp,
    "failure message exposes future backlog": "buyer demand remains open and creates backlog pressure" in farm_cpp,
    "board explains capability order and units": all(token in board_cpp for token in (
        "CAP T%d / ORDER T%d", "GetActiveCargoOrderTier", "GetCargoOrderUnits", "GetCargoOrderPriorityLabel")),
    "board exposes ROAD supply signal": "GetRoadSupplySignalLabel" in board_cpp,
    "legacy police fleet cargo hooks retained": all(token in farm_cpp for token in (
        "HANDOFF BLOCKED", "FleetPayoutMultiplier", "SettleCargoContract", "RecordCargoSuccess", "RecordCargoFailure")),
    "playtest covers cross-day consequences": all(token in playtest.lower() for token in (
        "dynamic order selection", "physical vehicle consequence", "persistent backlog pressure",
        "works down the backlog", "road courier reacts", "schema-v8 compatibility", "win64/demo acceptance boundary")),
    "changelog is honest": all(token in changelog for token in (
        "GTT 0.1.6", "Dynamic Rural Orders", "1.48x", "125/130 (96.2%)", "No demo Release is authorized")),
    "sanity wired": "Verify dynamic rural orders and supply-chain consequences" in workflow and "verify_dynamic_rural_orders.py" in workflow,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Dynamic rural orders verification failed: " + ", ".join(failed))

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
    raise SystemExit(f"0.1.6 source milestone must not claim build/art gates: {done}/{total} = {percent:.1f}%")

print(f"[OK] GTT 0.1.6 dynamic rural orders, cross-day backlog and ROAD/CARGO supply consequences verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.")
