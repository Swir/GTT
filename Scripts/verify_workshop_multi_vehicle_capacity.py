#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.47 multi-vehicle workshop appointments.

This checks deterministic source/persistence/gameplay invariants only. It deliberately does
not claim Unreal compilation, packaged Win64 execution, visual acceptance, or demo readiness.
Later milestones may insert timed service and explicit pickup between READY and final queue
removal; the original capacity, exact-ID, locked-quote and non-blocking economy guarantees
remain mandatory.
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


def main() -> int:
    save_h = read("Source/GTT/Public/Save/GTTWorkshopQueueSaveGame.h")
    queue_h = read("Source/GTT/Public/World/GTTWorkshopRepairQueueSubsystem.h")
    queue_cpp = read("Source/GTT/Private/World/GTTWorkshopRepairQueueSubsystem.cpp")
    garage = read("Source/GTT/Private/World/GTTGarageTerminal.cpp")
    old_runtime = read("Source/GTT/Private/Core/GTTWorkshopQueueRuntimeEvidenceSubsystem.cpp")
    bridge_h = read("Source/GTT/Public/Core/GTTWorkshopLegacyEvidencePickupBridgeSubsystem.h")
    bridge_cpp = read("Source/GTT/Private/Core/GTTWorkshopLegacyEvidencePickupBridgeSubsystem.cpp")
    playtest = read("Docs/PLAYTEST_0.1.47.md")
    changelog = read("CHANGELOG.d/0.1.47.md")
    workflow = read(".github/workflows/gtt-0.1.47-workshop-multi-vehicle-capacity.yml")
    roadmap = read("Docs/ROADMAP.md")
    readme = read("README.md")

    require(save_h, [
        "FGTTWorkshopQueueSaveEntry", "SchemaVersion = 1", "Appointments",
        "Legacy first-entry mirror", "PersistentVehicleId", "LockedQuote",
        "ReadyDay", "ReadyHour",
    ], "additive queue persistence")
    require(queue_h, [
        "MaxQueuedRepairs = 4", "AppointmentSpacingHours = 0.75f",
        "GetQueuedRepairCount", "GetQueueCapacity", "GetQueueSnapshots",
        "HasQueuedRepairForVehicle", "QueueEntries",
    ], "capacity API")
    require(queue_cpp, [
        "HasQueuedRepairForVehicle(CandidateId)",
        "QueueEntries.Num() >= MaxQueuedRepairs",
        "ResolveNextAppointment",
        "Existing.ReadyHour + AppointmentSpacingHours",
        "Save->Appointments",
        "bMigratedLegacy",
        "SeenIds.Contains",
        "FindExactQueuedVehicle(Entry.PersistentVehicleId",
        "const int32 LockedQuote = Entry.LockedQuote",
        "SpendCash(LockedQuote",
        "ApplyNativeWorkshopService()",
        "AddCash(LockedQuote",
        "later due appointments can still proceed",
        "WORKSHOP_QUEUE_READY_FOR_PICKUP",
        "ReleaseCompletedRepairForPickup",
        "GameMode->SaveProgress()",
    ], "multi-vehicle execution")

    booking_start = queue_cpp.index("bool UGTTWorkshopRepairQueueSubsystem::TryQueueNearestEligibleNativeRoadVehicle")
    cancel_start = queue_cpp.index("bool UGTTWorkshopRepairQueueSubsystem::CancelQueuedRepair")
    booking = queue_cpp[booking_start:cancel_start]
    if "SpendCash(" in booking or "ApplyNativeWorkshopService(" in booking:
        raise AssertionError("booking must stay no-precharge/no-mutation")

    # Exact-ID cancellation must remove only one waiting appointment, not wipe the book.
    cancel_end = queue_cpp.index("bool UGTTWorkshopRepairQueueSubsystem::PromoteQueuedRepairToUrgent")
    cancel = queue_cpp[cancel_start:cancel_end]
    require(cancel, ["IndexOfByPredicate", "RemoveEntryAt(Index", "No workshop appointment belongs"], "exact cancellation")
    if "ClearCheckpoint(" in cancel:
        raise AssertionError("single appointment cancellation must not clear the whole appointment book")

    execution = queue_cpp[queue_cpp.index("void UGTTWorkshopRepairQueueSubsystem::TryExecuteReadyReservations"):]
    insufficient = execution.index("if (!Economy->SpendCash(LockedQuote")
    continue_after = execution.index("++Index;", insufficient)
    service_after = execution.index("ApplyNativeWorkshopService()", continue_after)
    if not (insufficient < continue_after < service_after):
        raise AssertionError("underfunded appointment must continue to later due entries")

    # 0.1.49+ inserts timed check-in before debit. Verify behavior from executable source structure.
    if "bCheckedIn" in queue_h or "WORKSHOP_QUEUE_CHECKED_IN" in queue_cpp:
        require(queue_cpp, [
            "AWAITING_PAYMENT", "timed_service=YES",
            "MutableEntry.bCheckedIn = true;",
            "ResolveServiceCompletion(Day, Hour, DurationHours",
        ], "timed lifecycle forward compatibility")
        checkin_start = execution.index("if (!Entry.bCheckedIn)")
        checkout_start = execution.index(
            "if (!IsAtOrAfter(Day, Hour, Entry.ServiceCompleteDay, Entry.ServiceCompleteHour))",
            checkin_start,
        )
        checkin = execution[checkin_start:checkout_start]
        require(checkin, [
            "if (!IsVehicleAtWorkshop(Vehicle))",
            "MutableEntry.bCheckedIn = true;",
            "WriteCheckpoint()",
            "++Index;",
            "continue;",
        ], "timed lifecycle check-in")
        if "SpendCash(" in checkin or "ApplyNativeWorkshopService(" in checkin:
            raise AssertionError("timed check-in must preserve locked quote without charging or servicing before completion")

    # 0.1.51+ keeps successful paid work READY_FOR_PICKUP. That is a forward-compatible lifecycle
    # extension only if repair/payment remain authoritative and the historical packaged evidence can
    # release through the same production pickup API under explicit evidence flags.
    if "bReadyForPickup" in queue_h:
        require(queue_cpp, [
            "MutableEntry.bReadyForPickup = true", "PaidAmount = LockedQuote",
            "WORKSHOP_QUEUE_READY_FOR_PICKUP", "ReleaseCompletedRepairForPickup",
        ], "pickup forward compatibility")
        require(bridge_h, [
            "Evidence-only compatibility bridge", "UGTTWorkshopLegacyEvidencePickupBridgeSubsystem",
        ], "historical evidence bridge header")
        require(bridge_cpp, [
            "GTTDemoSmokeScenario", "GTTWorkshopQueueRuntimeScenario", "GTTWorkshopCapacityRuntimeScenario",
            "ReleaseCompletedRepairForPickup", "WORKSHOP_LEGACY_EVIDENCE_AUTO_PICKUP",
            "charged_again=NO", "repair_mutation=NO",
        ], "historical evidence bridge implementation")

    require(queue_cpp, [
        "bFarmCargoContractActive", "FarmCargoBoundVehicleId == VehicleId",
        "ImpoundedVehicleId == VehicleId", "RequiresHardWorkshopHold(Entry.PersistentVehicleId)",
    ], "authority isolation")

    require(garage, [
        "GetQueuedRepairCount() < RepairQueue->GetQueueCapacity()",
        "GetQueueStatusText", "appointments %d/%d", "Interact again with another eligible damaged vehicle",
        "hard holds never enter the deferred appointment book",
    ], "garage capacity presentation")

    # Historical 0.1.46 packaged evidence still consumes the schema-v1 first-entry mirror and still
    # expects the queue to be empty after successful service. The evidence-only pickup bridge above
    # satisfies that expectation through the production release API rather than bypassing service.
    require(old_runtime, [
        "Save->SchemaVersion == 1", "Save->bQueued", "Save->PersistentVehicleId == VehicleId",
        "Save->LockedQuote == LockedQuote", "!Queue->HasQueuedRepair()",
    ], "0.1.46 compatibility")

    scenarios = re.findall(r"^- \[ \] \d+\.", playtest, flags=re.MULTILINE)
    if len(scenarios) != 72:
        raise AssertionError(f"expected exactly 72 playtest scenarios, found {len(scenarios)}")
    require(changelog, [
        "0.1.47", "four", "45-minute", "no pre-charge", "underfunded",
        "125 / 130 (96.2%)", "Win64",
    ], "milestone changelog")
    require(workflow, [
        "verify_workshop_multi_vehicle_capacity.py",
        "verify_workshop_repair_queue.py",
        "verify_workshop_queue_runtime.py",
        "generate_progress_svg.py --check",
    ], "milestone workflow")

    done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_ = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    if (done, open_) != (125, 5):
        raise AssertionError(f"roadmap truth changed unexpectedly: {done}/{done+open_}")
    require(roadmap, [
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "../assets/readme/progress-mini.svg",
        "| **125** | **5** | **130** | **96.2%** |",
    ], "roadmap presentation")
    require(readme, [
        "<!-- SWIR-README-STANDARD:v2 -->", "assets/readme/progress-card.svg",
        "## 🔎 Search Keywords",
    ], "README presentation")
    if roadmap.count("../assets/readme/progress-mini.svg") != 1:
        raise AssertionError("roadmap must contain exactly one progress-mini SVG")
    if readme.count("assets/readme/progress-card.svg") != 1:
        raise AssertionError("README must contain exactly one progress-card SVG")
    legacy_meter = re.compile(r"[█▓▒░]{4,}|(?:\[[#=\-]{6,}\])")
    if legacy_meter.search(roadmap) or legacy_meter.search(readme):
        raise AssertionError("legacy text/Unicode progress meter returned")

    print("GTT 0.1.47 multi-vehicle workshop capacity source contract: PASS")
    print("Capacity: 4 exact-ID appointments; spacing: 45 minutes; booking/cancel: no pre-charge")
    print("Underfunded due checkout: non-blocking by source contract")
    print("Later pickup lifecycle: compatible through exact-ID production release + evidence-only bridge")
    print(f"Roadmap: {done}/{done + open_} = {done/(done+open_)*100:.1f}% (unchanged)")
    print("Runtime/Win64 verification: NOT CLAIMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
