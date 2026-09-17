#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

rel_h = read("Source/GTT/Public/World/GTTDispatcherRelationshipSubsystem.h")
rel_cpp = read("Source/GTT/Private/World/GTTDispatcherRelationshipSubsystem.cpp")
logistics_h = read("Source/GTT/Public/World/GTTLogisticsReputationSubsystem.h")
logistics_cpp = read("Source/GTT/Private/World/GTTLogisticsReputationSubsystem.cpp")
dispatch_cpp = read("Source/GTT/Private/NPC/GTTLogisticsDispatcherPawn.cpp")
board_cpp = read("Source/GTT/Private/World/GTTContractBoardTerminal.cpp")
save_h = read("Source/GTT/Public/Save/GTTSaveGame.h")
workflow = read(".github/workflows/project-sanity.yml")
changelog = read("CHANGELOG.d/0.1.8.md")
playtest = read("Docs/PLAYTEST_0.1.8.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "relationship subsystem API exists": all(token in rel_h for token in (
        "UGTTDispatcherRelationshipSubsystem", "GetFeedDispatcherRelationship", "GetHillReceiverRelationship",
        "GetWoodForemanRelationship", "GetCargoDeskAccessTier", "CanAccessCargoTier",
        "GetContractDeskSummary", "GetDispatcherReaction")),
    "relationships derive from persistent logistics record": all(token in rel_cpp for token in (
        "GetCargoCompletedRuns()", "GetCargoFailedRuns()", "GetCompletedRuns()", "GetFailedRuns()",
        "GetReputation()", "GetCleanStreak()", "FMath::Clamp(Score, 0, 100)")),
    "role formulas are materially different": all(token in rel_cpp for token in (
        "CargoWins * 6", "CargoLosses * 8", "CargoWins * 5", "CargoLosses * 7",
        "RoadWins * 3", "RoadLosses * 5")),
    "relationship tiers exposed": all(token in rel_cpp for token in (
        "PREFERRED", "TRUSTED", "KNOWN", "NEW DRIVER")),
    "desk tier progression is bounded": all(token in rel_cpp for token in (
        "Relationship >= 70 ? 3", "Relationship >= 35 ? 2 : 1",
        "FMath::Min(RelationshipTier", "Logistics->GetCargoRouteTier()")),
    "shared desk contains road and cargo": all(token in rel_cpp for token in (
        "GetCargoNegotiationOptions()", "Tier <= AccessTier", "ROAD OPEN", "ROAD CLOSED",
        "DESK CARGO %s")),
    "existing cargo market remains authoritative": all(token in logistics_cpp for token in (
        "IsCargoOrderTierAvailableInternal", "FeedDepotStock < RequiredStock", "HillFarmDemand <= 0",
        "Tier >= 2 && WoodYardDemand <= 0", "CargoNegotiatedOrderTier")),
    "dispatcher cycles authoritative order with trust filter": all(token in dispatch_cpp for token in (
        "GetCargoDeskAccessTier", "CycleCargoNegotiatedOrder", "GetActiveCargoOrderTier() <= AccessTier",
        "ClearCargoNegotiatedOrder", "Complete clean deliveries", "SaveProgress()")),
    "0.1.7 dispatcher contract remains visible": all(token in dispatch_cpp for token in (
        "FEED DISPATCH NEGOTIATION", "E NEGOTIATE", "HILL NEED %d | E STATUS", "WOOD NEED %d | E STATUS")),
    "dispatcher reactions reach all three staff roles": all(token in dispatch_cpp for token in (
        "GetFeedRelationshipLabel", "GetHillRelationshipLabel", "GetWoodRelationshipLabel",
        "GetDispatcherReaction(FeedDepotDispatcherRole)", "GetDispatcherReaction(HillFarmReceiverRole)",
        "GetDispatcherReaction(WoodYardForemanRole)")),
    "world labels remain compact": all(token in dispatch_cpp for token in (
        "E NEGOTIATE | DESK T%d", "HILL NEED %d | E STATUS", "WOOD NEED %d | E STATUS")),
    "cargo board gates acceptance by desk access": all(token in board_cpp for token in (
        "CanAccessCargoTier(RouteTier)", "BUILD DISPATCHER TRUST", "bCargoDeskAccess",
        "Offer.bCanAcceptNow && bCargoScheduleOpen && bCargoMarketOpen && bCargoDeskAccess")),
    "fleet preparation remains available before trust": board_cpp.find("else if (Offer.bNeedsPreparation)") < board_cpp.find("else if (JobTag == FarmCargoJob && !bCargoDeskAccess)"),
    "road board shares transport desk": board_cpp.count("GetContractDeskSummary()") >= 2,
    "relationship gate does not replace schedule market gates": all(token in board_cpp for token in (
        "IsCargoDepotWindowOpen", "CanAcceptCargoContract", "GetActiveCargoOrderTier",
        "Offer.bNeedsPreparation")),
    "save schema remains v8 without duplicate relationship fields": (
        "SaveVersion = 8" in save_h and "CargoNegotiatedOrderTier" in save_h and
        "DispatcherRelationship" not in save_h and "FeedDispatcherRelationship" not in save_h),
    "existing negotiation persistence retained": all(token in logistics_cpp for token in (
        "Save->CargoNegotiatedOrderTier = CargoNegotiatedOrderTier",
        "Save->CargoNegotiationDay = CargoNegotiationDay",
        "CargoNegotiationDay == GetDayNumber()")),
    "milestone docs describe verification boundary": all(token in changelog for token in (
        "0.1.8", "relationship", "SaveVersion remains 8", "No demo Release")) and all(token in playtest for token in (
        "0.1.8", "T2 trust gate", "Preferred-driver T3 access", "Win64/demo acceptance boundary")),
    "workflow runs milestone verifier": "python Scripts/verify_dispatcher_relationship_desk.py" in workflow,
}

checkboxes = re.findall(r"^\s*-\s*\[([x ])\]", roadmap, flags=re.MULTILINE | re.IGNORECASE)
done = sum(1 for value in checkboxes if value.lower() == "x")
total = len(checkboxes)
remaining = total - done
progress = round((done / total * 100.0), 1) if total else 0.0
filled = round(done * 20.0 / total) if total else 0
bar = "█" * filled + "░" * (20 - filled)
checks["roadmap checkbox truth is unchanged"] = (done, remaining, total, progress) == (125, 5, 130, 96.2)
checks["SWIR roadmap dashboard lock remains intact"] = all(token in roadmap for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "<!-- ROADMAP-PROGRESS:START -->", "<!-- ROADMAP-PROGRESS:END -->",
    'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"',
    "## 📊 Overall progress", f"ROADMAP-{progress:.1f}%25", f"DONE-{done}%2F{total}", "STATUS-IN%20PROGRESS",
    f"{bar} {progress:.1f}%", "| Completed | Remaining | Total | Progress |",
    f"| **{done}** | **{remaining}** | **{total}** | **{progress:.1f}%** |"))

failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(f"[{'OK' if ok else 'FAIL'}] {name}")

if failed:
    raise SystemExit("Dispatcher relationship desk verification failed: " + "; ".join(failed))

print(f"Verified GTT 0.1.8 dispatcher relationships and multi-order desk; roadmap {done}/{total} ({progress:.1f}%).")
