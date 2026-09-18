#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.35 Farm Cargo emergency-patch runtime evidence."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.is_file(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")


def require(text: str, token: str, where: str) -> None:
    assert token in text, f"{where}: missing {token!r}"


header = read("Source/GTT/Public/Core/GTTFarmCargoBreakdownEvidenceSubsystem.h")
cpp = read("Source/GTT/Private/Core/GTTFarmCargoBreakdownEvidenceSubsystem.cpp")
production = read("Source/GTT/Private/Activities/GTTFarmCargoBreakdownRecoverySubsystem.cpp")
roadside = read("Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp")
evaluator = read("Scripts/evaluate_farm_cargo_breakdown_runtime.ps1")
demo_gate = read("Scripts/evaluate_demo_candidate.ps1")
workflow = read(".github/workflows/win64-package-evidence.yml")
readme = read("README.md")
roadmap = read("Docs/ROADMAP.md")

for token in (
    "DamageAndRequestPatch", "AwaitPatch", "AwaitPatchCooldown", "DamageAndRequestTow", "AwaitTow",
    "bPatchBreakdownProven", "bPatchRequested", "bPatchCompleted", "bPatchIdentityPreserved",
    "bPatchBodyPreserved", "bPatchTimerContinued", "bPatchIntegrityNotImproved", "bPatchWorkshopStillRequired",
    "PatchCostDelta", "PatchCompletedAt", "PatchBodyBefore",
):
    require(header, token, "0.1.35 header")

for token in (
    "GlobalDeadlineSeconds = 272.0f",
    "PatchDispatchProofSeconds = 3.25f", "PatchCooldownProofSeconds = 12.25f",
    "version=2 route=feed-breakdown-patch-tow-hill-wood",
    "RestorePersistentMigrationSnapshot(Staged)",
    "Staged.ConditionPercent = FMath::Min(Staged.ConditionPercent, 0.19f)",
    "Staged.TireIntegrity = FMath::Min(Staged.TireIntegrity, 0.20f)",
    "Breakdown->AssessVehicle(NativeMulebox.Get())",
    "Assessment.Recommendation == EGTTBreakdownRecommendation::TowRecommended",
    "Assessment.bEmergencyPatchPossible",
    "Roadside->RequestEmergencyRoadsidePatch(NativeMulebox.Get())",
    "Roadside->IsRoadsidePatchPending(NativeMulebox.Get())",
    "phase=BREAKDOWN_PATCH_REQUEST", "phase=PATCH_COMPLETE", "phase=PATCH_COOLDOWN",
    "PatchCostDelta = CashBeforePatch - CashAfterPatch",
    "AfterPatch.ConditionPercent >= 0.299f", "AfterPatch.TireIntegrity >= 0.319f",
    "bPatchIdentityPreserved", "bPatchBodyPreserved", "bPatchTimerContinued", "bPatchIntegrityNotImproved",
    "Elapsed - PatchCompletedAt < PatchCooldownProofSeconds",
    "ApplyPoliceSpikeDamage(0.96f, 0.30f)",
    "Roadside->RequestRoadsideTow(NativeMulebox.Get())",
    "WRONG_VEHICLE_AFTER_TOW", "HILL_HANDOFF", "FINAL_HANDOFF", "PERSISTENCE",
    "patch_cost_delta=%d tow_cost_delta=%d payout_delta=%d",
):
    require(cpp, token, "0.1.35 runtime route")

for forbidden in ("AddCash(", "CompleteCargoContract(", "TryCompleteFinalStop(PlayerPawn.Get())"):
    assert forbidden not in cpp, f"evidence harness must not bypass gameplay authority: {forbidden}"

for token in (
    "cargo-roadside-patch-pre-service", "event=PATCH_CHECKPOINT", "event=POST_PATCH_VERIFY result=PASS",
    "identity_preserved=YES", "timer_paused=NO transfer_allowed=NO",
    "cargo-roadside-tow-pre-move", "event=TOW_CHECKPOINT", "event=POST_RECOVERY_VERIFY result=PASS",
):
    require(production, token, "production cargo recovery")
