#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.38 roadside dispatch + Farm Cargo runtime evidence."""
from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.is_file(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")


def require(text: str, token: str, where: str) -> None:
    assert token in text, f"{where}: missing {token!r}"


header = read("Source/GTT/Public/Core/GTTFarmCargoDispatchEvidenceSubsystem.h")
cpp = read("Source/GTT/Private/Core/GTTFarmCargoDispatchEvidenceSubsystem.cpp")
roadside_h = read("Source/GTT/Public/Vehicles/GTTRoadsideRecoverySubsystem.h")
roadside_cpp = read("Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp")
smoke = read("Scripts/smoke_test_windows.ps1")
evaluator = read("Scripts/evaluate_farm_cargo_dispatch_runtime.ps1")
workflow = read(".github/workflows/win64-package-evidence.yml")
roadmap = read("Docs/ROADMAP.md")
readme = read("README.md")

for token in (
    "UGTTFarmCargoDispatchEvidenceSubsystem", "RequestPatchContract", "ObservePatchContract", "CancelPatch",
    "RequestTowContract", "ObserveTowContract", "CancelTow", "ReRequestPatch", "AwaitPatchCompletion",
    "WrongVehicle", "HillHandoff", "FinalHandoff", "VerifyPersistence",
    "bPatchRequestLocked", "bPatchEtaAdvanced", "bPatchCancelledNoCharge",
    "bTowRequestLocked", "bTowEtaAdvanced", "bTowCancelledNoCharge",
    "bPatchChargeMatched", "LoadedVehicleId", "BaselineMigration",
):
    require(header, token, "0.1.38 header")

for token in (
    "StartDelaySeconds = 282.0f", "GlobalDeadlineSeconds = 320.0f",
    'TEXT("GTTFarmCargoDispatchScenario")',
    "FARM_CARGO_DISPATCH_RUNTIME_BEGIN version=1 route=feed-dispatch-cancel-rerequest-hill-wood",
    "Roadside->RequestEmergencyRoadsidePatch(NativeMulebox.Get())",
    "Roadside->GetPendingRecoveryQuote(NativeMulebox.Get())",
    "Roadside->GetPendingRecoverySecondsRemaining(NativeMulebox.Get())",
    "Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get())",
    "Roadside->GetPendingRecoveryMode(NativeMulebox.Get())",
    "Roadside->CancelPendingRoadsideService(NativeMulebox.Get())",
    "Roadside->RequestRoadsideTow(NativeMulebox.Get())",
    "phase=PATCH_REQUEST", "phase=PATCH_OBSERVE", "phase=PATCH_CANCEL",
    "phase=TOW_REQUEST", "phase=TOW_OBSERVE", "phase=TOW_CANCEL",
    "phase=PATCH_REREQUEST", "phase=PATCH_COMPLETE", "phase=WRONG_VEHICLE",
    "phase=HILL_HANDOFF", "phase=FINAL_HANDOFF", "phase=PERSISTENCE",
    "FARM_CARGO_DISPATCH_RUNTIME_COMPLETE",
    "CashBeforeFinalPatch - CashAfterFinalPatch == FinalPatchQuote",
    "PatchEtaObserved < PatchEtaInitial", "TowEtaObserved < TowEtaInitial",
    "Authority->GetBoundCargoVehicleId() == LoadedVehicleId",
    "TimerAfterPatch < TimerBeforeDispatch",
    "IntegrityAfterPatch <= IntegrityBeforeDispatch + KINDA_SMALL_NUMBER",
    "HillTerminal->Interact_Implementation(PlayerPawn.Get())",
    "FinalTerminal->Interact_Implementation(PlayerPawn.Get())",
    "GameMode && GameMode->SaveProgress()",
    "RestoreBaselineState()",
):
    require(cpp, token, "0.1.38 runtime route")

for forbidden in ("AddCash(", "CompleteCargoContract(", "TryCompleteFinalStop(PlayerPawn.Get())"):
    assert forbidden not in cpp, f"dispatch evidence harness bypasses gameplay authority: {forbidden}"

for token in (
    "GetPendingRecoveryMode", "GetPendingRecoveryQuote", "GetPendingRecoverySecondsRemaining", "GetPendingRecoveryVehicleId",
    "CancelPendingRoadsideService",
):
    require(roadside_h, token, "production roadside public contract")
for token in (
    "NATIVE_ROADSIDE_DISPATCH_CANCELLED", "locked_quote=%d charged=NO",
    "NATIVE_ROADSIDE_PATCH_REQUESTED", "quote_locked=YES target_pinned=YES",
    "NATIVE_ROADSIDE_TOW_REQUESTED", "NATIVE_ROADSIDE_PATCH_COMPLETE",
):
    require(roadside_cpp, token, "production roadside implementation")

for token in (
    "-GTTFarmCargoDispatchScenario", "farm_cargo_dispatch_runtime_scenario = $true",
):
    require(smoke, token, "packaged smoke")

for token in (
    "gtt.farm-cargo-dispatch-runtime.v1", "FARM_CARGO_DISPATCH_RUNTIME_BEGIN",
    "PATCH_REQUEST", "PATCH_OBSERVE", "PATCH_CANCEL", "TOW_REQUEST", "TOW_OBSERVE", "TOW_CANCEL",
    "PATCH_REREQUEST", "PATCH_COMPLETE", "WRONG_VEHICLE", "HILL_HANDOFF", "FINAL_HANDOFF", "PERSISTENCE",
    "patch_cancel_no_charge", "tow_cancel_no_charge", "patch_charge_matched", "exact_vehicle_preserved",
    "cargo_timer_continued", "cargo_integrity_not_improved", "wrong_vehicle_rejected",
    "FARM_CARGO_DISPATCH_RUNTIME.json", "diagnostic_failure_count",
):
    require(evaluator, token, "0.1.38 evaluator")

for token in (
    "default: '0.1.38'", "MinimumAliveSeconds 324", "LaunchTimeoutSeconds 350",
    "evaluate_farm_cargo_dispatch_runtime.ps1", "FARM_CARGO_DISPATCH_RUNTIME.json",
):
    require(workflow, token, "Win64 package evidence workflow")
assert workflow.count("FARM_CARGO_DISPATCH_RUNTIME.json") >= 4, "dispatch manifest must be validated and retained in candidate/diagnostics"

require(readme, "<!-- SWIR-README-STANDARD:v2 -->", "README")
require(readme, "## 🔎 Search Keywords", "README")
require(readme, "Current development milestone: **0.1.38", "README")
require(readme, "Release readiness: **NOT READY**", "README")
require(readme, "assets/readme/progress-card.svg", "README")
assert readme.count("assets/readme/progress-card.svg") == 1, "README must embed exactly one project progress card"
assert "progress-mini.svg" not in readme, "README must not duplicate card + mini for the same scope"

for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "<!-- ROADMAP-PROGRESS:START -->", "<!-- ROADMAP-PROGRESS:END -->",
    "../assets/readme/progress-mini.svg", "**125** | **5** | **130** | **96.2%**",
):
    require(roadmap, token, "ROADMAP")
