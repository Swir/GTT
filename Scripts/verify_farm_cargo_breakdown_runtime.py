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
        "Source/GTT/Public/Core/GTTFarmCargoBreakdownEvidenceSubsystem.h",
        "UGTTFarmCargoBreakdownEvidenceSubsystem",
        "DamageAndRequestTow",
        "WrongVehicleAfterTow",
        "FGTTRoadVehicleMigrationSnapshot BaselineMigration",
        "TowCostDelta",
    )
    cpp = require(
        "Source/GTT/Private/Core/GTTFarmCargoBreakdownEvidenceSubsystem.cpp",
        'FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))',
        'FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRuntimeScenario"))',
        'FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRecoveryScenario"))',
        'FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoBreakdownScenario"))',
        "StartDelaySeconds = 228.0f",
        "GlobalDeadlineSeconds = 248.0f",
        "StartTerminal->Interact_Implementation(PlayerPawn.Get())",
        "PickupTerminal->Interact_Implementation(PlayerPawn.Get())",
        "NativeMulebox->ApplyPoliceSpikeDamage(0.96f, 0.08f)",
        "Roadside->RequestRoadsideTow(NativeMulebox.Get())",
        "Roadside->IsRoadsideTowPending(NativeMulebox.Get())",
        "Authority->GetBoundCargoVehicle() == NativeMulebox.Get()",
        "TimerAfterTow < TimerBeforeTow",
        "IntegrityAfterTow <= IntegrityBeforeTow + KINDA_SMALL_NUMBER",
        "AfterTow.TireIntegrity <= TireIntegrityAfterDamage + 0.001f",
        "WRONG_VEHICLE_AFTER_TOW",
        "HillTerminal->Interact_Implementation(PlayerPawn.Get())",
        "FinalTerminal->Interact_Implementation(PlayerPawn.Get())",
        "GameMode && GameMode->SaveProgress()",
        "FARM_CARGO_BREAKDOWN_RUNTIME_COMPLETE",
        "RestoreBaselineState()",
    )
    if "AddCash(" in cpp or "CompleteCargoContract(" in cpp or "TryCompleteFinalStop(PlayerPawn.Get())" in cpp:
        raise AssertionError("breakdown evidence harness must not bypass terminal/economy authorities")
    if cpp.count("FARM_CARGO_BREAKDOWN_RUNTIME_BEGIN") != 1:
        raise AssertionError("breakdown runtime must emit exactly one begin format")

    native = require(
        "Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h",
        "RestorePersistentMigrationSnapshot",
        "MigrationSnapshot = InSnapshot",
        "SyncLegacyMirror()",
    )
    _ = header, native

    production = require(
        "Source/GTT/Private/Activities/GTTFarmCargoBreakdownRecoverySubsystem.cpp",
        "cargo-roadside-tow-pre-move",
        "event=TOW_CHECKPOINT",
        "timer_paused=NO transfer_allowed=NO",
        "event=POST_RECOVERY_VERIFY result=PASS",
        "identity_preserved=YES",
    )
    roadside = require(
        "Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp",
        "NATIVE_ROADSIDE_TOW_REQUESTED",
        "player_authorized=YES",
        "NATIVE_ROADSIDE_TOW_COMPLETE",
        "damage_preserved=",
    )
    _ = production, roadside

    smoke = require(
        "Scripts/smoke_test_windows.ps1",
        "-GTTFarmCargoBreakdownScenario",
        "farm_cargo_breakdown_runtime_scenario = $true",
    )
    evaluator = require(
        "Scripts/evaluate_farm_cargo_breakdown_runtime.ps1",
        "gtt.farm-cargo-breakdown-runtime.v1",
        "FARM_CARGO_BREAKDOWN_RUNTIME_BEGIN",
        "FARM_CARGO_BREAKDOWN_RUNTIME_COMPLETE",
        "FARM_CARGO_BREAKDOWN_RUNTIME.json",
        "production cargo recovery did not prove the pre-tow primary-save checkpoint",
        "native roadside service did not prove completed tow with preserved damage",
        "tow cost disagrees between TOW_COMPLETE and COMPLETE",
        "cargo completion history delta was not exactly one",
    )
    _ = smoke, evaluator

    workflow = require(
        ".github/workflows/win64-package-evidence.yml",
        "evaluate_farm_cargo_breakdown_runtime.ps1",
        "FARM_CARGO_BREAKDOWN_RUNTIME.json",
        "MinimumAliveSeconds 250",
        "LaunchTimeoutSeconds 275",
    )
    demo_gate = require(
        "Scripts/evaluate_demo_candidate.ps1",
        "FARM_CARGO_BREAKDOWN_RUNTIME.json",
        "gtt.farm-cargo-breakdown-runtime.v1",
        "farm_cargo_breakdown_runtime='PASS'",
    )
    _ = workflow, demo_gate

    roadmap = text("Docs/ROADMAP.md")
    if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
        raise AssertionError("SWIR roadmap style marker missing")
    done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_ = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    if (done, open_) != (125, 5):
        raise AssertionError(f"0.1.33 source evidence must not close runtime roadmap gates: got {done}/{done+open_}")
    if "96.2%" not in roadmap or "125" not in roadmap or "130" not in roadmap:
        raise AssertionError("roadmap dashboard no longer reflects 125/130 = 96.2%")

    require("assets/readme/progress-card.svg", "96.2%", "125 / 130", "NOT READY")
    require("assets/readme/progress-mini.svg", "96.2%", "125 / 130")
    require("assets/readme/progress-template.svg", "N/A")
    require("Scripts/generate_progress_svg.py", "SWIR-PROGRESS-SVG-PRO:v1")

    playtest = text("Docs/PLAYTEST_0.1.33.md")
    scenarios = len(re.findall(r"^\d+\. ", playtest, flags=re.MULTILINE))
    if scenarios < 56:
        raise AssertionError(f"PLAYTEST_0.1.33.md must contain at least 56 numbered scenarios, found {scenarios}")

    changelog = require(
        "CHANGELOG.d/0.1.33.md",
        "GTT 0.1.33",
        "FARM_CARGO_BREAKDOWN_RUNTIME.json",
        "125/130 (96.2%)",
        "does not prove",
    )
    _ = changelog

    print("GTT 0.1.33 Farm Cargo breakdown runtime evidence contract: PASS")


if __name__ == "__main__":
    main()
