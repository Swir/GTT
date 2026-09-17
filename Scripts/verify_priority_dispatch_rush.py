#!/usr/bin/env python3
"""Source-contract gate for GTT 0.1.10 shared priority dispatch and ROAD rush contracts.

This is deliberately a repository/source sanity check. It does not claim Unreal compilation,
packaging, packaged-EXE runtime smoke, handling quality, or rendered visual acceptance.
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


def verify_priority_model() -> None:
    header = read("Source/GTT/Public/World/GTTLogisticsReputationSubsystem.h")
    cpp = read("Source/GTT/Private/World/GTTLogisticsPriorityDispatch.cpp")

    for api in (
        "GetRoadPriorityUrgency() const",
        "GetCargoPriorityUrgency() const",
        "GetRoadPriorityRewardMultiplier() const",
        "GetRoadPriorityTimeScale() const",
        "GetPriorityDispatchLabel() const",
        "GetPriorityDispatchSummary() const",
        "GetPriorityVehicleLabel() const",
    ):
        require(header, api, "logistics priority header")
        require(cpp, f"UGTTLogisticsReputationSubsystem::{api}", "logistics priority implementation")

    # ROAD is physically meaningful only for the FARM PARTS rotation and reuses the persistent
    # Wood/backlog state rather than inventing a parallel urgency save field.
    require(cpp, "if (CargoRotationIndex != 2) return 0;", "ROAD priority rotation gate")
    require(cpp, "WoodYardDemand + CargoBacklogPressure * 2", "ROAD priority pressure")
    require(cpp, "ToUrgency(Pressure, 5, 9, 14)", "ROAD urgency thresholds")

    # CARGO urgency stays linked to buyer demand, backlog, active route weight and protected stock.
    require(cpp, "FeedDepotStock > 0 || CargoReservedUnits.Num() > 0", "CARGO deliverable-load gate")
    require(cpp, "FMath::Max(HillFarmDemand, WoodYardDemand)", "CARGO buyer pressure")
    require(cpp, "CargoBacklogPressure * 2 + (ActiveTier - 1)", "CARGO backlog/tier pressure")
    require(cpp, "ToUrgency(Pressure, 6, 11, 16)", "CARGO urgency thresholds")

    # ROAD rush is a bounded, real risk/reward contract rather than an informational badge.
    for value in ("1.06f", "1.12f", "1.18f", "0.94f", "0.88f", "0.82f"):
        require(cpp, value, "ROAD rush economics")

    # 0.1.10 intentionally does not stack a hidden CARGO multiplier on the pre-existing capped market.
    cargo_reward_match = re.search(
        r"GetCargoPriorityRewardMultiplier\(\) const\s*\{(?P<body>.*?)\n\}", cpp, re.S
    )
    assert cargo_reward_match, "missing explicit neutral CARGO reward policy"
    assert "return 1.0f;" in cargo_reward_match.group("body"), "CARGO priority must remain payout-neutral in 0.1.10"
    assert "demand premium" not in cpp, "dispatch UI must not claim an unapplied CARGO payout premium"

    # The shared recommendation must name both real fleet roles and a neutral fallback.
    for token in ("RATTLEBACK 82 / ROAD", "MULEBOX 1200 / CARGO", "ANY READY LOGISTICS VEHICLE"):
        require(cpp, token, "priority vehicle recommendation")


def verify_road_rush_integration() -> None:
    road = read("Source/GTT/Private/Activities/GTTRoadRunDirector.cpp")
    header = read("Source/GTT/Public/Activities/GTTRoadRunDirector.h")

    require(
        road,
        "Logistics->GetRoadCourierRewardMultiplier() * Logistics->GetRoadPriorityRewardMultiplier()",
        "ROAD contract reward lock",
    )
    require(road, "PriorityTimeScale = Logistics->GetRoadPriorityTimeScale();", "ROAD pickup priority scale")
    require(road, "TimeRemaining = DeliveryTimeLimit * PriorityTimeScale;", "ROAD rush delivery window")
    require(road, "GetPriorityDispatchLabel()", "ROAD priority briefing")

    # Proven 0.1.2-0.1.9 gameplay consequences must stay in the same contract loop.
    for token in (
        "PartsPickupLocation",
        "HillFarmRelayLocation",
        "DeliveryLocation",
        "ParcelIntegrity",
        "NativeImpactCountDuringRun",
        "bPoliceIncidentDuringRun",
        "AssessJobReadiness(RoadRunJob)",
        "COURIER HANDOFF BLOCKED",
        "RecordCourierSuccess",
        "RecordCourierFailure",
    ):
        require(road, token, "ROAD regression contract")

    require(header, "DeliveryTimeLimit = 185.0f", "ROAD baseline timer")
    require(header, "BaseReward = 310", "ROAD baseline reward")


def verify_dispatcher_and_queue_integration() -> None:
    dispatcher = read("Source/GTT/Private/NPC/GTTLogisticsDispatcherPawn.cpp")
    logistics = read("Source/GTT/Private/World/GTTLogisticsReputationSubsystem.cpp")
    queue_gate = read("Scripts/verify_reserved_contract_queue.py")

    # Preserve compact interaction contracts verified by 0.1.9 while adding priority context.
    for token in (
        "E NEGOTIATE | DESK T%d",
        "HILL NEED %d | E STATUS",
        "WOOD NEED %d | E STATUS",
        "GetPriorityDispatchLabel()",
        "GetPriorityDispatchSummary()",
        "GetPriorityVehicleLabel()",
        "GetCargoPriorityUrgency()",
        "GetRoadPriorityUrgency()",
    ):
        require(dispatcher, token, "dispatcher priority integration")

    # The stock-backed queue remains authoritative and the new urgency model consumes it rather
    # than replacing it. Checking both the implementation and previous gate protects this seam.
    for token in (
        "ReserveNegotiatedCargoOrder",
        "CargoReservedOrderTiers",
        "CargoReservedUnits",
        "CargoReservationExpiryHours",
        "GetReservedCargoOrderTier",
    ):
        require(logistics, token, "0.1.9 reservation regression")
        require(queue_gate, token, "0.1.9 verifier regression")


def verify_save_and_docs_boundary() -> None:
    save = read("Source/GTT/Public/Save/GTTSaveGame.h")
    changelog = read("CHANGELOG.d/0.1.10.md")
    playtest = read("Docs/PLAYTEST_0.1.10.md")

    require(save, "SaveVersion = 8", "save schema")
    assert "SaveVersion = 9" not in save, "0.1.10 must not introduce an unnecessary save migration"

    for doc_name, text in (("changelog", changelog), ("playtest", playtest)):
        require(text, "0.1.10", doc_name)
        require(text, "Win64", doc_name)
        require(text, "packaged", doc_name)
        assert "No demo Release is authorized" in text or "Do not publish a demo" in text, f"{doc_name}: demo boundary missing"


def verify_roadmap_style_lock() -> None:
    roadmap = read("Docs/ROADMAP.md")
    for token in (
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
        "CI",
        "ROADMAP",
        "DONE",
        "STATUS",
        "📊 Overall progress",
        "Completed",
        "Remaining",
        "Total",
        "Progress",
    ):
        require(roadmap, token, "SWIR Roadmap Style Lock")

    checkboxes = re.findall(r"\[(x|X| )\]", roadmap)
    completed = sum(1 for x in checkboxes if x.lower() == "x")
    total = len(checkboxes)
    remaining = total - completed
    assert total > 0, "roadmap checklist not found"
    percent = round(completed * 100.0 / total, 1)

    # Canonical dashboard values must be computed from the checklist, never guessed.
    assert re.search(rf"\|\s*Completed\s*\|\s*{completed}\s*\|", roadmap), "ROADMAP Completed dashboard is stale"
    assert re.search(rf"\|\s*Remaining\s*\|\s*{remaining}\s*\|", roadmap), "ROADMAP Remaining dashboard is stale"
    assert re.search(rf"\|\s*Total\s*\|\s*{total}\s*\|", roadmap), "ROADMAP Total dashboard is stale"
    assert re.search(rf"\|\s*Progress\s*\|\s*{re.escape(f'{percent:.1f}%')}\s*\|", roadmap), "ROADMAP Progress dashboard is stale"

    filled = round(completed * 20 / total)
    bar = "█" * filled + "░" * (20 - filled)
    require(roadmap, f"{bar} {percent:.1f}%", "20-segment roadmap bar")

    # This milestone intentionally cannot close real hardware/package acceptance blockers.
    assert (completed, remaining, total) == (125, 5, 130), (
        f"0.1.10 unexpectedly changed roadmap completion to {completed}/{total}; "
        "only real accepted blockers may alter checklist state"
    )


def main() -> None:
    verify_priority_model()
    verify_road_rush_integration()
    verify_dispatcher_and_queue_integration()
    verify_save_and_docs_boundary()
    verify_roadmap_style_lock()
    print("GTT 0.1.10 priority-dispatch source contracts: OK")
    print("Boundary: no Unreal/Win64 compile, package, packaged runtime, or rendered visual claim is made by this gate.")


if __name__ == "__main__":
    main()
