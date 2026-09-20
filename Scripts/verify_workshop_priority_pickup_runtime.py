#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.52 packaged workshop priority/pickup evidence.

This validates deterministic source/evidence wiring only. It does not claim Unreal compilation,
Win64 packaging, packaged execution, rendered visual acceptance or demo readiness.
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


def require_min_candidate_default(text: str, minimum=(0, 1, 52)) -> None:
    match = re.search(r"default:\s*['\"]([0-9]+)\.([0-9]+)\.([0-9]+)['\"]", text)
    if not match:
        raise AssertionError("Win64 evidence integration: current candidate default is missing")
    version = tuple(map(int, match.groups()))
    if version < minimum:
        raise AssertionError(f"Win64 evidence integration: candidate default regressed below {minimum}: {version}")


def main() -> int:
    runtime_h = read("Source/GTT/Public/Core/GTTWorkshopPriorityPickupRuntimeEvidenceSubsystem.h")
    runtime_cpp = read("Source/GTT/Private/Core/GTTWorkshopPriorityPickupRuntimeEvidenceSubsystem.cpp")
    bridge_h = read("Source/GTT/Public/Core/GTTWorkshopLegacyEvidencePickupBridgeSubsystem.h")
    bridge_cpp = read("Source/GTT/Private/Core/GTTWorkshopLegacyEvidencePickupBridgeSubsystem.cpp")
    queue_h = read("Source/GTT/Public/World/GTTWorkshopRepairQueueSubsystem.h")
    queue_cpp = read("Source/GTT/Private/World/GTTWorkshopRepairQueueSubsystem.cpp")
    smoke = read("Scripts/smoke_test_windows.ps1")
    evaluator = read("Scripts/evaluate_workshop_priority_pickup_runtime.ps1")
    promoter = read("Scripts/promote_demo_gate_workshop_priority_pickup.ps1")
    win64 = read(".github/workflows/win64-package-evidence.yml")
    playtest = read("Docs/PLAYTEST_0.1.52.md")
    changelog = read("CHANGELOG.d/0.1.52.md")
    roadmap = read("Docs/ROADMAP.md")

    require(runtime_h, [
        "UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem", "PreparePriority", "AwaitCheckIn",
        "AwaitCheckout", "VerifyPickup", "bWrongIdRejected", "bNoSecondCharge",
    ], "runtime header")
    require(runtime_cpp, [
        "GTTWorkshopPriorityPickupRuntimeScenario", "WORKSHOP_PRIORITY_PICKUP_RUNTIME_BEGIN",
        "WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=PRIORITY", "WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=CHECKIN",
        "WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=CHECKOUT", "WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=PICKUP",
        "WORKSHOP_PRIORITY_PICKUP_RUNTIME_COMPLETE", "TryQueueNearestEligibleNativeRoadVehicle",
        "PromoteQueuedRepairToUrgent", "ReleaseCompletedRepairForPickup", "VerifyPriorityCheckpoint(false)",
        "VerifyPriorityCheckpoint(true)", "ExpectedUrgentSurchargePercent = 20",
        "ExpectedUrgentMultiplier = 0.80f", "CashBeforePickup", "VerifyPrimaryCargoContinuity",
    ], "runtime implementation")
    require(runtime_cpp, [
        "UrgentQuote == ExpectedUrgentQuote", "bReadyForPickup", "PaidAmount == UrgentQuote",
        "Queue->IsVehicleAwaitingPickup(VehicleId)", "Economy->GetCash() == CashBeforePickup",
        "Vehicle->GetPersistentVehicleId() == VehicleId",
    ], "runtime hard gates")

    require(queue_h, [
        "UrgentQuoteSurchargePercent = 20", "UrgentServiceDurationMultiplier = 0.80f",
        "PromoteQueuedRepairToUrgent", "ReleaseCompletedRepairForPickup", "IsVehicleAwaitingPickup",
    ], "production queue API")
    require(queue_cpp, [
        "WORKSHOP_QUEUE_PRIORITY_UPGRADED", "WORKSHOP_QUEUE_CHECKED_IN",
        "WORKSHOP_QUEUE_READY_FOR_PICKUP", "WORKSHOP_QUEUE_PICKUP_RELEASED",
    ], "production queue markers")

    require(bridge_h, ["bPriorityPickupEvidence", "Elapsed"], "legacy evidence isolation header")
    require(bridge_cpp, [
        "GTTWorkshopPriorityPickupRuntimeScenario", "PriorityPickupEvidenceStartSeconds = 451.0f",
        "LegacyBridgeStopMarginSeconds = 2.0f", "priority_pickup_isolation",
    ], "legacy evidence isolation")
    cutoff = bridge_cpp.index("Elapsed >= PriorityPickupEvidenceStartSeconds - LegacyBridgeStopMarginSeconds")
    release = bridge_cpp.index("ReleaseCompletedRepairForPickup", cutoff)
    if cutoff >= release:
        raise AssertionError("legacy auto-pickup must stop before the explicit 0.1.52 pickup route")

    require(smoke, [
        "-GTTWorkshopPriorityPickupRuntimeScenario", "workshop_priority_pickup_runtime_scenario = $true",
    ], "packaged smoke route")
    require(evaluator, [
        "gtt.workshop-priority-pickup-runtime.v1", "WORKSHOP_PRIORITY_PICKUP_RUNTIME.json",
        "urgent_surcharge_percent=20", "urgent_service_multiplier=0.80",
        "exact_id_priority_promotion=$true", "single_locked_quote_debit=$true",
        "ready_for_pickup_persisted=$true", "wrong_id_pickup_rejected=$true",
        "exact_id_pickup_release=$true", "pickup_has_no_second_charge=$true",
    ], "runtime evaluator")
    require(promoter, [
        "schema -ne 16", "$gate.schema=17", "workshop_capacity_runtime",
        "workshop_priority_pickup_runtime='PASS'", "WORKSHOP_PRIORITY_PICKUP_RUNTIME.json",
    ], "schema-17 promoter")
    require_min_candidate_default(win64)
    require(win64, [
        "evaluate_workshop_priority_pickup_runtime.ps1",
        "promote_demo_gate_workshop_priority_pickup.ps1", "WORKSHOP_PRIORITY_PICKUP_RUNTIME.json",
        "schema -ne 17", "workshop_priority_pickup_runtime -ne 'PASS'",
    ], "Win64 evidence integration")

    require(playtest, [
        "72-case matrix", "STANDARD", "URGENT", "+20%", "x0.80", "READY_FOR_PICKUP",
        "wrong-ID", "single debit", "No packaged Win64 proof is claimed",
    ], "0.1.52 playtest")
    require(changelog, [
        "0.1.52", "WORKSHOP_PRIORITY_PICKUP_RUNTIME.json", "schema 17", "+20%", "x0.80",
        "READY_FOR_PICKUP", "125 / 130 (96.2%)", "Win64",
    ], "0.1.52 changelog")

    done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_ = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    if (done, open_) != (125, 5):
        raise AssertionError(f"roadmap truth changed unexpectedly: {done}/{done + open_}")
    require(roadmap, [
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "../assets/readme/progress-mini.svg",
        "| **125** | **5** | **130** | **96.2%** |",
    ], "roadmap presentation")
    if re.search(r"[█▓▒░]{4,}|(?:\[[#=\-]{6,}\])", roadmap):
        raise AssertionError("legacy text/Unicode progress meter returned")

    print("GTT 0.1.52 workshop priority/pickup packaged-evidence source contract: PASS")
    print("Runtime route: STANDARD -> URGENT (+20%, x0.80) -> one debit -> READY_FOR_PICKUP -> exact pickup")
    print("Evidence: same-SHA manifest + schema-17 promotion wired into Win64 candidate workflow")
    print(f"Roadmap: {done}/{done + open_} = {done/(done+open_)*100:.1f}% (unchanged)")
    print("Unreal/Win64 packaged runtime verification: NOT CLAIMED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
