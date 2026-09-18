#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def text(path: str) -> str:
    p = ROOT / path
    if not p.is_file():
        raise AssertionError(f"missing required file: {path}")
    return p.read_text(encoding="utf-8")


def require(path: str, *needles: str) -> str:
    data = text(path)
    for needle in needles:
        if needle not in data:
            raise AssertionError(f"{path} missing contract marker: {needle}")
    return data


def main() -> None:
    header = require(
        "Source/GTT/Public/Core/GTTFarmCargoPatchEvidenceSubsystem.h",
        "UGTTFarmCargoPatchEvidenceSubsystem",
        "DamageAndRequestPatch",
        "WrongVehicleAfterPatch",
        "bBodyDamagePreserved",
        "bLimpHomeFloorsApplied",
        "PatchCostDelta",
        "FGTTRoadVehicleMigrationSnapshot BaselineMigration",
    )
    cpp = require(
        "Source/GTT/Private/Core/GTTFarmCargoPatchEvidenceSubsystem.cpp",
        'FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))',
        'FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoBreakdownScenario"))',
        'FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoPatchScenario"))',
        "StartDelaySeconds = 252.0f",
        "GlobalDeadlineSeconds = 274.0f",
        "StartTerminal->Interact_Implementation(PlayerPawn.Get())",
        "PickupTerminal->Interact_Implementation(PlayerPawn.Get())",
        "NativeMulebox->ApplyPoliceSpikeDamage(0.96f, 0.08f)",
        "Assessment.bEmergencyPatchPossible",
        "Breakdown->CanEmergencyPatch(NativeMulebox.Get())",
        "Roadside->RequestEmergencyRoadsidePatch(NativeMulebox.Get())",
        "Roadside->IsRoadsidePatchPending(NativeMulebox.Get())",
        "Authority->GetBoundCargoVehicle() == NativeMulebox.Get()",
        "TimerAfterPatch < TimerBeforePatch",
        "IntegrityAfterPatch <= IntegrityBeforePatch + KINDA_SMALL_NUMBER",
        "BodySnapshotMatches",
        "AfterPatch.ConditionPercent",
        "AfterPatch.TireIntegrity",
        "AfterPatch.FuelLiters",
        "WRONG_VEHICLE_AFTER_PATCH",
        "HillTerminal->Interact_Implementation(PlayerPawn.Get())",
        "FinalTerminal->Interact_Implementation(PlayerPawn.Get())",
        "GameMode && GameMode->SaveProgress()",
        "FARM_CARGO_PATCH_RUNTIME_COMPLETE",
        "RestoreBaselineState()",
    )
    if "AddCash(" in cpp or "CompleteCargoContract(" in cpp or "TryCompleteFinalStop(PlayerPawn.Get())" in cpp:
        raise AssertionError("patch evidence harness must not bypass terminal/economy authorities")
    if cpp.count("FARM_CARGO_PATCH_RUNTIME_BEGIN") != 1:
        raise AssertionError("patch runtime must emit exactly one begin format")
    if cpp.find("StartDelaySeconds = 252.0f") > cpp.find("GlobalDeadlineSeconds = 274.0f"):
        raise AssertionError("patch runtime timing contract is malformed")
    _ = header

    production = require(
        "Source/GTT/Private/Activities/GTTFarmCargoBreakdownRecoverySubsystem.cpp",
        "cargo-roadside-patch-pre-service",
        "event=PATCH_CHECKPOINT",
        "timer_paused=NO transfer_allowed=NO",
        "event=POST_PATCH_VERIFY result=PASS",
        "cargo-roadside-patch-post-service",
        "identity_preserved=YES",
    )
    roadside = require(
        "Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp",
        "NATIVE_ROADSIDE_PATCH_REQUESTED",
        "player_authorized=YES",
        "NATIVE_ROADSIDE_PATCH_COMPLETE",
        "body_preserved=YES",
        "workshop_repair_still_required=YES",
    )
    _ = production, roadside

    smoke = require(
        "Scripts/smoke_test_windows.ps1",
        "-GTTFarmCargoPatchScenario",
        "farm_cargo_patch_runtime_scenario = $true",
    )
    evaluator = require(
        "Scripts/evaluate_farm_cargo_patch_runtime.ps1",
        "gtt.farm-cargo-patch-runtime.v1",
        "FARM_CARGO_PATCH_RUNTIME_BEGIN",
        "FARM_CARGO_PATCH_RUNTIME_COMPLETE",
        "FARM_CARGO_PATCH_RUNTIME.json",
        "production cargo recovery did not prove the pre-patch primary-save checkpoint",
        "native roadside service did not prove completed patch with body preservation",
        "patch cost disagrees between PATCH_COMPLETE and COMPLETE",
        "cargo completion history delta was not exactly one",
        "limp_home_floors_applied",
    )
    _ = smoke, evaluator

    workflow = require(
        ".github/workflows/win64-package-evidence.yml",
        "evaluate_farm_cargo_patch_runtime.ps1",
        "FARM_CARGO_PATCH_RUNTIME.json",
        "MinimumAliveSeconds 276",
        "LaunchTimeoutSeconds 300",
    )
    demo_gate = require(
        "Scripts/evaluate_demo_candidate.ps1",
        "FARM_CARGO_PATCH_RUNTIME.json",
        "gtt.farm-cargo-patch-runtime.v1",
        "farm_cargo_patch_runtime='PASS'",
        "schema=10",
    )
    _ = workflow, demo_gate

    roadmap = text("Docs/ROADMAP.md")
    if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
        raise AssertionError("SWIR roadmap style marker missing")
    if "<!-- ROADMAP-PROGRESS:START -->" not in roadmap or "<!-- ROADMAP-PROGRESS:END -->" not in roadmap:
        raise AssertionError("protected roadmap progress block missing")
    done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_ = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    if (done, open_) != (125, 5):
        raise AssertionError(f"0.1.35 source evidence must not close runtime roadmap gates: got {done}/{done+open_}")
    if "96.2%" not in roadmap or "125" not in roadmap or "130" not in roadmap:
        raise AssertionError("roadmap dashboard no longer reflects 125/130 = 96.2%")
    if re.search(r"[█░▓▒]{4,}", roadmap):
        raise AssertionError("legacy character progress meter returned; SWIR Progress SVG PRO requires SVG-only visualization")
    if "../assets/readme/progress-mini.svg" not in roadmap:
        raise AssertionError("roadmap mini progress SVG embed missing")

    progress = require(
        "Scripts/generate_progress_svg.py",
        "SWIR-PROGRESS-SVG-PRO:v1",
        "legacy character progress meter",
        "progress-mini.svg",
    )
    require("assets/readme/progress-card.svg", "96.2%", "125 / 130", "NOT READY")
    require("assets/readme/progress-mini.svg", "96.2%", "125 / 130")
    require("assets/readme/progress-template.svg", "N/A")
    _ = progress

    readme = require(
        "README.md",
        "<!-- SWIR-README-STANDARD:v2 -->",
        "0.1.35",
        "## 🔎 Search Keywords",
        "125 / 130 tasks complete (96.2%)",
        "Release readiness: **NOT READY**",
        "FARM_CARGO_PATCH_RUNTIME.json",
    )
    if re.search(r"[█░▓▒]{4,}", readme):
        raise AssertionError("README must not reintroduce a legacy character progress meter")

    playtest = text("Docs/PLAYTEST_0.1.35.md")
    scenarios = len(re.findall(r"^\d+\. ", playtest, flags=re.MULTILINE))
    if scenarios < 70:
        raise AssertionError(f"PLAYTEST_0.1.35.md must contain at least 70 numbered scenarios, found {scenarios}")

    changelog = require(
        "CHANGELOG.d/0.1.35.md",
        "GTT 0.1.35",
        "FARM_CARGO_PATCH_RUNTIME.json",
        "125/130 (96.2%)",
        "does not prove",
    )
    _ = changelog

    print("GTT 0.1.35 Farm Cargo emergency-patch runtime evidence contract: PASS")
    print(f"Roadmap remains {done}/{done+open_} = {done/(done+open_)*100:.1f}% with SVG-only progress presentation")


if __name__ == "__main__":
    main()