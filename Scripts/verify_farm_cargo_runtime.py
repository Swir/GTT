#!/usr/bin/env python3
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def text(path: str) -> str:
    p = ROOT / path
    if not p.exists():
        raise AssertionError(f"missing required file: {path}")
    return p.read_text(encoding="utf-8")


def require(path: str, *tokens: str) -> str:
    data = text(path)
    for token in tokens:
        if token not in data:
            raise AssertionError(f"{path} missing token: {token}")
    return data


def main() -> None:
    header = require(
        "Source/GTT/Public/Core/GTTFarmCargoEvidenceScenarioSubsystem.h",
        "UGTTFarmCargoEvidenceScenarioSubsystem",
        "UTickableWorldSubsystem",
        "WrongVehicleProbe",
        "VerifyPersistence",
        "BaselineLogisticsSave",
    )
    cpp = require(
        "Source/GTT/Private/Core/GTTFarmCargoEvidenceScenarioSubsystem.cpp",
        'FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))',
        'FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRuntimeScenario"))',
        "StartDelaySeconds = 180.0f",
        "GlobalDeadlineSeconds = 196.0f",
        "Logistics->CaptureToSave(BaselineLogisticsSave)",
        "Logistics->RecordCargoSuccess(0, 1.0f, true, false, true)",
        "StartTerminal->Interact_Implementation(PlayerPawn.Get())",
        "PickupTerminal->Interact_Implementation(PlayerPawn.Get())",
        "Authority->GetBoundCargoVehicle()",
        "wrong-vehicle-handoff-was-not-rejected",
        "HillTerminal->Interact_Implementation(PlayerPawn.Get())",
        "FinalTerminal->Interact_Implementation(PlayerPawn.Get())",
        "PayoutDelta = Economy->GetCash() - EvidenceCashBefore",
        "CargoRunsDelta = Logistics->GetCargoCompletedRuns() - EvidenceCargoRunsBefore",
        "ReputationDelta = Logistics->GetReputation() - EvidenceReputationBefore",
        "GameMode->SaveProgress()",
        "Logistics->RestoreFromSave(BaselineLogisticsSave)",
        "FARM_CARGO_RUNTIME_COMPLETE result=%s route=feed-hill-wood",
    )
    if "AddCash(" in cpp or "SettleCargoContract(" in cpp:
        raise AssertionError("runtime harness must observe, not replace, FarmJobDirector payout/market authority")
    if "TryCompleteJob(" in cpp or "TryCompleteFinalStop(" in cpp:
        raise AssertionError("runtime harness must go through real FarmJobTerminal interactions")

    authority = require(
        "Source/GTT/Private/Activities/GTTFarmCargoAuthoritySubsystem.cpp",
        "FARM_CARGO_AUTHORITY event=BIND result=PASS",
        "FARM_CARGO_AUTHORITY event=HANDOFF_CHECK result=PASS",
        "BoundCargoVehicleId",
    )
    terminal = require(
        "Source/GTT/Private/Activities/GTTFarmJobTerminal.cpp",
        "ValidateBoundCargoHandoff",
        "Authority->BindLoadedVehicle",
        "Director->TryCompleteJob(Pawn)",
        "Director->TryCompleteFinalStop(Pawn)",
    )
    director = require(
        "Source/GTT/Private/Activities/GTTFarmJobDirector.cpp",
        "Economy->AddCash(TotalReward",
        "Logistics->RecordCargoSuccess",
        "GameMode->SaveProgress()",
    )
    _ = header, authority, terminal, director

    evaluator = require(
        "Scripts/evaluate_farm_cargo_runtime.ps1",
        "gtt.farm-cargo-runtime.v1",
        "-GTTFarmCargoRuntimeScenario",
        "FARM_CARGO_RUNTIME_BEGIN",
        "wrong-vehicle handoff rejection was not proven",
        "FARM_CARGO_RUNTIME_COMPLETE",
        "FARM_CARGO_RUNTIME.json",
        "Require-IntegerField",
        "missing required integer field",
        "completion marker gate",
        "evidence disagrees between FINAL_HANDOFF and COMPLETE",
    )
    for required_field in (
        "active_order_tier",
        "same_vehicle",
        "payout_delta",
        "cargo_runs_delta",
        "reputation_delta",
        "authority_cleared",
        "accepted",
        "pickup",
        "wrong_vehicle_rejected",
        "hill",
        "final",
        "save",
    ):
        if evaluator.count(f"'{required_field}'") < 1:
            raise AssertionError(f"Farm Cargo evaluator does not explicitly require {required_field}")

    smoke = require(
        "Scripts/smoke_test_windows.ps1",
        "-GTTDemoSmokeScenario",
        "-GTTFarmCargoRuntimeScenario",
        "farm_cargo_runtime_scenario = $true",
    )
    package_flow = require(
        ".github/workflows/win64-package-evidence.yml",
        "MinimumAliveSeconds 200",
        "LaunchTimeoutSeconds 220",
        "evaluate_farm_cargo_runtime.ps1",
        "FARM_CARGO_RUNTIME.json",
    )
    candidate = require(
        "Scripts/evaluate_demo_candidate.ps1",
        "FARM_CARGO_RUNTIME.json",
        "gtt.farm-cargo-runtime.v1",
        "farm_cargo_runtime='PASS'",
    )
    _ = smoke, package_flow, candidate

    playtest = require(
        "Docs/PLAYTEST_0.1.29.md",
        "wrong vehicle",
        "Hill Farm",
        "North Wood Yard",
        "packaged",
        "Win64",
    )
    changelog = require(
        "CHANGELOG.d/0.1.29.md",
        "GTT 0.1.29",
        "Deterministic Packaged Farm Cargo Runtime Exercise",
        "does not claim",
    )
    vertical = require(
        "Docs/DEMO_VERTICAL_SLICE.md",
        "FARM_CARGO_RUNTIME.json",
        "exact loaded vehicle",
    )
    _ = playtest, changelog, vertical

    roadmap = text("Docs/ROADMAP.md")
    if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
        raise AssertionError("SWIR Roadmap v1 marker missing")
    checks = re.findall(r"^- \[([ xX])\]", roadmap, flags=re.M)
    done = sum(1 for mark in checks if mark.lower() == "x")
    total = len(checks)
    if (done, total) != (125, 130):
        raise AssertionError(f"runtime-only milestone must not alter roadmap completion: {done}/{total}")
    for token in (
        "125 / 130",
        "96.2%",
        "███████████████████░ 96.2%",
        "assets/readme/progress-mini.svg",
    ):
        if token not in roadmap:
            raise AssertionError(f"roadmap dashboard drift: missing {token}")

    readme = require(
        "README.md",
        "<!-- SWIR-README-STANDARD:v2 -->",
        "## 🔎 Search Keywords",
        "assets/readme/progress-card.svg",
        "0.1.29",
        "FARM_CARGO_RUNTIME.json",
        "No public demo release is available yet.",
    )
    if "release readiness remains not ready" not in readme.lower():
        raise AssertionError("README must keep roadmap completion separate from release readiness")

    subprocess.run([sys.executable, str(ROOT / "Scripts/generate_progress_svg.py"), "--check"], check=True)
    print("[OK] GTT 0.1.29 Farm Cargo packaged-runtime harness is wired through exact-vehicle authority, payout/reputation/save observation, strict Win64 evidence fields and the demo technical gate.")
    print("[OK] Roadmap remains 125/130 (96.2%); Progress SVG values remain derived from the protected checklist; release readiness remains separate.")


if __name__ == "__main__":
    main()
