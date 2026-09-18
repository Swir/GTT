#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
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


def reject(text: str, needles: list[str], label: str) -> None:
    found = [n for n in needles if n in text]
    if found:
        raise AssertionError(f"{label}: forbidden tokens present: {found}")


def roadmap_math() -> tuple[int, int, float]:
    roadmap = read("Docs/ROADMAP.md")
    checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_items = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    total = checked + open_items
    if total <= 0:
        raise AssertionError("Roadmap contains no checklist items")
    percent = round(100.0 * checked / total, 1)
    if (checked, total, percent) != (125, 130, 96.2):
        raise AssertionError(f"Roadmap drifted unexpectedly: {checked}/{total} = {percent}%")
    require(
        roadmap,
        [
            "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
            "<!-- ROADMAP-PROGRESS:START -->",
            "<!-- ROADMAP-PROGRESS:END -->",
            "../assets/readme/progress-mini.svg",
            "| **125** | **5** | **130** | **96.2%** |",
        ],
        "roadmap presentation",
    )
    return checked, total, percent


def main() -> int:
    header = read("Source/GTT/Public/Core/GTTWorkshopHoursRuntimeEvidenceSubsystem.h")
    cpp = read("Source/GTT/Private/Core/GTTWorkshopHoursRuntimeEvidenceSubsystem.cpp")
    smoke = read("Scripts/smoke_test_windows.ps1")
    evaluator = read("Scripts/evaluate_workshop_hours_runtime.ps1")
    promoter = read("Scripts/promote_demo_gate_workshop_hours.ps1")
    win64 = read(".github/workflows/win64-package-evidence.yml")
    roadmap = read("Docs/ROADMAP.md")
    readme = read("README.md")

    require(
        header,
        [
            "UGTTWorkshopHoursRuntimeEvidenceSubsystem",
            "ClosedOrdinaryService",
            "VerifyEmergencyQuote",
            "ApplyEmergencyService",
            "bClosedNoCharge",
            "bClosedNoMutation",
            "bEmergencySingleCharge",
            "bIdentityPreserved",
        ],
        "runtime evidence header",
    )
    require(
        cpp,
        [
            "StartDelaySeconds = 384.0f",
            "GlobalDeadlineSeconds = 404.0f",
            "GTTWorkshopHoursRuntimeScenario",
            "WORKSHOP_HOURS_RUNTIME_BEGIN version=1",
            "phase=BOUNDARIES",
            "phase=CLOSED_ORDINARY",
            "phase=TOW_REQUEST",
            "phase=TOW_COMPLETE",
            "phase=EMERGENCY_QUOTE",
            "phase=EMERGENCY_SERVICE",
            "WORKSHOP_HOURS_RUNTIME_COMPLETE",
            "GTTWorkshopHoursPolicy::OpeningHour",
            "GTTWorkshopHoursPolicy::ClosingHour",
            "GTTWorkshopHoursPolicy::CalculateEmergencyRecoveryTotal(BaseWorkshopQuote)",
            "Roadside->RequestRoadsideTow(Vehicle.Get())",
            "GarageFleet->IsVehicleOnWorkshopHold(VehicleId)",
            "Workshop->GetNativeRoadCheckoutQuote(Vehicle.Get())",
            "Workshop->Interact_Implementation(PlayerPawn.Get())",
            "CashBeforeEmergency - Economy->GetCash()",
            "Vehicle->GetPersistentVehicleId() == VehicleId",
        ],
        "runtime evidence implementation",
    )
    closed_pos = cpp.index("case EPhase::ClosedOrdinaryService")
    tow_pos = cpp.index("case EPhase::RequestTow")
    if closed_pos >= tow_pos:
        raise AssertionError("ordinary after-hours rejection must execute before the tow creates a hard hold")
    quote_pos = cpp.index("case EPhase::VerifyEmergencyQuote")
    service_pos = cpp.index("case EPhase::ApplyEmergencyService")
    if quote_pos >= service_pos:
        raise AssertionError("emergency checkout quote must be verified before service debit")

    require(
        smoke,
        [
            "-GTTWorkshopHoursRuntimeScenario",
            "workshop_hours_runtime_scenario = $true",
        ],
        "packaged smoke route",
    )
    require(
        evaluator,
        [
            "gtt.workshop-hours-runtime.v1",
            "WORKSHOP_HOURS_RUNTIME_BEGIN version=1",
            "WORKSHOP_HOURS_RUNTIME.json",
            "ordinary_after_hours_rejected=$true",
            "ordinary_after_hours_no_charge=$true",
            "ordinary_after_hours_no_mutation=$true",
            "emergency_surcharge_percent=$surcharge",
            "emergency_quote_exact=$true",
            "emergency_single_charge=$true",
            "native_tow_complete_markers=$nativeTow",
            "diagnostic_failure_count=$diagnosticFailures",
        ],
        "runtime evaluator",
    )
    require(
        promoter,
        [
            "schema -ne 13",
            "$gate.schema=14",
            "farm_cargo_workshop_recovery_runtime -ne 'PASS'",
            "workshop_hours_runtime='PASS'",
            "emergency_surcharge_percent -ne 35",
            "emergency_checkout_quote",
            "diagnostic_failure_count",
        ],
        "demo gate promotion",
    )
    require(
        win64,
        [
            "default: '0.1.44'",
            "-MinimumAliveSeconds 408 -LaunchTimeoutSeconds 440",
            "-MinimumRuntimeSeconds 408",
            "evaluate_workshop_hours_runtime.ps1",
            "WORKSHOP_HOURS_RUNTIME.json",
            "promote_demo_gate_workshop_hours.ps1",
            "schema -ne 14",
            "workshop_hours_runtime -ne 'PASS'",
        ],
        "Win64 candidate pipeline",
    )

    checked, total, percent = roadmap_math()
    require(readme, ["<!-- SWIR-README-STANDARD:v2 -->", "assets/readme/progress-card.svg", "## 🔎 Search Keywords"], "README standard")
    if readme.count("assets/readme/progress-card.svg") != 1:
        raise AssertionError("README must embed exactly one progress-card.svg")
    if roadmap.count("../assets/readme/progress-mini.svg") != 1:
        raise AssertionError("Roadmap must embed exactly one progress-mini.svg")
    reject(readme + "\n" + roadmap, ["████", "▓▓▓", "░░░", "[########", "[========"], "legacy progress meters")

    print(f"GTT 0.1.44 workshop-hours runtime source contract: PASS ({checked}/{total} = {percent:.1f}%)")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
