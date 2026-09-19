#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.51 workshop priority, pickup and fleet return.

This validates deterministic source/persistence/gameplay invariants only. It does not claim Unreal
compilation, Win64 packaging, packaged runtime execution, rendered visual acceptance, or demo readiness.
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing required file: {rel}")
    return path.read_text(encoding="utf-8")


def require(text: str, needles: list[str], label: str) -> None:
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise AssertionError(f"{label}: missing {missing}")


def block(text: str, start: str, end: str | None = None) -> str:
    start_i = text.index(start)
    return text[start_i:] if end is None else text[start_i:text.index(end, start_i)]


def main() -> int:
    save_h = read("Source/GTT/Public/Save/GTTWorkshopQueueSaveGame.h")
    queue_h = read("Source/GTT/Public/World/GTTWorkshopRepairQueueSubsystem.h")
    queue_cpp = read("Source/GTT/Private/World/GTTWorkshopRepairQueueSubsystem.cpp")
    board_h = read("Source/GTT/Public/World/GTTWorkshopJobBoardTerminal.h")
    board_cpp = read("Source/GTT/Private/World/GTTWorkshopJobBoardTerminal.cpp")
    priority_h = read("Source/GTT/Public/World/GTTWorkshopPriorityDeskTerminal.h")
    priority_cpp = read("Source/GTT/Private/World/GTTWorkshopPriorityDeskTerminal.cpp")
    slot_cpp = read("Source/GTT/Private/World/GTTGarageSlotTerminal.cpp")
    playtest = read("Docs/PLAYTEST_0.1.51.md")
    changelog = read("CHANGELOG.d/0.1.51.md")
    roadmap = read("Docs/ROADMAP.md")

    require(save_h, [
        "SchemaVersion = 1", "bUrgent", "bReadyForPickup", "PaidAmount", "PaidDay", "PaidHour",
        "0.1.51 additive priority contract", "0.1.51 additive paid-service pickup checkpoint",
    ], "additive persistence")

    require(queue_h, [
        "UrgentQuoteSurchargePercent = 20", "UrgentServiceDurationMultiplier = 0.80f",
        "PromoteQueuedRepairToUrgent", "ReleaseCompletedRepairForPickup",
        "IsVehicleAwaitingPickup", "GetUrgentQuoteForVehicle", "READY_FOR_PICKUP",
    ], "priority/pickup API")
    require(queue_cpp, [
        "WORKSHOP_QUEUE_PRIORITY_UPGRADED", "WORKSHOP_QUEUE_READY_FOR_PICKUP",
        "WORKSHOP_QUEUE_PICKUP_RELEASED", "bReadyForPickup = true", "PaidAmount = LockedQuote",
        "CalculateUrgentQuote", "AdvanceUrgentAppointment", "SortQueueForServiceOrder",
        "UrgentServiceDurationMultiplier", "Saved.bUrgent = Entry.bUrgent",
        "Saved.bReadyForPickup = Entry.bReadyForPickup", "Entry.PaidAmount = Saved.PaidAmount",
    ], "priority/pickup implementation")

    promotion = block(
        queue_cpp,
        "bool UGTTWorkshopRepairQueueSubsystem::PromoteQueuedRepairToUrgent",
        "bool UGTTWorkshopRepairQueueSubsystem::ReleaseCompletedRepairForPickup",
    )
    require(promotion, [
        "bUrgent = true", "LockedQuote = UrgentQuote", "AdvanceUrgentAppointment(Index)",
        "WriteCheckpoint()", "no charge", "GTTWorkshopHoursPolicy::IsOpen",
    ], "urgent promotion transaction")
    if "SpendCash(" in promotion or "ApplyNativeWorkshopService(" in promotion:
        raise AssertionError("priority promotion must not charge or repair at promotion time")

    pickup = block(
        queue_cpp,
        "bool UGTTWorkshopRepairQueueSubsystem::ReleaseCompletedRepairForPickup",
        "FGTTWorkshopRepairQueueSnapshot UGTTWorkshopRepairQueueSubsystem::BuildSnapshot",
    )
    require(pickup, [
        "bReadyForPickup", "PaidAmount != Entry.LockedQuote", "FindExactQueuedVehicle",
        "IsVehicleAtWorkshop", "QueueEntries.RemoveAt(Index)", "WriteCheckpoint()",
        "fleet_return=YES", "GameMode->SaveProgress()",
    ], "pickup release transaction")
    if "SpendCash(" in pickup or "ApplyNativeWorkshopService(" in pickup or "AddCash(" in pickup:
        raise AssertionError("pickup release must not own economy or repair mutation")

    execution = block(queue_cpp, "void UGTTWorkshopRepairQueueSubsystem::TryExecuteReadyReservations")
    debit = execution.index("SpendCash(LockedQuote")
    mutation = execution.index("ApplyNativeWorkshopService()", debit)
    pickup_checkpoint = execution.index("MutableEntry.bReadyForPickup = true", mutation)
    persist = execution.index("if (!WriteCheckpoint())", pickup_checkpoint)
    if not (debit < mutation < pickup_checkpoint < persist):
        raise AssertionError("checkout must debit, repair, then persist READY_FOR_PICKUP before normal fleet release")
    if "QueueEntries.RemoveAt(Index)" not in execution[persist:]:
        raise AssertionError("persistence-failure fallback must avoid stranding a paid repaired vehicle")

    require(board_h, ["virtual void BeginPlay() override", "safe waiting-job cancellation", "explicit post-service pickup"], "job board header")
    require(board_cpp, [
        "READY_FOR_PICKUP", "PICKUP", "ReleaseCompletedRepairForPickup", "WORKSHOP_JOB_BOARD_PICKUP",
        "Snapshot.Priority", "SpawnActor<AGTTWorkshopPriorityDeskTerminal>", "HasNearbyPriorityDesk",
    ], "job board integration")
    for forbidden in ("SpendCash(", "ApplyNativeWorkshopService(", "AddCash("):
        if forbidden in board_cpp:
            raise AssertionError(f"job board must not become repair/economy authority: {forbidden}")

    require(priority_h, ["AGTTWorkshopPriorityDeskTerminal", "PendingVehicleId", "ConfirmationSeconds"], "priority desk header")
    require(priority_cpp, [
        "FindNearestUpgradeableVehicle", "GetUrgentQuoteForVehicle", "PromoteQueuedRepairToUrgent",
        "WORKSHOP_PRIORITY_DESK_ARMED", "WORKSHOP_PRIORITY_DESK_CONFIRMED", "two-step confirm",
    ], "priority desk integration")
    for forbidden in ("SpendCash(", "ApplyNativeWorkshopService(", "AddCash("):
        if forbidden in priority_cpp:
            raise AssertionError(f"priority desk must not own repair/economy mutation: {forbidden}")

    require(slot_cpp, [
        '#include "World/GTTWorkshopRepairQueueSubsystem.h"', "IsVehicleAwaitingPickup",
        "PICKUP HOLD - JOB BOARD", "collect at workshop job board", "no extra charge applies",
    ], "garage fleet-return gate")
    pickup_hold = slot_cpp.index("if (Queue->IsVehicleAwaitingPickup(VehicleId))")
    recall = slot_cpp.index("const FVector BayLocation", pickup_hold)
    if pickup_hold >= recall:
        raise AssertionError("pickup hold must block garage recall before teleport/payment")

    require(playtest, [
        "80-case matrix", "READY_FOR_PICKUP", "STANDARD", "URGENT", "Farm Cargo",
        "No packaged Win64 proof is claimed",
    ], "0.1.51 playtest")
    require(changelog, [
        "0.1.51", "URGENT", "+20%", "READY_FOR_PICKUP", "fleet", "125 / 130 (96.2%)", "Win64",
    ], "0.1.51 changelog")

    done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_ = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    if (done, open_) != (125, 5):
        raise AssertionError(f"roadmap truth changed unexpectedly: {done}/{done + open_}")
    require(roadmap, [
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "../assets/readme/progress-mini.svg",
        "| **125** | **5** | **130** | **96.2%** |",
    ], "roadmap presentation")
    legacy_meter = re.compile(r"[█▓▒░]{4,}|(?:\[[#=\-]{6,}\])")
    if legacy_meter.search(roadmap):
        raise AssertionError("legacy text/Unicode progress meter returned")

    print("GTT 0.1.51 workshop priority/pickup source contract: PASS")
    print("Priority: two-step exact-ID STANDARD -> URGENT, +20% locked checkout, x0.80 service, no pre-charge")
    print("Pickup: successful checkout persists READY_FOR_PICKUP; job board releases exact repaired vehicle")
    print("Fleet return: garage dispatch is blocked until pickup release, without a second charge")
    print(f"Roadmap: {done}/{done + open_} = {done/(done+open_)*100:.1f}% (unchanged)")
    print("Unreal/Win64 packaged runtime verification: NOT CLAIMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
