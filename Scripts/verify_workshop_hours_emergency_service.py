#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.43 workshop hours and after-hours recovery."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def text(path: str) -> str:
    p = ROOT / path
    if not p.is_file():
        raise AssertionError(f"missing required file: {path}")
    return p.read_text(encoding="utf-8")


def require(source: str, needle: str, label: str) -> None:
    if needle not in source:
        raise AssertionError(f"missing {label}: {needle}")


def parse_number(source: str, name: str) -> float:
    match = re.search(rf"constexpr\s+(?:float|int32)\s+{re.escape(name)}\s*=\s*([0-9.]+)", source)
    if not match:
        raise AssertionError(f"could not parse {name}")
    return float(match.group(1))


def main() -> int:
    policy = text("Source/GTT/Public/World/GTTWorkshopHoursPolicy.h")
    service_h = text("Source/GTT/Public/World/GTTServiceTerminal.h")
    service_cpp = text("Source/GTT/Private/World/GTTServiceTerminal.cpp")
    garage = text("Source/GTT/Private/World/GTTGarageTerminal.cpp")
    daynight = text("Source/GTT/Public/World/GTTDayNightCycle.h")
    old_recovery = text("Scripts/verify_garage_workshop_recovery.py")
    workflow = text(".github/workflows/workshop-hours-emergency-service-sanity.yml")
    playtest = text("Docs/PLAYTEST_0.1.43.md")
    changelog = text("CHANGELOG.d/0.1.43.md")
    roadmap = text("Docs/ROADMAP.md")
    readme = text("README.md")

    # One deterministic schedule + surcharge policy is shared by workshop and garage presentation.
    opening = parse_number(policy, "OpeningHour")
    closing = parse_number(policy, "ClosingHour")
    surcharge = parse_number(policy, "AfterHoursRecoverySurchargePercent")
    if (opening, closing, surcharge) != (6.5, 20.0, 35.0):
        raise AssertionError(f"unexpected workshop policy: open={opening}, close={closing}, surcharge={surcharge}")
    require(policy, "Normalized >= OpeningHour && Normalized < ClosingHour", "half-open daytime service window")
    require(policy, "CalculateEmergencyRecoveryTotal", "emergency quote function")
    require(policy, "FMath::CeilToInt", "deterministic surcharge rounding")
    require(policy, "GetScheduleText", "shared schedule text")

    # Boundary math is intentionally explicit: 06:30 inclusive, 20:00 exclusive.
    def is_open(hour: float) -> bool:
        hour %= 24.0
        return opening <= hour < closing

    expected = {
        6.49: False,
        6.50: True,
        12.00: True,
        19.99: True,
        20.00: False,
        2.00: False,
    }
    for hour, state in expected.items():
        if is_open(hour) != state:
            raise AssertionError(f"schedule boundary regression at {hour:.2f}")
    base_quote = 101
    emergency_quote = base_quote + max(1, int((base_quote * surcharge / 100.0) + 0.999999))
    if emergency_quote != 137:
        raise AssertionError("emergency surcharge sanity math changed unexpectedly")

    # Time comes from the existing world clock; missing clock fails open rather than soft-locking stripped test maps.
    require(daynight, "float GetTimeOfDayHours() const", "authoritative world clock API")
    require(service_cpp, "FindDayNightCycle", "workshop world-clock lookup")
    require(service_cpp, "Clock->GetTimeOfDayHours()", "workshop consumes world time")
    require(service_cpp, "return !Clock || GTTWorkshopHoursPolicy::IsOpen", "missing-clock fail-open")
    require(service_h, "bool IsWorkshopOpenNow() const", "Blueprint-friendly workshop-open API")
    require(service_h, "FText GetWorkshopStatusText() const", "Blueprint-friendly workshop-status API")
    require(service_h, "GetNativeRoadCheckoutQuote", "authoritative checkout quote API")

    # Closed ordinary service fails before any charge/mutation; hard holds get one explicit emergency lane.
    require(service_cpp, "const bool bAfterHoursEmergency = bWorkshopHold && !bWorkshopOpen", "after-hours hard-hold eligibility")
    require(service_cpp, "if (!bWorkshopOpen && !bWorkshopHold)", "ordinary service closed gate")
    require(service_cpp, "CalculateEmergencyRecoveryTotal(BaseCost)", "native road emergency total")
    require(service_cpp, "after-hours recovery complete", "native road emergency UX")
    require(service_cpp, "Ordinary repair/refuel waits for opening", "closed ordinary-service UX")
    require(service_cpp, "ApplyNativeWorkshopService()", "existing authoritative repair path")
    require(service_cpp, "GameMode->SaveProgress()", "service persistence checkpoint")
    native_start = service_cpp.find("if (AGTTRoadVehicleNativePawn* NativeRoad")
    closed_gate = service_cpp.find("if (!bWorkshopOpen && !bWorkshopHold)", native_start)
    first_native_spend = service_cpp.find("Economy->SpendCash", native_start)
    if not (0 <= native_start < closed_gate < first_native_spend):
        raise AssertionError("ordinary closed-hours gate must run before native road economy debit")

    # Fieldmaster and legacy vehicles obey the same opening hours and hard-hold escape rule.
    require(service_cpp, "Fieldmaster service resumes at opening", "Fieldmaster closed-hours gate")
    require(service_cpp, "Ordinary service resumes at opening", "legacy vehicle closed-hours gate")
    if service_cpp.count("CalculateEmergencyRecoveryTotal(WorkshopServiceCost)") < 2:
        raise AssertionError("Fieldmaster and legacy hard holds both need the emergency surcharge path")

    # Garage office surfaces schedule + hold consequence so closure is discoverable before walking to the bay.
    require(garage, "GTTWorkshopHoursPolicy::IsOpen", "garage uses shared workshop policy")
    require(garage, "WORKSHOP %s | hours %s", "garage open/closed summary")
    require(garage, "Emergency recovery remains available after hours", "garage emergency recovery guidance")
    require(garage, "ordinary repair/refuel waits for opening", "garage ordinary-service guidance")
    require(garage, "workshop %s %s", "garage interaction schedule state")

    # Historical recovery contract stays active and has been hardened for later README milestones.
    require(old_recovery, "README current development milestone must not regress below 0.1.41", "historical verifier forward-compatibility")
    require(workflow, "python Scripts/verify_garage_workshop_recovery.py", "0.1.41 regression verifier")
    require(workflow, "python Scripts/verify_farm_cargo_workshop_recovery_runtime.py", "0.1.42 evidence regression verifier")
    require(workflow, "python Scripts/verify_workshop_hours_emergency_service.py", "0.1.43 verifier")
    require(workflow, "python Scripts/generate_progress_svg.py --check", "deterministic progress check")

    # Documentation must describe gameplay without inflating roadmap/runtime readiness.
    require(playtest, "06:30", "playtest opening boundary")
    require(playtest, "20:00", "playtest closing boundary")
    require(playtest, "+35%", "playtest emergency surcharge")
    require(playtest, "WORKSHOP HOLD", "playtest hard-hold path")
    require(changelog, "0.1.43", "changelog milestone")
    require(changelog, "after-hours", "changelog after-hours behavior")
    require(roadmap, "0.1.43 workshop hours", "roadmap milestone note")
    require(roadmap, "| **125** | **5** | **130** | **96.2%** |", "authoritative roadmap count")
    require(readme, "<!-- SWIR-README-STANDARD:v2 -->", "README v2 marker")
    require(readme, "assets/readme/progress-card.svg", "README progress card")
    require(readme, "Current development milestone: **0.1.43", "README milestone sync")
    require(readme, "## 🔎 Search Keywords", "README search keywords")

    # SWIR Visual Report v3 stays SVG-only; no retired character meter may return.
    progress_block = roadmap.split("<!-- ROADMAP-PROGRESS:START -->", 1)[1].split("<!-- ROADMAP-PROGRESS:END -->", 1)[0]
    legacy_patterns = [r"[█▓▒░]{4,}", r"\[[#=\-]{5,}\]"]
    if any(re.search(pattern, progress_block) for pattern in legacy_patterns):
        raise AssertionError("legacy text progress meter returned to ROADMAP-PROGRESS")
    if readme.count("assets/readme/progress-card.svg") != 1:
        raise AssertionError("README must embed exactly one authoritative progress card")
    if roadmap.count("../assets/readme/progress-mini.svg") != 1:
        raise AssertionError("Roadmap must embed exactly one authoritative progress mini")

    print("GTT 0.1.43 workshop hours and after-hours recovery: PASS")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, IndexError, ValueError) as exc:
        print(f"GTT 0.1.43 workshop hours and after-hours recovery: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
