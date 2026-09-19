#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.49 timed workshop check-in/service/checkout.

This validates deterministic source and persistence invariants only. It does not claim Unreal
compilation, Win64 packaging, packaged runtime execution, visual acceptance, or demo readiness.
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
    if end is None:
        return text[start_i:]
    return text[start_i:text.index(end, start_i)]


def main() -> int:
    save_h = read("Source/GTT/Public/Save/GTTWorkshopQueueSaveGame.h")
    queue_h = read("Source/GTT/Public/World/GTTWorkshopRepairQueueSubsystem.h")
    queue_cpp = read("Source/GTT/Private/World/GTTWorkshopRepairQueueSubsystem.cpp")
    service_terminal_cpp = read("Source/GTT/Private/World/GTTServiceTerminal.cpp")
    old_capacity = read("Source/GTT/Private/Core/GTTWorkshopCapacityRuntimeEvidenceSubsystem.cpp")
    capacity_bridge_h = read("Source/GTT/Public/Core/GTTWorkshopCapacityLifecycleBridgeSubsystem.h")
    capacity_bridge_cpp = read("Source/GTT/Private/Core/GTTWorkshopCapacityLifecycleBridgeSubsystem.cpp")
    old_capacity_verify = read("Scripts/verify_workshop_multi_vehicle_capacity.py")
    runtime_verify = read("Scripts/verify_workshop_capacity_runtime.py")
    playtest = read("Docs/PLAYTEST_0.1.49.md")
    changelog = read("CHANGELOG.d/0.1.49.md")
    workflow = read(".github/workflows/gtt-0.1.49-workshop-service-lifecycle.yml")
    roadmap = read("Docs/ROADMAP.md")
    readme = read("README.md")

    require(save_h, [
        "SchemaVersion = 1", "Appointments", "bCheckedIn", "ServiceStartDay",
        "ServiceStartHour", "ServiceCompleteDay", "ServiceCompleteHour",
        "0.1.49 additive lifecycle checkpoint",
    ], "additive lifecycle persistence")
    require(queue_h, [
        "MinimumServiceDurationHours = 0.50f", "MaximumServiceDurationHours = 1.50f",
        "IsVehicleInWorkshopService", "CalculateServiceDurationHours",
        "ResolveServiceCompletion", "HoursUntilServiceComplete",
    ], "lifecycle API")
    require(queue_cpp, [
        "ConditionDeficit", "TireDeficit", "FuelDeficit", "BodyDeficit", "DetachedPenalty",
        "WORKSHOP_QUEUE_CHECKED_IN", "WORKSHOP_QUEUE_SERVICE_PAUSED",
        "timed_service=YES", "checkout only after service", "AWAITING_PAYMENT",
        "Saved.bCheckedIn = Entry.bCheckedIn", "Entry.bCheckedIn = true",
        "ServiceCompleteDay", "ServiceCompleteHour", "later due appointments can still proceed",
    ], "timed service implementation")

    execute = block(queue_cpp, "void UGTTWorkshopRepairQueueSubsystem::TryExecuteReadyReservations")
    checkin_log = execute.index("WORKSHOP_QUEUE_CHECKED_IN")
    completion_gate_token = "if (!IsAtOrAfter(Day, Hour, Entry.ServiceCompleteDay, Entry.ServiceCompleteHour))"
    completion_gate = execute.index(completion_gate_token)
    debit = execute.index("SpendCash(LockedQuote")
    mutation = execute.index("ApplyNativeWorkshopService()")
    if not (checkin_log < completion_gate < debit < mutation):
        raise AssertionError(
            "service must log successful check-in, then pass the exact completion-time gate before debit/mutation"
        )

    # Do not use a loose `Entry.ServiceCompleteDay` substring for ordering: it also occurs inside
    # `MutableEntry.ServiceCompleteDay` while the check-in timestamp is being assigned. The exact
    # completion-gate statement above makes this source-contract check resilient to that aliasing.
    checkin_block = block(
        queue_cpp,
        "if (!Entry.bCheckedIn)",
        completion_gate_token,
    )
    if "ApplyNativeWorkshopService()" in checkin_block or "SpendCash(LockedQuote" in checkin_block:
        raise AssertionError("check-in must not charge or repair")
    require(checkin_block, [
        "ResolveServiceCompletion", "WORKSHOP_QUEUE_CHECKED_IN", "WriteCheckpoint()",
        "charged=NO", "mutation=NO",
    ], "check-in persistence/no-charge contract")

    pause_block = block(
        queue_cpp,
        "if (Entry.bCheckedIn && !IsVehicleAtWorkshop(Vehicle))",
        "if (!Entry.bCheckedIn)",
    )
    require(pause_block, [
        "bCheckedIn = false", "ServiceCompleteDay = 0", "WriteCheckpoint()",
        "charged=NO", "appointment_preserved=YES",
    ], "leave-workshop pause")
    if "RemoveEntryAt" in pause_block or "SpendCash" in pause_block:
        raise AssertionError("leaving service area must preserve appointment without debit")

    load = block(
        queue_cpp,
        "void UGTTWorkshopRepairQueueSubsystem::LoadCheckpointOnce",
        "bool UGTTWorkshopRepairQueueSubsystem::WriteCheckpoint",
    )
    write = block(
        queue_cpp,
        "bool UGTTWorkshopRepairQueueSubsystem::WriteCheckpoint",
        "void UGTTWorkshopRepairQueueSubsystem::ClearCheckpoint",
    )
    require(load, ["bLifecycleValid", "Saved.bCheckedIn", "Entry.ServiceCompleteHour"], "lifecycle load")
    require(write, [
        "Saved.bCheckedIn", "Saved.ServiceStartDay", "Saved.ServiceCompleteHour", "Save->bQueued = true",
    ], "lifecycle write")

    require(service_terminal_cpp, [
        '#include "World/GTTWorkshopRepairQueueSubsystem.h"',
        "ResolveQueuedWorkshopAppointment", "HasQueuedRepairForVehicle", "GetQueueSnapshots",
        "WORKSHOP_QUEUE_TERMINAL_GUARD", "queue_authority=YES", "direct_service=BLOCKED",
        "!bWorkshopHold && ResolveQueuedWorkshopAppointment", "Queue lifecycle owns this exact vehicle",
    ], "workshop terminal queue authority")
    native_terminal = block(
        service_terminal_cpp,
        "if (AGTTRoadVehicleNativePawn* NativeRoad = FindActiveNativeRoadVehicle",
        "if (AGTTFieldmasterNativePawn* Native = FindActiveNativeFieldmaster",
    )
    guard = native_terminal.index("ResolveQueuedWorkshopAppointment")
    direct_debit = native_terminal.index("SpendCash(TotalCost")
    direct_mutation = native_terminal.index("ApplyNativeWorkshopService()")
    if not (guard < direct_debit < direct_mutation):
        raise AssertionError("queued appointment guard must run before direct workshop debit/mutation")
    guard_block = block(
        native_terminal,
        "if (!bWorkshopHold && ResolveQueuedWorkshopAppointment",
        "if (!bNeedsMechanical && !bNeedsFuel)",
    )
    if "SpendCash(" in guard_block or "ApplyNativeWorkshopService" in guard_block or "return;" not in guard_block:
        raise AssertionError("terminal queue guard must stop direct service without charge/mutation")

    require(old_capacity, [
        "GTTWorkshopCapacityRuntimeScenario", "ExpectedSpacingHours = 0.75f",
        "bUnderfundedNonBlocking", "bLaterSingleDebit", "bCargoContinuity",
    ], "0.1.48 capacity evidence retained")
    require(capacity_bridge_h, [
        "UGTTWorkshopCapacityLifecycleBridgeSubsystem", "Evidence-only time bridge",
    ], "0.1.48 lifecycle bridge header")
    require(capacity_bridge_cpp, [
        "GTTDemoSmokeScenario", "GTTWorkshopCapacityRuntimeScenario", "Entries.Num() < 2",
        "Entry.bCheckedIn", "ServiceCompleteDay", "LatestCompletionAbsoluteHours",
        "Clock->RestoreTime", "WORKSHOP_CAPACITY_LIFECYCLE_BRIDGE",
        "0.1.49_timed_service_compatibility",
    ], "0.1.48 lifecycle bridge implementation")
    require(old_capacity_verify, [
        "MaxQueuedRepairs = 4", "AppointmentSpacingHours = 0.75f", "timed lifecycle forward compatibility",
    ], "0.1.47 verifier retained")
    require(runtime_verify, [
        "WORKSHOP_CAPACITY_RUNTIME", "later timed-service lifecycle compatibility",
    ], "0.1.48 runtime evaluator retained")

    scenarios = re.findall(r"^- \[ \] \d+\.", playtest, flags=re.MULTILINE)
    if len(scenarios) != 68:
        raise AssertionError(f"expected exactly 68 playtest scenarios, found {len(scenarios)}")
    require(playtest, [
        "Direct terminal bypass protection", "direct walk-up repair/refuel remains blocked",
        "hard WORKSHOP HOLD still takes priority",
    ], "terminal authority playtest")
    require(changelog, [
        "0.1.49", "check-in", "30", "90", "locked quote", "no charge",
        "terminal", "bypass", "125 / 130 (96.2%)", "Win64",
    ], "milestone changelog")
    require(workflow, [
        "Source/GTT/Private/World/GTTServiceTerminal.cpp", "verify_workshop_service_lifecycle.py",
        "verify_workshop_multi_vehicle_capacity.py", "verify_workshop_capacity_runtime.py",
        "GTTWorkshopCapacityLifecycleBridgeSubsystem.cpp", "generate_progress_svg.py --check",
    ], "milestone workflow")

    done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_ = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    if (done, open_) != (125, 5):
        raise AssertionError(f"roadmap truth changed unexpectedly: {done}/{done + open_}")
    require(roadmap, [
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "../assets/readme/progress-mini.svg",
        "| **125** | **5** | **130** | **96.2%** |",
    ], "roadmap presentation")
    require(readme, [
        "<!-- SWIR-README-STANDARD:v2 -->", "assets/readme/progress-card.svg", "## 🔎 Search Keywords",
    ], "README presentation")
    if roadmap.count("../assets/readme/progress-mini.svg") != 1:
        raise AssertionError("roadmap must contain exactly one progress-mini SVG")
    if readme.count("assets/readme/progress-card.svg") != 1:
        raise AssertionError("README must contain exactly one progress-card SVG")
    legacy_meter = re.compile(r"[█▓▒░]{4,}|(?:\[[#=\-]{6,}\])")
    if legacy_meter.search(roadmap) or legacy_meter.search(readme):
        raise AssertionError("legacy text/Unicode progress meter returned")

    print("GTT 0.1.49 workshop service lifecycle source contract: PASS")
    print("Lifecycle: READY -> IN_SERVICE -> AWAITING_PAYMENT -> checkout")
    print("Service duration: 0.5-1.5 world hours derived from vehicle workload")
    print("Leave service area: appointment preserved, timer reset, no charge")
    print("Workshop terminal: queued exact-ID lifecycle cannot be bypassed by direct walk-up service")
    print("Hard WORKSHOP HOLD: retains higher-priority emergency/direct recovery authority")
    print("0.1.48 packaged-capacity route: preserved through evidence-only post-check-in clock bridge")
    print(f"Roadmap: {done}/{done + open_} = {done/(done+open_)*100:.1f}% (unchanged)")
    print("Unreal/Win64 packaged runtime verification: NOT CLAIMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
