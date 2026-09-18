#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.45 workshop repair queue.

This proves repository wiring and invariants only. It does not claim an Unreal compile,
packaged Win64 execution, runtime smoke evidence, or demo readiness.
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def text(rel: str) -> str:
    p = ROOT / rel
    if not p.is_file():
        raise AssertionError(f"missing required file: {rel}")
    return p.read_text(encoding="utf-8")


def require(haystack: str, needles: list[str], label: str) -> None:
    missing = [n for n in needles if n not in haystack]
    if missing:
        raise AssertionError(f"{label}: missing {missing}")


def main() -> int:
    queue_h = text("Source/GTT/Public/World/GTTWorkshopRepairQueueSubsystem.h")
    queue_cpp = text("Source/GTT/Private/World/GTTWorkshopRepairQueueSubsystem.cpp")
    save_h = text("Source/GTT/Public/Save/GTTWorkshopQueueSaveGame.h")
    garage = text("Source/GTT/Private/World/GTTGarageTerminal.cpp")
    policy = text("Source/GTT/Public/World/GTTWorkshopHoursPolicy.h")
    playtest = text("Docs/PLAYTEST_0.1.45.md")
    changelog = text("CHANGELOG.d/0.1.45.md")
    roadmap = text("Docs/ROADMAP.md")
    readme = text("README.md")
    workflow = text(".github/workflows/gtt-0.1.45-workshop-repair-queue.yml")

    require(save_h, [
        "SchemaVersion = 1", "bQueued", "PersistentVehicleId", "LockedQuote",
        "RequestedDay", "RequestedHour", "ReadyDay", "ReadyHour",
    ], "queue savegame")

    require(queue_h, [
        "UGTTWorkshopRepairQueueSubsystem", "UTickableWorldSubsystem",
        "TryQueueNearestEligibleNativeRoadVehicle", "CancelQueuedRepair",
        "GetQueueSnapshot", "GetQueueStatusText", "HoursUntilReady",
    ], "queue public contract")

    require(policy, [
        "ResolveNextOpening", "HoursUntilNextOpening",
        "OpeningHour = 6.5f", "ClosingHour = 20.0f",
        "AfterHoursRecoverySurchargePercent = 35",
    ], "workshop schedule")

    require(queue_cpp, [
        "GTT_WorkshopQueue_01",
        "CalculateRepairEstimate(Best, DefaultWorkshopBaseCost)",
        "State.bOwnedByPlayer",
        "RequiresHardWorkshopHold",
        "IsCargoVehicleCompatible",
        "IsVehicleImpounded",
        "SaveGameToSlot",
        "LoadGameFromSlot",
        "FindExactQueuedVehicle",
        "AMBIGUOUS_PERSISTENT_ID",
        "IsVehicleAtWorkshop",
        "GetServiceType() != EGTTServiceType::Workshop",
        "SpendCash(LockedQuote",
        "ApplyNativeWorkshopService()",
        "AddCash(LockedQuote",
        "GameMode->SaveProgress()",
        "charged=NO",
        "locked_quote_match=YES",
    ], "queue implementation")

    booking_start = queue_cpp.index("bool UGTTWorkshopRepairQueueSubsystem::TryQueueNearestEligibleNativeRoadVehicle")
    cancel_start = queue_cpp.index("bool UGTTWorkshopRepairQueueSubsystem::CancelQueuedRepair")
    booking = queue_cpp[booking_start:cancel_start]
    if "SpendCash(" in booking or "ApplyNativeWorkshopService(" in booking:
        raise AssertionError("booking path must not charge or mutate the vehicle")

    execute_start = queue_cpp.index("void UGTTWorkshopRepairQueueSubsystem::TryExecuteReadyReservation")
    execute = queue_cpp[execute_start:]
    debit = execute.index("SpendCash(LockedQuote")
    mutation = execute.index("ApplyNativeWorkshopService()")
    if debit >= mutation:
        raise AssertionError("queued service must debit immediately before authoritative mutation")
    if "IsVehicleAtWorkshop(Vehicle)" not in execute:
        raise AssertionError("execution must require physical workshop presence")
    if "RequiresHardWorkshopHold(QueuedVehicleId)" not in execute:
        raise AssertionError("hard WORKSHOP HOLD must pre-empt deferred queue")

    require(garage, [
        '#include "World/GTTWorkshopRepairQueueSubsystem.h"',
        "TryQueueNearestEligibleNativeRoadVehicle",
        "HasQueuedRepair",
        "GetQueueStatusText",
        'TEXT("1 QUEUED")',
        "no pre-charge",
    ], "garage integration")

    require(queue_cpp, [
        "bFarmCargoContractActive", "FarmCargoBoundVehicleId == VehicleId",
        "ImpoundedVehicleId == VehicleId",
    ], "authority guards")
    forbidden_writes = [
        "FarmCargoStage =", "FarmCargoTimeRemaining =", "FarmCargoIntegrity =",
        "FarmCargoBoundVehicleId =", "ImpoundedVehicleId =",
    ]
    for token in forbidden_writes:
        if token in queue_cpp:
            raise AssertionError(f"queue must not own primary authority field: {token}")

    scenarios = re.findall(r"^- \[ \] \d+\.", playtest, flags=re.MULTILINE)
    if len(scenarios) < 80:
        raise AssertionError(f"expected >=80 playtest scenarios, found {len(scenarios)}")
    require(changelog, ["no pre-charge", "125 / 130 (96.2%)", "Win64"], "changelog")
    require(workflow, [
        "verify_workshop_repair_queue.py",
        "verify_workshop_hours_emergency_service.py",
        "verify_workshop_hours_runtime.py",
        "generate_progress_svg.py --check",
    ], "workflow")

    done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
    open_ = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    if (done, open_) != (125, 5):
        raise AssertionError(f"roadmap truth changed unexpectedly: {done}/{done+open_}")
    if "**125** | **5** | **130** | **96.2%**" not in roadmap:
        raise AssertionError("roadmap progress table is stale")
    if "../assets/readme/progress-mini.svg" not in roadmap:
        raise AssertionError("roadmap progress-mini SVG missing")
    if "assets/readme/progress-card.svg" not in readme:
        raise AssertionError("README progress-card SVG missing")
    if "<!-- SWIR-README-STANDARD:v2 -->" not in readme:
        raise AssertionError("README standard v2 marker missing")
    if "## 🔎 Search Keywords" not in readme:
        raise AssertionError("README Search Keywords section missing")

    legacy_meter = re.compile(r"[█▓▒░]{4,}|(?:\[[#=\-]{6,}\])")
    if legacy_meter.search(readme) or legacy_meter.search(roadmap):
        raise AssertionError("legacy text/Unicode progress meter returned")

    print("GTT 0.1.45 workshop repair queue source contract: PASS")
    print(f"Roadmap: {done}/{done+open_} = {done/(done+open_)*100:.1f}% (unchanged)")
    print(f"Playtest scenarios: {len(scenarios)}")
    print("Runtime/Win64 verification: NOT CLAIMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
