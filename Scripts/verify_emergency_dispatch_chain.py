#!/usr/bin/env python3
"""Source-contract gate for GTT 0.1.11 emergency dispatch windows and priority chains.

This verifies repository integration only. It does not claim Unreal compilation, Win64 packaging,
packaged-EXE runtime smoke, rendered handling feel, or visual acceptance.
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


def verify_priority_chain_model() -> None:
    header = read("Source/GTT/Public/World/GTTLogisticsReputationSubsystem.h")
    cpp = read("Source/GTT/Private/World/GTTLogisticsPriorityDispatch.cpp")
    reputation = read("Source/GTT/Private/World/GTTLogisticsReputationSubsystem.cpp")
    save = read("Source/GTT/Public/Save/GTTSaveGame.h")

    for api in (
        "GetPriorityChainStreak() const",
        "GetPriorityChainRewardMultiplier() const",
        "GetRoadPriorityPickupSlaSeconds() const",
        "GetCargoPriorityPickupSlaMinutes() const",
        "GetPriorityChainSummary() const",
    ):
        require(header, api, "0.1.11 logistics header")
        require(cpp, f"UGTTLogisticsReputationSubsystem::{api}", "0.1.11 priority implementation")

    for value in ("90.0f", "70.0f", "50.0f"):
        require(cpp, value, "ROAD pickup SLA table")
    for value in ("case 3: return 15", "case 2: return 25", "case 1: return 40"):
        require(cpp, value, "CARGO pickup SLA table")

    require(cpp, "FMath::Clamp(CleanStreak, 0, 3)", "shared priority chain cap")
    require(cpp, "* 0.03f", "priority chain reward step")
    require(cpp, "* 10.0f", "ROAD chain grace")
    require(cpp, "* 5", "CARGO chain grace")
    require(cpp, "FMath::Min(1.28f", "ROAD emergency reward hard cap")
    require(cpp, "if (RoadUrgency <= 0) return 1.0f;", "STANDARD ROAD chain isolation")

    cargo_reward = re.search(r"GetCargoPriorityRewardMultiplier\(\) const\s*\{(?P<body>.*?)\n\}", cpp, re.S)
    assert cargo_reward and "return 1.0f;" in cargo_reward.group("body"), "CARGO must keep neutral extra priority multiplier"

    for token in (
        "Save->LogisticsCleanStreak = CleanStreak",
        "CleanStreak = FMath::Max(0, Save->LogisticsCleanStreak)",
        "CleanStreak = bClean ? FMath::Min(CleanStreak + 1, 99) : 0",
        "CleanStreak = 0",
    ):
        require(reputation, token, "persisted clean-chain source")
    require(save, "SaveVersion = 8", "save schema remains v8")
    assert "PriorityChainStreak" not in save, "0.1.11 must not add a duplicate mutable priority-chain save field"


def verify_road_emergency_contract() -> None:
    road_h = read("Source/GTT/Public/Activities/GTTRoadRunDirector.h")
    road = read("Source/GTT/Private/Activities/GTTRoadRunDirector.cpp")

    for token in (
        "PickupSlaRemaining",
        "LockedDeliveryTimeScale",
        "PriorityUrgencyAtStart",
        "PriorityLabelAtStart",
    ):
        require(road_h, token, "ROAD emergency state")

    for token in (
        "PriorityUrgencyAtStart = Logistics->GetRoadPriorityUrgency()",
        "PriorityTimeScale = Logistics->GetRoadPriorityTimeScale();",
        "LockedDeliveryTimeScale = PriorityTimeScale",
        "PriorityLabelAtStart = Logistics->GetPriorityDispatchLabel()",
        "Logistics->GetRoadPriorityPickupSlaSeconds()",
        "Logistics->GetRoadCourierRewardMultiplier() * Logistics->GetRoadPriorityRewardMultiplier()",
    ):
        require(road, token, "ROAD acceptance lock")

    require(road, "PickupSlaRemaining = FMath::Max(0.0f, PickupSlaRemaining - DeltaSeconds)", "ROAD SLA countdown")
    require(road, "Priority pickup SLA expired before the parts were collected.", "ROAD SLA failure")
    require(road, "FailContract(PlayerPawn", "ROAD SLA consequence path")
    require(road, "PICKUP SLA %.0fs", "ROAD objective SLA visibility")

    begin_delivery = re.search(r"void AGTTRoadRunDirector::BeginDelivery\(.*?\)\s*\{(?P<body>.*?)\n\}", road, re.S)
    assert begin_delivery, "ROAD BeginDelivery implementation missing"
    body = begin_delivery.group("body")
    require(body, "const float PriorityTimeScale = LockedDeliveryTimeScale;", "locked ROAD delivery scale")
    require(body, "TimeRemaining = DeliveryTimeLimit * PriorityTimeScale;", "locked ROAD delivery timer")
    require(body, "*PriorityLabelAtStart", "locked ROAD priority label")
    assert "GetRoadPriorityTimeScale" not in body, "ROAD terms must not re-query live priority at pickup"

    require(road, "ActiveDeliveryTimeLimit = DeliveryTimeLimit * LockedDeliveryTimeScale", "fast-bonus denominator uses locked timer")
    require(road, "TimeRemaining / ActiveDeliveryTimeLimit", "fast-bonus ratio uses actual emergency window")

    for token in (
        "ParcelIntegrity",
        "NativeImpactCountDuringRun",
        "bPoliceIncidentDuringRun",
        "COURIER HANDOFF BLOCKED",
        "AssessJobReadiness(RoadRunJob)",
        "RecordCourierSuccess",
        "RecordCourierFailure",
    ):
        require(road, token, "ROAD gameplay regression")


def verify_cargo_sla_integration() -> None:
    rel_h = read("Source/GTT/Public/World/GTTDispatcherRelationshipSubsystem.h")
    rel = read("Source/GTT/Private/World/GTTDispatcherRelationshipSubsystem.cpp")
    dispatcher = read("Source/GTT/Private/NPC/GTTLogisticsDispatcherPawn.cpp")
    logistics = read("Source/GTT/Private/World/GTTLogisticsReputationSubsystem.cpp")

    require(rel_h, "GetEffectiveCargoReservationHoldMinutes() const", "effective CARGO hold API")
    for token in (
        "const int32 RelationshipHoldMinutes = GetCargoReservationHoldMinutes()",
        "const int32 PrioritySlaMinutes = Logistics->GetCargoPriorityPickupSlaMinutes()",
        "FMath::Min(RelationshipHoldMinutes, PrioritySlaMinutes)",
        "PRIORITY SLA %d MIN | EFFECTIVE %d MIN",
    ):
        require(rel, token, "effective CARGO emergency hold")

    for token in (
        "if (Relationship >= 70) return 110",
        "if (Relationship >= 45) return 80",
        "if (Relationship >= 25) return 50",
        "return 35",
        "PREFERRED FAVOR: DOUBLE DESK / 110 MIN HOLD",
        "TRUST FAVOR: DOUBLE DESK / 80 MIN HOLD",
    ):
        require(rel, token, "legacy relationship favor regression")

    for token in (
        "Relationships->GetCargoReservationHoldMinutes()",
        "Relationships->GetEffectiveCargoReservationHoldMinutes()",
        "ReserveNegotiatedCargoOrder(\n                EffectiveHoldMinutes",
        "GetCargoReservationCapacity()",
    ):
        require(dispatcher, token, "dispatcher applies effective SLA")

    for token in (
        "FeedDepotStock -= RequiredStock",
        "FeedDepotStock = FMath::Clamp(FeedDepotStock +",
        "CargoBacklogPressure = FMath::Clamp(CargoBacklogPressure + 1",
        "Consumed protected T%d dispatcher load",
        "without a second stock debit",
    ):
        require(logistics, token, "stock-backed CARGO queue regression")


def verify_docs_ci_and_roadmap() -> None:
    changelog = read("CHANGELOG.d/0.1.11.md")
    playtest = read("Docs/PLAYTEST_0.1.11.md")
    workflow = read(".github/workflows/project-sanity.yml")
    roadmap = read("Docs/ROADMAP.md")

    for token in ("GTT 0.1.11", "90/70/50", "40/25/15", "SaveVersion remains 8", "No demo Release"):
        require(changelog, token, "0.1.11 changelog")
    for token in ("ROAD PRIORITY pickup SLA", "CARGO PRIORITY effective hold", "Schema-v8 compatibility", "Win64/demo acceptance boundary"):
        require(playtest, token, "0.1.11 playtest")
    require(workflow, "Verify emergency dispatch windows and priority job chains", "project sanity workflow")
    require(workflow, "python Scripts/verify_emergency_dispatch_chain.py", "project sanity workflow")

    for token in (
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
        "<!-- ROADMAP-PROGRESS:START -->",
        "<!-- ROADMAP-PROGRESS:END -->",
        'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"',
        "## 📊 Overall progress",
        "| ✅ Completed | ⏳ Remaining | 📦 Total | 🎯 Progress |",
        "../assets/readme/progress-mini.svg",
    ):
        require(roadmap, token, "SWIR Roadmap SVG-only Style Lock")

    checkboxes = re.findall(r"^\s*-\s+\[(x|X| )\]\s+", roadmap, re.M)
    completed = sum(1 for value in checkboxes if value.lower() == "x")
    total = len(checkboxes)
    remaining = total - completed
    assert total > 0, "roadmap checklist not found"
    percent = round(completed * 100.0 / total, 1)

    for token in (
        f"ROADMAP-{percent:.1f}%25",
        f"DONE-{completed}%2F{total}",
        f"| **{completed}** | **{remaining}** | **{total}** | **{percent:.1f}%** |",
    ):
        require(roadmap, token, "truthful roadmap dashboard")
    assert roadmap.count("../assets/readme/progress-mini.svg") == 1, "roadmap must embed exactly one progress-mini.svg"
    assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, re.MULTILINE), "legacy text/Unicode roadmap progress meter must not return"
    assert (completed, remaining, total, percent) == (125, 5, 130, 96.2), (
        f"0.1.11 cannot close hardware/package blockers: {completed}/{total} = {percent:.1f}%"
    )


def main() -> None:
    verify_priority_chain_model()
    verify_road_emergency_contract()
    verify_cargo_sla_integration()
    verify_docs_ci_and_roadmap()
    print("GTT 0.1.11 emergency dispatch + priority-chain source contracts: OK")
    print("Boundary: no Unreal/Win64 compile, package, packaged runtime, or rendered visual claim is made by this gate.")


if __name__ == "__main__":
    main()
