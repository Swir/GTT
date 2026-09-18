#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.47 multi-vehicle workshop appointments.

This checks deterministic source/persistence/gameplay invariants only. It deliberately does
not claim Unreal compilation, packaged Win64 execution, visual acceptance, or demo readiness.
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
        "++Index; // Capacity rule: an underfunded vehicle never blocks later due appointments.",
        "RemoveEntryAt(Index, TEXT(\"SERVICE_COMPLETED\"))",
        "GameMode->SaveProgress()",
        "WORKSHOP_QUEUE_COMPLETED",
    ], "multi-vehicle execution")

    booking_start = queue_cpp.index("bool UGTTWorkshopRepairQueueSubsystem::TryQueueNearestEligibleNativeRoadVehicle")
    cancel_start = queue_cpp.index("bool UGTTWorkshopRepairQueueSubsystem::CancelQueuedRepair")
    booking = queue_cpp[booking_start:cancel_start]
    if "SpendCash(" in booking or "ApplyNativeWorkshopService(" in booking:
        raise AssertionError("booking must stay no-precharge/no-mutation")

    # Exact-ID cancellation must remove only one appointment, not wipe the book.
    cancel_end = queue_cpp.index("FGTTWorkshopRepairQueueSnapshot UGTTWorkshopRepairQueueSubsystem::BuildSnapshot")
    cancel = queue_cpp[cancel_start:cancel_end]
    require(cancel, ["IndexOfByPredicate", "RemoveEntryAt(Index", "No workshop appointment belongs"], "exact cancellation")
    if "ClearCheckpoint(" in cancel:
        raise AssertionError("single appointment cancellation must not clear the whole appointment book")

    execution = queue_cpp[queue_cpp.index("void UGTTWorkshopRepairQueueSubsystem::TryExecuteReadyReservations"):]
    insufficient = execution.index("if (!Economy->SpendCash(LockedQuote")
    continue_after = execution.index("++Index; // Capacity rule", insufficient)
    service_after = execution.index("ApplyNativeWorkshopService()", continue_after)
    if not (insufficient < continue_after < service_after):
        raise AssertionError("underfunded appointment must continue to later due entries")

    require(queue_cpp, [
        "bFarmCargoContractActive", "FarmCargoBoundVehicleId == VehicleId",
        "ImpoundedVehicleId == VehicleId", "RequiresHardWorkshopHold(Entry.PersistentVehicleId)",
    ], "authority isolation")

    require(garage, [
        "GetQueuedRepairCount() < RepairQueue->GetQueueCapacity()",
        "GetQueueStatusText", "appointments %d/%d", "Interact again with another eligible damaged vehicle",
        "hard holds never enter the deferred appointment book",
    ], "garage capacity presentation")

    # 0.1.46 packaged evidence still consumes the schema-v1 first-entry mirror, so additive
    # capacity cannot silently invalidate the already-established exact-single-vehicle gate.
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
    print("Underfunded due appointment: non-blocking by source contract")
    print(f"Roadmap: {done}/{done + open_} = {done/(done+open_)*100:.1f}% (unchanged)")
    print("Runtime/Win64 verification: NOT CLAIMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