for token in (
    "NATIVE_ROADSIDE_PATCH_REQUESTED", "NATIVE_ROADSIDE_PATCH_COMPLETE", "workshop_repair_still_required=YES",
    "NATIVE_ROADSIDE_TOW_REQUESTED", "NATIVE_ROADSIDE_TOW_COMPLETE",
):
    require(roadside, token, "production roadside recovery")

for token in (
    "gtt.farm-cargo-breakdown-runtime.v2",
    "BREAKDOWN_PATCH_REQUEST", "PATCH_COMPLETE", "PATCH_COOLDOWN",
    "production_pre_patch_checkpoint", "production_post_patch_identity_verification",
    "native_patch_request_marker", "native_patch_complete_marker",
    "patch_exact_vehicle_identity_preserved", "patch_body_preserved", "patch_timer_continued",
    "patch_cargo_integrity_not_improved", "patch_workshop_still_required", "patch_cooldown_wait_seconds",
    "patch cost disagrees between PATCH_COMPLETE and COMPLETE",
    "wrong_vehicle_rejected_after_tow",
):
    require(evaluator, token, "0.1.35 evaluator")

for token in (
    "gtt.farm-cargo-breakdown-runtime.v2", "schema=10", "farm_cargo_emergency_patch='PASS'",
    "farm_cargo_patch_identity_preserved", "farm_cargo_patch_body_preserved",
    "farm_cargo_patch_timer_continued", "farm_cargo_patch_workshop_still_required",
):
    require(demo_gate, token, "demo technical gate")

for token in (
    "default: '0.1.35'", "MinimumAliveSeconds 280", "LaunchTimeoutSeconds 305",
    "MinimumRuntimeSeconds 280", "evaluate_farm_cargo_breakdown_runtime.ps1",
    "FARM_CARGO_BREAKDOWN_RUNTIME.json",
):
    require(workflow, token, "Win64 package workflow")

require(readme, "<!-- SWIR-README-STANDARD:v2 -->", "README")
require(readme, "## 🔎 Search Keywords", "README")
require(readme, "Release readiness: **NOT READY**", "README")
require(roadmap, "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "ROADMAP")
require(roadmap, "<!-- ROADMAP-PROGRESS:START -->", "ROADMAP")
require(roadmap, "<!-- ROADMAP-PROGRESS:END -->", "ROADMAP")
checks = re.findall(r"^- \[(x| )\] ", roadmap, flags=re.MULTILINE)
assert checks, "ROADMAP checklist missing"
done = sum(item == "x" for item in checks)
assert (done, len(checks)) == (125, 130), f"runtime-evidence work must not close hardware/runtime gates: {done}/{len(checks)}"
assert "96.2%" in roadmap and "**125** | **5** | **130** | **96.2%**" in roadmap

for path, needles in (
    ("assets/readme/progress-card.svg", ("96.2%", "125 / 130", "NOT READY")),
    ("assets/readme/progress-mini.svg", ("96.2%", "125 / 130")),
    ("assets/readme/progress-template.svg", ("N/A",)),
):
    data = read(path)
    for needle in needles:
        require(data, needle, path)
require(read("Scripts/generate_progress_svg.py"), "SWIR-PROGRESS-SVG-PRO:v1", "progress generator")

playtest = read("Docs/PLAYTEST_0.1.35.md")
scenarios = len(re.findall(r"^\d+\. ", playtest, flags=re.MULTILINE))
assert scenarios >= 60, f"PLAYTEST_0.1.35.md must contain at least 60 numbered scenarios, found {scenarios}"
changelog = read("CHANGELOG.d/0.1.35.md")
for token in ("GTT 0.1.35", "emergency patch", "FARM_CARGO_BREAKDOWN_RUNTIME.json", "schema v2", "125/130", "does not prove"):
    require(changelog, token, "0.1.35 changelog")

print("GTT 0.1.35 Farm Cargo emergency-patch + re-breakdown/tow source contract: PASS")
print(f"Roadmap remains {done}/{len(checks)} = {done / len(checks) * 100:.1f}% until real Win64/Chaos/trailer evidence exists")
