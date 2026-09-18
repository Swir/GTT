#!/usr/bin/env python3
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing required file: {rel}")
    return path.read_text(encoding="utf-8")


def require(rel: str, *tokens: str) -> str:
    data = read(rel)
    for token in tokens:
        if token not in data:
            raise AssertionError(f"{rel} missing token: {token}")
    return data


def main() -> None:
    base = require(
        "Source/GTT/Public/Vehicles/GTTVehicleBase.h",
        "AssignPersistentVehicleIdForInstance",
        "NewPersistentVehicleId.IsNone() || bOwnedByPlayer || bOccupied",
        "PersistentVehicleId = NewPersistentVehicleId",
    )
    identity_h = require(
        "Source/GTT/Public/Vehicles/GTTVehicleIdentitySubsystem.h",
        "UGTTVehicleIdentitySubsystem",
        "UTickableWorldSubsystem",
        "RefreshFleetIdentity",
        "CollisionRepairCount",
    )
    identity_cpp = require(
        "Source/GTT/Private/Vehicles/GTTVehicleIdentitySubsystem.cpp",
        "VEHICLE_IDENTITY event=OWNED_COLLISION result=UNRESOLVED",
        "persisted_id_is_immutable",
        "BuildUniqueInstanceId",
        "ObservedVehicles",
        "AssignPersistentVehicleIdForInstance",
        "VEHICLE_IDENTITY event=ASSIGN result=PASS",
    )
    if identity_cpp.index("Vehicle->IsOwnedByPlayer()") > identity_cpp.index("AssignPersistentVehicleIdForInstance"):
        raise AssertionError("fleet identity must reserve owned IDs before renaming new duplicate instances")
    _ = base, identity_h

    scenario_h = require(
        "Source/GTT/Public/Core/GTTFarmCargoRecoveryEvidenceSubsystem.h",
        "UGTTFarmCargoRecoveryEvidenceSubsystem",
        "SaveLoaded",
        "ReloadLoaded",
        "WrongVehicleAfterReload",
        "SaveRelay",
        "ReloadRelay",
        "CompletionReload",
    )
    scenario = require(
        "Source/GTT/Private/Core/GTTFarmCargoRecoveryEvidenceSubsystem.cpp",
        'FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRecoveryScenario"))',
        "StartDelaySeconds = 200.0f",
        "GlobalDeadlineSeconds = 224.0f",
        "GTTFarmCargoRecoveryPrimary_0_1_31",
        "GTTFarmCargoRecoveryDecoy_0_1_31",
        "AssignPersistentVehicleIdForInstance",
        "VehicleIdentity->RefreshFleetIdentity()",
        "GameMode && GameMode->SaveProgress()",
        "GameMode && GameMode->LoadProgress()",
        "SpawnedPickupVan->Destroy()",
        "DisturbCargoRuntimeState()",
        "wrong-vehicle-after-reload-was-not-rejected",
        "FARM_CARGO_RECOVERY_RUNTIME phase=RELOAD_LOADED",
        "FARM_CARGO_RECOVERY_RUNTIME phase=RELOAD_RELAY",
        "FARM_CARGO_RECOVERY_RUNTIME phase=COMPLETION_RELOAD",
        "FARM_CARGO_RECOVERY_RUNTIME_COMPLETE result=%s route=feed-hill-wood",
    )
    if "AddCash(" in scenario or "SettleCargoContract(" in scenario:
        raise AssertionError("recovery harness must observe authoritative payout rather than minting money")
    if "TryCompleteJob(" in scenario or "TryCompleteFinalStop(" in scenario:
        raise AssertionError("recovery harness must exercise real terminal handoffs")
    for terminal_call in (
        "StartTerminal->Interact_Implementation(PlayerPawn.Get())",
        "PickupTerminal->Interact_Implementation(PlayerPawn.Get())",
        "HillTerminal->Interact_Implementation(PlayerPawn.Get())",
        "FinalTerminal->Interact_Implementation(PlayerPawn.Get())",
    ):
        if terminal_call not in scenario:
            raise AssertionError(f"recovery harness bypasses terminal path: {terminal_call}")
    _ = scenario_h

    recovery = require(
        "Scripts/evaluate_farm_cargo_recovery_runtime.ps1",
        "gtt.farm-cargo-recovery-runtime.v1",
        "FARM_CARGO_RECOVERY_RUNTIME.json",
        "-GTTFarmCargoRecoveryScenario",
        "RELOAD_LOADED",
        "WRONG_VEHICLE_AFTER_RELOAD",
        "RELOAD_RELAY",
        "COMPLETION_RELOAD",
        "Require-IntegerField",
        "stable_vehicle_id",
        "diagnostic_failure_count",
    )
    for field in (
        "same_model_vehicle_identity_unique",
        "loaded_checkpoint_saved",
        "loaded_stage_restored",
        "loaded_vehicle_id_restored",
        "recreated_actor_rebound",
        "loaded_timer_restored",
        "loaded_integrity_restored",
        "loaded_stock_stable",
        "wrong_vehicle_rejected_after_reload",
        "relay_checkpoint_saved",
        "relay_stage_restored",
        "relay_vehicle_id_restored",
        "relay_same_vehicle",
        "relay_timer_restored",
        "relay_integrity_restored",
        "relay_stock_stable",
        "completion_reload_stable",
        "authority_cleared",
        "final_save",
    ):
        if field not in recovery:
            raise AssertionError(f"recovery manifest missing evidence field: {field}")

    smoke = require(
        "Scripts/smoke_test_windows.ps1",
        "-GTTFarmCargoRecoveryScenario",
        "farm_cargo_recovery_runtime_scenario = $true",
    )
    workflow = require(
        ".github/workflows/win64-package-evidence.yml",
        "Evaluate Farm Cargo mid-route save/load recovery runtime",
        "evaluate_farm_cargo_recovery_runtime.ps1",
        "FARM_CARGO_RECOVERY_RUNTIME.json",
    )
    runtime_match = re.search(
        r"smoke_test_windows\.ps1[^\n]*-MinimumAliveSeconds\s+(\d+)[^\n]*-LaunchTimeoutSeconds\s+(\d+)",
        workflow,
    )
    gameplay_match = re.search(
        r"evaluate_packaged_gameplay_smoke\.ps1[^\n]*-MinimumRuntimeSeconds\s+(\d+)",
        workflow,
    )
    if not runtime_match or not gameplay_match:
        raise AssertionError("Win64 workflow does not expose recovery evidence timing")
    alive = int(runtime_match.group(1))
    timeout = int(runtime_match.group(2))
    gameplay = int(gameplay_match.group(1))
    if alive < 228 or timeout <= alive or timeout < 250 or gameplay < alive:
        raise AssertionError(f"recovery evidence window too short: alive={alive}, gameplay={gameplay}, timeout={timeout}")
    if workflow.index("evaluate_farm_cargo_runtime.ps1") > workflow.index("evaluate_farm_cargo_recovery_runtime.ps1"):
        raise AssertionError("base Farm Cargo evidence must be evaluated before save/load recovery evidence")
    if workflow.index("evaluate_farm_cargo_recovery_runtime.ps1") > workflow.index("evaluate_demo_candidate.ps1"):
        raise AssertionError("save/load recovery evidence must gate the demo technical candidate")
    _ = smoke

    candidate = require(
        "Scripts/evaluate_demo_candidate.ps1",
        "FARM_CARGO_RECOVERY_RUNTIME.json",
        "gtt.farm-cargo-recovery-runtime.v1",
        "farm_cargo_recovery_runtime='PASS'",
        "farm_cargo_recovery_loaded_rebind",
        "farm_cargo_recovery_relay_rebind",
        "farm_cargo_recovery_completion_reload",
    )
    schema_match = re.search(r"(?m)^\s*schema=(\d+)\s*$", candidate)
    if not schema_match or int(schema_match.group(1)) < 8:
        raise AssertionError("demo technical gate schema did not advance to the Farm Cargo recovery evidence revision")

    persistence = require(
        "Source/GTT/Private/Activities/GTTFarmJobPersistence.cpp",
        "RestoreActiveCargoFromSave",
        "AdoptRestoredCargoVehicle",
        "FARM_CARGO_RECOVERY event=RESTORE result=PASS",
    )
    authority = require(
        "Source/GTT/Private/Activities/GTTFarmCargoAuthoritySubsystem.cpp",
        "ResolveVehicleByPersistentId(BoundCargoVehicleId)",
        "FARM_CARGO_RECOVERY event=REBIND result=PASS",
        "Director->AdoptRestoredCargoVehicle(Resolved)",
    )
    _ = persistence, authority

    playtest = read("Docs/PLAYTEST_0.1.31.md")
    for token in (
        "actor recreation",
        "wrong vehicle",
        "loaded checkpoint",
        "relay checkpoint",
        "completion reload",
        "Win64",
    ):
        if token.lower() not in playtest.lower():
            raise AssertionError(f"0.1.31 playtest missing: {token}")

    changelog = read("CHANGELOG.d/0.1.31.md")
    for token in (
        "GTT 0.1.31",
        "Fleet Identity",
        "FARM_CARGO_RECOVERY_RUNTIME.json",
        "does not claim",
    ):
        if token.lower() not in changelog.lower():
            raise AssertionError(f"0.1.31 changelog missing: {token}")

    roadmap = read("Docs/ROADMAP.md")
    if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
        raise AssertionError("SWIR Roadmap v1 marker missing")
    checks = re.findall(r"^- \[([ xX])\]", roadmap, flags=re.M)
    done = sum(1 for mark in checks if mark.lower() == "x")
    total = len(checks)
    if (done, total) != (125, 130):
        raise AssertionError(f"source/runtime-evidence milestone must not alter roadmap completion: {done}/{total}")
    for token in (
        "ROADMAP-96.2%25",
        "DONE-125%2F130",
        "███████████████████░ 96.2%",
        "| **125** | **5** | **130** | **96.2%** |",
        "../assets/readme/progress-mini.svg",
    ):
        if token not in roadmap:
            raise AssertionError(f"roadmap dashboard drift: missing {token}")

    readme = require(
        "README.md",
        "<!-- SWIR-README-STANDARD:v2 -->",
        "## 🔎 Search Keywords",
        "assets/readme/progress-card.svg",
        "0.1.31",
        "FARM_CARGO_RECOVERY_RUNTIME.json",
        "No public demo release is available yet.",
    )
    if "release readiness remains not ready" not in readme.lower():
        raise AssertionError("README must separate checklist progress from release readiness")

    subprocess.run([sys.executable, str(ROOT / "Scripts/generate_progress_svg.py"), "--check"], check=True)
    print(
        f"[OK] GTT 0.1.31 Farm Cargo recovery evidence wired: same-model legacy fleet IDs, "
        f"loaded/relay save-load, actor recreation, wrong-vehicle rejection and completion reload; runtime={alive}s/{timeout}s."
    )
    print("[OK] Demo technical gate requires FARM_CARGO_RECOVERY_RUNTIME.json schema v1 and remains bound to exact Win64 build SHA.")
    print("[OK] Roadmap remains 125/130 (96.2%); source CI does not close the five runtime/hardware blockers.")


if __name__ == "__main__":
    main()
