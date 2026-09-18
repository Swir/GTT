#!/usr/bin/env python3
"""Source-contract gate for GTT 0.1.12 priority route planning and consequence ledger.

This verifies repository integration only. It does not claim Unreal compilation, Win64 packaging,
packaged-EXE runtime smoke, driving feel, or rendered visual acceptance.
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    path = ROOT / rel
    assert path.is_file(), f"missing required file: {rel}"
    return path.read_text(encoding="utf-8")


def require(text: str, token: str, where: str) -> None:
    assert token in text, f"{where}: missing contract token {token!r}"


def verify_planner_model() -> None:
    header = read("Source/GTT/Public/World/GTTLogisticsRoutePlanner.h")
    cpp = read("Source/GTT/Private/World/GTTLogisticsRoutePlanner.cpp")
    save = read("Source/GTT/Public/Save/GTTSaveGame.h")
    for token in (
        "EGTTLogisticsPriorityLane", "FGTTLogisticsRoutePlan", "RoadScore", "CargoScore",
        "ConsequencePressure", "GetCargoConflictHoldCapMinutes", "BuildSummary",
    ):
        require(header, token, "route planner public contract")
    for token in (
        'const FName RoadRunJob(TEXT("RoadRun"))', 'const FName FarmCargoJob(TEXT("FarmCargo"))',
        "AssessJobReadiness(RoadRunJob)", "AssessJobReadiness(FarmCargoJob)", "GetRoadPriorityUrgency()",
        "GetCargoPriorityUrgency()", "GetCargoReservationCount()", "GetCargoBacklogPressure()",
        "IsRoadCourierWindowOpen()", "IsCargoDepotWindowOpen()", "GetWoodYardDemand()", "GetHillFarmDemand()",
    ):
        require(cpp, token, "live route-score inputs")
    for token in (
        "GetFailedRuns() - Logistics->GetCompletedRuns() / 3", "GetCargoFailedRuns() - Logistics->GetCargoCompletedRuns() / 3",
        ", 0, 4)", "Plan.BacklogPressure * 2",
    ):
        require(cpp, token, "bounded consequence ledger")
    for label in ("ROAD / RATTLEBACK 82", "CARGO / MULEBOX 1200", "SPLIT / CHOOSE COMMITMENT", "WAIT / SERVICE FLEET"):
        require(cpp, label, "planner recommendation states")
    require(save, "SaveVersion = 8", "save schema remains v8")
    for forbidden in ("PriorityRouteScore", "PriorityRouteChoice", "LogisticsConsequenceDebt"):
        assert forbidden not in save, f"planner must reconstruct state, not persist duplicate field {forbidden}"


def verify_conflict_hold_is_real_gameplay() -> None:
    planner = read("Source/GTT/Private/World/GTTLogisticsRoutePlanner.cpp")
    relationships = read("Source/GTT/Private/World/GTTDispatcherRelationshipSubsystem.cpp")
    logistics = read("Source/GTT/Private/World/GTTLogisticsReputationSubsystem.cpp")
    dispatcher = read("Source/GTT/Private/NPC/GTTLogisticsDispatcherPawn.cpp")
    for token in (
        "Plan.RoadUrgency <= 0 || Plan.CargoUrgency <= 0", "Plan.RecommendedLane != EGTTLogisticsPriorityLane::Road",
        "if (ScoreGap < 15) return 0", "ScoreGap >= 50 ? 0.55f : (ScoreGap >= 30 ? 0.65f : 0.75f)", "FMath::Max(10",
    ):
        require(planner, token, "cross-lane hold policy")
    for token in (
        "FMath::Min(RelationshipHoldMinutes, PrioritySlaMinutes)", "FGTTLogisticsRoutePlanner::GetCargoConflictHoldCapMinutes(GetWorld())",
        "FMath::Min(PriorityLimitedHold, ConflictHoldMinutes)", "ROUTE CONFLICT CAP %d MIN", "FGTTLogisticsRoutePlanner::BuildSummary(GetWorld())",
    ):
        require(relationships, token, "authoritative dispatcher hold integration")
    for token in (
        "FeedDepotStock -= RequiredStock", "FeedDepotStock = FMath::Clamp(FeedDepotStock +",
        "CargoBacklogPressure = FMath::Clamp(CargoBacklogPressure + 1", "without a second stock debit",
    ):
        require(logistics, token, "reservation/expiry regression")
    require(dispatcher, "Relationships->GetContractDeskSummary()", "dispatcher desk presentation")


def verify_previous_emergency_contract_survives() -> None:
    priority = read("Source/GTT/Private/World/GTTLogisticsPriorityDispatch.cpp")
    relationships = read("Source/GTT/Private/World/GTTDispatcherRelationshipSubsystem.cpp")
    for token in ("90.0f", "70.0f", "50.0f", "case 3: return 15", "case 2: return 25", "case 1: return 40"):
        require(priority, token, "0.1.11 emergency SLA regression")
    for token in ("if (Relationship >= 70) return 110", "if (Relationship >= 45) return 80", "if (Relationship >= 25) return 50", "return 35"):
        require(relationships, token, "relationship favor regression")


def verify_docs_ci_and_roadmap() -> None:
    changelog = read("CHANGELOG.d/0.1.12.md")
    playtest = read("Docs/PLAYTEST_0.1.12.md")
    workflow = read(".github/workflows/priority-route-planner-sanity.yml")
    roadmap = read("Docs/ROADMAP.md")
    for token in ("GTT 0.1.12", "75%/65%/55%", "SaveVersion remains 8", "No demo Release"):
        require(changelog, token, "0.1.12 changelog")
    for token in ("ROAD wins a real emergency conflict", "CARGO wins a real emergency conflict", "Expired hold changes future planning", "Win64/demo acceptance boundary"):
        require(playtest, token, "0.1.12 playtest")
    require(workflow, "python Scripts/verify_priority_route_planner.py", "dedicated 0.1.12 workflow")

    for token in (
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "<!-- ROADMAP-PROGRESS:START -->", "<!-- ROADMAP-PROGRESS:END -->",
        'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"', "## 📊 Overall progress",
        "../assets/readme/progress-mini.svg", "| ✅ Completed | ⏳ Remaining | 📦 Total | 🎯 Progress |",
    ):
        require(roadmap, token, "SWIR Roadmap Style Lock")

    checkboxes = re.findall(r"^\s*-\s+\[(x|X| )\]\s+", roadmap, re.M)
    completed = sum(1 for value in checkboxes if value.lower() == "x")
    total = len(checkboxes)
    remaining = total - completed
    assert total > 0, "roadmap checklist not found"
    percent = round(completed * 100.0 / total, 1)
    for token in (
        f"ROADMAP-{percent:.1f}%25", f"DONE-{completed}%2F{total}",
        f"| **{completed}** | **{remaining}** | **{total}** | **{percent:.1f}%** |",
    ):
        require(roadmap, token, "truthful roadmap dashboard")
    assert roadmap.count("../assets/readme/progress-mini.svg") == 1, "roadmap must embed exactly one progress-mini.svg"
    assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, re.M), "legacy text/Unicode roadmap progress meter must not return"
    assert (completed, remaining, total, percent) == (125, 5, 130, 96.2), (
        f"0.1.12 cannot close hardware/package blockers: {completed}/{total} = {percent:.1f}%"
    )


def main() -> None:
    verify_planner_model()
    verify_conflict_hold_is_real_gameplay()
    verify_previous_emergency_contract_survives()
    verify_docs_ci_and_roadmap()
    print("GTT 0.1.12 priority route planner + consequence ledger source contracts: OK")
    print("Boundary: no Unreal/Win64 compile, package, packaged runtime, or rendered visual claim is made by this gate.")


if __name__ == "__main__":
    main()
