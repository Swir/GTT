#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.48 packaged multi-vehicle workshop capacity evidence.

This verifies source/evaluator/release-gate wiring. It never claims Unreal compilation or
packaged runtime execution; those require the self-hosted Windows x64 UE 5.8 evidence job.
Later workshop lifecycle milestones may add timed check-in and explicit paid pickup while
preserving all original 0.1.48 exact-ID/capacity/economy evidence guarantees.
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
    missing = [n for n in needles if n not in text]
    if missing:
        raise AssertionError(f"{label}: missing {missing}")


def main() -> int:
    header = read("Source/GTT/Public/Core/GTTWorkshopCapacityRuntimeEvidenceSubsystem.h")
    cpp = read("Source/GTT/Private/Core/GTTWorkshopCapacityRuntimeEvidenceSubsystem.cpp")
    queue_cpp = read("Source/GTT/Private/World/GTTWorkshopRepairQueueSubsystem.cpp")
    legacy_bridge_h = read("Source/GTT/Public/Core/GTTWorkshopLegacyEvidencePickupBridgeSubsystem.h")
    legacy_bridge_cpp = read("Source/GTT/Private/Core/GTTWorkshopLegacyEvidencePickupBridgeSubsystem.cpp")
    evaluator = read("Scripts/evaluate_workshop_capacity_runtime.ps1")
    promoter = read("Scripts/promote_demo_gate_workshop_capacity.ps1")
    smoke = read("Scripts/smoke_test_windows.ps1")
    win64 = read(".github/workflows/win64-package-evidence.yml")
    milestone_workflow = read(".github/workflows/gtt-0.1.48-workshop-capacity-runtime.yml")
    playtest = read("Docs/PLAYTEST_0.1.48.md")
    changelog = read("CHANGELOG.d/0.1.48.md")

    require(header, [
        "UGTTWorkshopCapacityRuntimeEvidenceSubsystem", "PrepareBookings", "VerifyDiskAndCancel",
        "VerifyRebook", "AwaitCapacityExecution", "bUnderfundedNonBlocking", "bLaterSingleDebit",
        "FirstVehicleId", "SecondVehicleId",
    ], "runtime subsystem header")
    require(cpp, [
        "GTTWorkshopCapacityRuntimeScenario", "StartDelaySeconds = 438.0f", "GlobalDeadlineSeconds = 468.0f",
        "capacity=4 spacing_hours=0.75", "StageDamage(FirstVehicle.Get(), true)",
        "StageDamage(SecondVehicle.Get(), false)", "TryQueueNearestEligibleNativeRoadVehicle",
        "GetQueueSnapshots", "CancelQueuedRepair(SecondVehicleId", "Save->Appointments.Num() == 2",
        "FirstLockedQuote > SecondLockedQuote", "ExpectedSpacingHours = 0.75f",
        "Economy->RestoreState(SecondLockedQuote", "HasQueuedRepairForVehicle(FirstVehicleId)",
        "!Queue->HasQueuedRepairForVehicle(SecondVehicleId)", "WORKSHOP_CAPACITY_RUNTIME_COMPLETE",
    ], "runtime route")
    require(queue_cpp, [
        "MaxQueuedRepairs", "AppointmentSpacingHours", "later due appointments can still proceed",
        "RequiresHardWorkshopHold(Entry.PersistentVehicleId)", "FarmCargoBoundVehicleId == VehicleId",
        "SpendCash(LockedQuote", "ApplyNativeWorkshopService()", "WORKSHOP_QUEUE_READY_FOR_PICKUP",
        "ReleaseCompletedRepairForPickup",
    ], "production capacity authority")
    if "WORKSHOP_QUEUE_CHECKED_IN" in queue_cpp:
        require(queue_cpp, [
            "AWAITING_PAYMENT", "timed_service=YES",
            "MutableEntry.bCheckedIn = true;",
            "ResolveServiceCompletion(Day, Hour, DurationHours",
        ], "later timed-service lifecycle compatibility")
        execution = queue_cpp[queue_cpp.index("void UGTTWorkshopRepairQueueSubsystem::TryExecuteReadyReservations"):]
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
        ], "timed lifecycle check-in authority")
        if "SpendCash(" in checkin or "ApplyNativeWorkshopService(" in checkin:
            raise AssertionError("timed check-in must not debit or service before completion")

    # 0.1.51 keeps paid repairs READY_FOR_PICKUP in ordinary gameplay. Historical packaged 0.1.48
    # evidence still expects the serviced later appointment to disappear before its assertions.
    # Preserve that old gate through an explicit evidence-only bridge that calls the production
    # exact-ID pickup API under the historical command-line scenarios, never through a fake source
    # completion marker or a second economy/repair path.
    require(legacy_bridge_h, [
        "Evidence-only compatibility bridge", "UGTTWorkshopLegacyEvidencePickupBridgeSubsystem",
    ], "pickup compatibility bridge header")
    require(legacy_bridge_cpp, [
        "GTTDemoSmokeScenario", "GTTWorkshopCapacityRuntimeScenario", "GTTWorkshopQueueRuntimeScenario",
        "ReleaseCompletedRepairForPickup", "WORKSHOP_LEGACY_EVIDENCE_AUTO_PICKUP",
        "charged_again=NO", "repair_mutation=NO",
    ], "pickup compatibility bridge implementation")
    for forbidden in ("SpendCash(", "ApplyNativeWorkshopService(", "AddCash("):
        if forbidden in legacy_bridge_cpp:
            raise AssertionError(f"legacy evidence bridge must not own economy/repair mutation: {forbidden}")

    require(evaluator, [
        "gtt.workshop-capacity-runtime.v1", "WORKSHOP_CAPACITY_RUNTIME.json", "appointment_spacing_minutes=45",
        "underfunded_earlier_nonblocking", "independent_exact_id_cancel", "production_complete_markers",
        "diagnostic_failure_count",
    ], "packaged evaluator")
    require(promoter, [
        "schema -ne 15", "$gate.schema=16", "workshop_queue_runtime", "workshop_capacity_runtime='PASS'",
        "underfunded_earlier_nonblocking", "farm_cargo_authority_preserved",
    ], "schema-16 technical gate")
    require(smoke, [
        "GTTWorkshopCapacityRuntimeScenario", "workshop_capacity_runtime_scenario = $true",
    ], "packaged smoke launch")
    require(win64, [
        "default: '0.1.48'", "MinimumAliveSeconds 472", "LaunchTimeoutSeconds 505",
        "evaluate_workshop_capacity_runtime.ps1", "promote_demo_gate_workshop_capacity.ps1",
        "WORKSHOP_CAPACITY_RUNTIME.json", "schema -ne 16", "workshop_capacity_runtime -ne 'PASS'",
    ], "Win64 candidate wiring")
    require(milestone_workflow, [
        "verify_workshop_capacity_runtime.py", "verify_workshop_multi_vehicle_capacity.py",
        "verify_workshop_queue_runtime.py", "verify_win64_evidence_pipeline.py",
        "generate_progress_svg.py --check",
    ], "milestone workflow")

    scenarios = re.findall(r"^- \[ \] \d+\.", playtest, flags=re.MULTILINE)
    if len(scenarios) != 72:
        raise AssertionError(f"expected exactly 72 playtest scenarios, found {len(scenarios)}")
    require(changelog, [
        "0.1.48", "WORKSHOP_CAPACITY_RUNTIME.json", "schema 16", "45-minute",
        "underfunded", "125 / 130 (96.2%)", "Win64",
    ], "milestone changelog")

    roadmap = read("Docs/ROADMAP.md")
    readme = read("README.md")
    done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_ = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    if (done, open_) != (125, 5):
        raise AssertionError(f"roadmap truth changed unexpectedly: {done}/{done+open_}")
    require(roadmap, ["<!-- SWIR-ROADMAP-STANDARD:v1 -->", "../assets/readme/progress-mini.svg"], "roadmap SVG presentation")
    require(readme, ["<!-- SWIR-README-STANDARD:v2 -->", "assets/readme/progress-card.svg", "## 🔎 Search Keywords"], "README SVG presentation")
    if roadmap.count("../assets/readme/progress-mini.svg") != 1 or readme.count("assets/readme/progress-card.svg") != 1:
        raise AssertionError("progress SVG must appear exactly once in each canonical scope")
    legacy_meter = re.compile(r"[█▓▒░]{4,}|(?:\[[#=\-]{6,}\])")
    if legacy_meter.search(roadmap) or legacy_meter.search(readme):
        raise AssertionError("legacy text/Unicode progress meter returned")

    print("GTT 0.1.48 packaged multi-vehicle workshop capacity source contract: PASS")
    print("Runtime route: two exact-ID bookings -> disk -> cancel/rebook -> underfunded non-blocking execution")
    print("0.1.51 pickup: historical packaged route auto-releases only through evidence-only production pickup bridge")
    print("Technical gate target: schema 16 (requires real same-SHA packaged PASS evidence)")
    print(f"Roadmap: {done}/{done + open_} = {done/(done+open_)*100:.1f}% (unchanged)")
    print("Runtime/Win64 verification: NOT CLAIMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
