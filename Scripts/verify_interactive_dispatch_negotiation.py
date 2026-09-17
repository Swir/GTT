#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

save_h = read("Source/GTT/Public/Save/GTTSaveGame.h")
logistics_h = read("Source/GTT/Public/World/GTTLogisticsReputationSubsystem.h")
logistics_cpp = read("Source/GTT/Private/World/GTTLogisticsReputationSubsystem.cpp")
dispatch_h = read("Source/GTT/Public/NPC/GTTLogisticsDispatcherPawn.h")
dispatch_cpp = read("Source/GTT/Private/NPC/GTTLogisticsDispatcherPawn.cpp")
farm_cpp = read("Source/GTT/Private/Activities/GTTFarmJobDirector.cpp")
board_cpp = read("Source/GTT/Private/World/GTTContractBoardTerminal.cpp")
workflow = read(".github/workflows/project-sanity.yml")
changelog = read("CHANGELOG.d/0.1.7.md")
playtest = read("Docs/PLAYTEST_0.1.7.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "save schema remains additive v8": all(token in save_h for token in (
        "SaveVersion = 8", "0.1.7", "CargoNegotiatedOrderTier = 0", "CargoNegotiationDay = 0", "additive")),
    "negotiation API exposed": all(token in logistics_h for token in (
        "GetCargoNegotiationOptions", "GetCargoNegotiationOptionsLabel", "GetCargoNegotiationStatusLabel",
        "HasExplicitCargoNegotiation", "CycleCargoNegotiatedOrder", "ClearCargoNegotiatedOrder")),
    "market recommendation retained": all(token in logistics_cpp for token in (
        "GetRecommendedCargoOrderTier", "HillFarmDemand >= WoodYardDemand + 3", "CargoBacklogPressure >= 2")),
    "negotiated choice overrides only while valid": all(token in logistics_cpp for token in (
        "CargoNegotiationDay == GetDayNumber()", "IsCargoOrderTierAvailableInternal(CargoNegotiatedOrderTier)",
        "return CargoNegotiatedOrderTier", "CargoNegotiatedOrderTier = 0")),
    "options obey capability stock and demand": all(token in logistics_cpp for token in (
        "Tier > GetCargoRouteTier()", "FeedDepotStock < RequiredStock", "HillFarmDemand <= 0",
        "Tier >= 2 && WoodYardDemand <= 0", "for (int32 Tier = 1; Tier <= CapabilityTier; ++Tier)")),
    "three materially different risk choices": all(token in logistics_cpp for token in (
        "T1 DIRECT", "2 units", "LOW load", "T2 RELAY", "3 units", "+$70 route bonus",
        "T3 BULK", "4 units", "+$120 route bonus", "HEAVY 1.20x load")),
    "cycle selection is deterministic": all(token in logistics_cpp for token in (
        "Options.IndexOfByKey(CurrentTier)", "(CurrentIndex + 1) % Options.Num()",
        "CargoNegotiatedOrderTier = Options[NextIndex]", "CargoNegotiationDay = GetDayNumber()")),
    "choice persists and restores": all(token in logistics_cpp for token in (
        "Save->CargoNegotiatedOrderTier = CargoNegotiatedOrderTier",
        "Save->CargoNegotiationDay = CargoNegotiationDay",
        "CargoNegotiatedOrderTier = FMath::Clamp(Save->CargoNegotiatedOrderTier",
        "CargoNegotiationDay = FMath::Max(0, Save->CargoNegotiationDay)")),
    "choice expires on new work day": all(token in logistics_cpp for token in (
        "CargoNegotiationDay > 0 && CargoNegotiationDay != CurrentDay", "same-shift commitments")),
    "choice resets after success failure": logistics_cpp.count("ClearCargoNegotiatedOrder();") >= 2,
    "dispatcher is interactable": all(token in dispatch_h for token in (
        "public IGTTInteractable", "Interact_Implementation", "GetInteractionText_Implementation")),
    "feed dispatcher negotiates and saves": all(token in dispatch_cpp for token in (
        "FeedDepotDispatcherRole", "CycleCargoNegotiatedOrder", "FEED DISPATCH NEGOTIATION", "GameMode->SaveProgress()")),
    "receiver and foreman give live briefings": all(token in dispatch_cpp for token in (
        "HILL RECEIVER", "GetHillFarmDemand", "WOOD FOREMAN", "GetWoodYardDemand", "GetCargoBacklogPressure")),
    "off shift interaction is blocked": all(token in dispatch_cpp for token in (
        "if (!IsOnShift())", "off shift", "return;")),
    "world labels stay compact actionable": all(token in dispatch_cpp for token in (
        "E NEGOTIATE", "HILL NEED %d | E STATUS", "WOOD NEED %d | E STATUS")),
    "existing director consumes selected active tier": "RouteTierAtStart = Logistics->GetActiveCargoOrderTier()" in farm_cpp,
    "existing risk reward remains physical": all(token in farm_cpp for token in (
        "ReliableChainBonus", "TrustedChainBonus", "CargoLoadFactor = RouteTierAtStart >= 3 ? 1.20f : 1.0f",
        "BulkRouteExtraTime")),
    "board automatically reflects negotiated active order": all(token in board_cpp for token in (
        "CAP T%d / ORDER T%d", "GetActiveCargoOrderTier", "GetCargoOrderUnits", "GetCargoOrderPriorityLabel")),
    "changelog documents honest boundary": all(token in changelog for token in (
        "GTT 0.1.7", "Interactive Dispatchers", "125/130 (96.2%)", "No demo Release is authorized")),
    "playtest covers negotiation persistence risk": all(token in playtest.lower() for token in (
        "dispatcher negotiation", "t1 direct", "t2 relay", "t3 bulk", "save/reload", "next work day",
        "win64/demo acceptance boundary")),
    "sanity wired": "Verify interactive dispatchers and rural order negotiation" in workflow and "verify_interactive_dispatch_negotiation.py" in workflow,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Interactive dispatcher negotiation verification failed: " + ", ".join(failed))

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
    raise SystemExit(f"0.1.7 source milestone must not claim build/art gates: {done}/{total} = {percent:.1f}%")

print(f"[OK] GTT 0.1.7 interactive dispatchers, negotiated CARGO choices and persistence verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.")
