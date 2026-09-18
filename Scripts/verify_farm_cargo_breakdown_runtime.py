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
        "AwaitTow",
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
        "StartTerminal->Interact_Implementation(PlayerPawn.Get())",
        "PickupTerminal->Interact_Implementation(PlayerPawn.Get())",
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

    # 0.1.35 may strengthen the original 0.1.33 route with an emergency patch before the tow.
    # Preserve all original tow/exact-vehicle guarantees while accepting only the stronger v2 route.
    if "version=2 route=feed-breakdown-patch-tow-hill-wood" in cpp:
        for token in (
            "DamageAndRequestPatch", "AwaitPatch", "AwaitPatchCooldown",
            "RequestEmergencyRoadsidePatch", "BREAKDOWN_PATCH_REQUEST", "PATCH_COMPLETE", "PATCH_COOLDOWN",
            "patch_cost_delta", "patch_identity_preserved", "patch_body_preserved", "patch_workshop_required",
        ):
            if token not in header + cpp:
                raise AssertionError(f"strengthened breakdown route missing {token!r}")
    elif "version=1 route=feed-breakdown-tow-hill-wood" not in cpp:
        raise AssertionError("breakdown runtime begin marker is neither preserved v1 nor strengthened v2")

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
        "FARM_CARGO_BREAKDOWN_RUNTIME_BEGIN",
        "FARM_CARGO_BREAKDOWN_RUNTIME_COMPLETE",
        "FARM_CARGO_BREAKDOWN_RUNTIME.json",
        "production cargo recovery did not prove the pre-tow primary-save checkpoint",
        "native roadside service did not prove completed tow with preserved damage",
        "tow cost disagrees between TOW_COMPLETE and COMPLETE",
        "cargo completion history delta was not exactly one",
    )
    if "version=2 route=feed-breakdown-patch-tow-hill-wood" in cpp:
        for token in (
            "gtt.farm-cargo-breakdown-runtime.v2",
            "production cargo recovery did not prove the pre-patch primary-save checkpoint",
            "native roadside service did not prove completed patch with body/identity preservation",
            "patch cost disagrees between PATCH_COMPLETE and COMPLETE",
        ):
            if token not in evaluator:
                raise AssertionError(f"v2 evaluator missing strengthened gate {token!r}")
    _ = smoke

    workflow = require(
        ".github/workflows/win64-package-evidence.yml",
        "evaluate_farm_cargo_breakdown_runtime.ps1",
        "FARM_CARGO_BREAKDOWN_RUNTIME.json",
    )
    m_alive = re.search(r"MinimumAliveSeconds\s+(\d+)", workflow)
    timeout = re.search(r"LaunchTimeoutSeconds\s+(\d+)", workflow)
    if not m_alive or int(m_alive.group(1)) < 250:
        raise AssertionError("Win64 candidate runtime window regressed below the original 250-second contract")
    if not timeout or int(timeout.group(1)) <= int(m_alive.group(1)):
        raise AssertionError("Win64 candidate launch timeout must exceed the alive window")

    demo_gate = require(
        "Scripts/evaluate_demo_candidate.ps1",
        "FARM_CARGO_BREAKDOWN_RUNTIME.json",
        "farm_cargo_breakdown_runtime='PASS'",
    )
    if "version=2 route=feed-breakdown-patch-tow-hill-wood" in cpp and "gtt.farm-cargo-breakdown-runtime.v2" not in demo_gate:
        raise AssertionError("demo technical gate must consume the strengthened v2 breakdown evidence")

    roadmap = text("Docs/ROADMAP.md")
    if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
        raise AssertionError("SWIR roadmap style marker missing")
    done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    open_ = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    if (done, open_) != (125, 5):
        raise AssertionError(f"source evidence must not close runtime roadmap gates: got {done}/{done+open_}")
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

    print("GTT Farm Cargo breakdown runtime evidence contract: PASS (0.1.33 guarantees preserved; later strengthening accepted)")


if __name__ == "__main__":
    main()
