#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = ROOT / path
    if not p.is_file():
        raise AssertionError(f"missing required file: {path}")
    return p.read_text(encoding="utf-8")

def require(text: str, needles: list[str], label: str) -> None:
    missing = [n for n in needles if n not in text]
    if missing:
        raise AssertionError(f"{label}: missing contract tokens: {missing}")

def main() -> int:
    header = read("Source/GTT/Public/Core/GTTWorkshopQueueRuntimeEvidenceSubsystem.h")
    cpp = read("Source/GTT/Private/Core/GTTWorkshopQueueRuntimeEvidenceSubsystem.cpp")
    queue_cpp = read("Source/GTT/Private/World/GTTWorkshopRepairQueueSubsystem.cpp")
    smoke = read("Scripts/smoke_test_windows.ps1")
    evaluator = read("Scripts/evaluate_workshop_queue_runtime.ps1")
    promoter = read("Scripts/promote_demo_gate_workshop_queue.ps1")
    win64 = read(".github/workflows/win64-package-evidence.yml")
    roadmap = read("Docs/ROADMAP.md")
    readme = read("README.md")

    require(header, ["UGTTWorkshopQueueRuntimeEvidenceSubsystem", "VerifyCheckpointLoad", "AwaitSubstituteRejection", "AwaitExactExecution", "bCargoContinuity", "bSingleDebit"], "runtime header")
    require(cpp, [
        "StartDelaySeconds = 410.0f", "GlobalDeadlineSeconds = 434.0f", "GTTWorkshopQueueRuntimeScenario",
        "WORKSHOP_QUEUE_RUNTIME_BEGIN version=1", "phase=BOOK", "phase=CHECKPOINT_LOAD", "phase=SUBSTITUTE",
        "phase=EXACT_SERVICE", "phase=CARGO", "WORKSHOP_QUEUE_RUNTIME_COMPLETE",
        "TryQueueNearestEligibleNativeRoadVehicle", "LoadGameFromSlot(QueueSlot", "DoesSaveGameExist(QueueSlot",
        "Snapshot.PersistentVehicleId == VehicleId", "Snapshot.LockedQuote == LockedQuote",
        "CashBeforeBooking - Economy->GetCash()", "VerifyPrimaryCargoContinuity()"
    ], "runtime implementation")
    require(queue_cpp, ["WORKSHOP_QUEUE_ACCEPTED", "WORKSHOP_QUEUE_COMPLETED", "SpendCash(LockedQuote", "ApplyNativeWorkshopService()", "SaveProgress()"], "production queue")
    require(smoke, ["-GTTWorkshopQueueRuntimeScenario", "workshop_queue_runtime_scenario = $true"], "smoke route")
    require(evaluator, ["gtt.workshop-queue-runtime.v1", "WORKSHOP_QUEUE_RUNTIME.json", "checkpoint_disk_roundtrip=$true", "substitute_vehicle_rejected=$true", "single_debit=$true", "farm_cargo_authority_preserved=$true"], "runtime evaluator")
    require(promoter, ["schema -ne 14", "$gate.schema=15", "workshop_hours_runtime -ne 'PASS'", "workshop_queue_runtime='PASS'", "workshop_queue_farm_cargo_authority_preserved"], "gate promoter")
    require(win64, ["evaluate_workshop_queue_runtime.ps1", "WORKSHOP_QUEUE_RUNTIME.json", "promote_demo_gate_workshop_queue.ps1", "schema -ne 15", "workshop_queue_runtime -ne 'PASS'"], "Win64 pipeline")

    done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_ = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    if (done, open_) != (125, 5):
        raise AssertionError(f"roadmap truth changed unexpectedly: {done}/{done+open_}")
    require(roadmap, ["<!-- SWIR-ROADMAP-STANDARD:v1 -->", "../assets/readme/progress-mini.svg", "| **125** | **5** | **130** | **96.2%** |"], "roadmap presentation")
    require(readme, ["<!-- SWIR-README-STANDARD:v2 -->", "assets/readme/progress-card.svg", "## 🔎 Search Keywords"], "README presentation")
    if roadmap.count("../assets/readme/progress-mini.svg") != 1 or readme.count("assets/readme/progress-card.svg") != 1:
        raise AssertionError("progress SVG embedding is duplicated or missing")
    legacy_meter = re.compile(r"[█▓▒░]{4,}|(?:\[[#=\-]{6,}\])")
    if legacy_meter.search(roadmap) or legacy_meter.search(readme):
        raise AssertionError("legacy text/Unicode progress meter returned")
    print("GTT 0.1.46 workshop queue packaged-runtime source contract: PASS")
    print("Roadmap: 125/130 = 96.2% (unchanged; packaged proof still required)")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