assert roadmap.count("../assets/readme/progress-mini.svg") == 1, "ROADMAP must embed exactly one mini progress SVG"
assert not re.search(r"[█▓▒░]{4,}|\[[#=\-]{6,}\]", roadmap), "retired character progress meter returned"
checks = re.findall(r"^- \[(x| )\] ", roadmap, flags=re.MULTILINE)
assert checks, "ROADMAP checklist missing"
done = sum(v == "x" for v in checks)
assert (done, len(checks)) == (125, 130), f"0.1.38 source evidence must not close runtime/art gates: {done}/{len(checks)}"

for path, needles in (
    ("assets/readme/progress-card.svg", ("96.2%", "125 / 130", "NOT READY", "#02050A", "#07111C", "#0088FF", "#62E5FF")),
    ("assets/readme/progress-mini.svg", ("96.2%", "125 / 130", "#02050A", "#0088FF", "#62E5FF")),
    ("assets/readme/progress-template.svg", ("TEMPLATE", "N/A")),
):
    data = read(path)
    ET.fromstring(data)
    for needle in needles:
        require(data, needle, path)
require(read("Scripts/generate_progress_svg.py"), "SWIR-PROGRESS-SVG-PRO:v1", "progress generator")

playtest = read("Docs/PLAYTEST_0.1.38.md")
scenarios = len(re.findall(r"^\d+\. ", playtest, flags=re.MULTILINE))
assert scenarios >= 72, f"PLAYTEST_0.1.38.md must contain at least 72 numbered scenarios, found {scenarios}"
changelog = read("CHANGELOG.d/0.1.38.md")
for token in (
    "GTT 0.1.38", "locked quote", "live ETA", "cancellation", "FARM_CARGO_DISPATCH_RUNTIME.json",
    "125/130", "96.2%", "does not prove",
):
    assert token.lower() in changelog.lower(), f"0.1.38 changelog missing {token!r}"

print("GTT 0.1.38 roadside dispatch + Farm Cargo packaged evidence source contract: PASS")
print(f"Roadmap remains {done}/{len(checks)} = {done / len(checks) * 100:.1f}% until real Win64/Chaos/trailer/visual evidence exists")
