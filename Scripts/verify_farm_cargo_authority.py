#!/usr/bin/env python3
from __future__ import annotations

import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    if not p.is_file():
        raise AssertionError(f"missing required file: {path}")
    return p.read_text(encoding="utf-8")


def require(path: str, *tokens: str) -> str:
    text = read(path)
    missing = [t for t in tokens if t not in text]
    if missing:
        raise AssertionError(f"{path}: missing tokens: {missing}")
    return text


def main() -> None:
    authority_h = require(
        "Source/GTT/Public/Activities/GTTFarmCargoAuthoritySubsystem.h",
        "class GTT_API UGTTFarmCargoAuthoritySubsystem",
        "CaptureLoadedVehicle",
        "ValidateHandoff",
        "MarkAcceptedHandoff",
        "TWeakObjectPtr<AActor> LoadedVehicle",
        "GetLoadedVehicleId",
    )
    authority_cpp = require(
        "Source/GTT/Private/Activities/GTTFarmCargoAuthoritySubsystem.cpp",
        "CargoPickupRadiusCm = 700.0f",
        "CargoHandoffRadiusCm = 750.0f",
        "CargoHandoffMaxSpeedKmh = 3.0f",
        "wrong-vehicle-or-loaded-vehicle-away",
        "loaded-vehicle-moving",
        "FARM_CARGO_AUTHORITY event=LOAD_LOCK",
        "FARM_CARGO_AUTHORITY event=HANDOFF_ACCEPT",
        "LoadedVehicle.Get()",
        "GetVelocity().Size2D() * 0.036f",
    )
    for forbidden in ("AddCash(", "SpendCash(", "AddHeat(", "SaveProgress(", "SettleCargoContract(", "RecordCargoSuccess("):
        if forbidden in authority_h or forbidden in authority_cpp:
            raise AssertionError(f"cargo authority illegally owns unrelated gameplay state: {forbidden}")

    terminal = require(
        "Source/GTT/Private/Activities/GTTFarmJobTerminal.cpp",
        "UGTTFarmCargoAuthoritySubsystem",
        "ResetForNewContract",
        "CaptureLoadedVehicle(Pawn)",
        "ValidateHandoff(GetActorLocation()",
        "HILL_FARM",
        "NORTH_WOOD_YARD",
        "pickup-rejected",
        "contract-complete",
    )
    if "FindNearbyWorkVehicle" in terminal:
        raise AssertionError("FarmJobTerminal must not reintroduce generic nearby-vehicle handoff authority")

    scenario_h = require(
        "Source/GTT/Public/Core/GTTFarmCargoEvidenceScenarioSubsystem.h",
        "UGTTFarmCargoEvidenceScenarioSubsystem",
        "UTickableWorldSubsystem",
    )
    scenario_cpp = require(
        "Source/GTT/Private/Core/GTTFarmCargoEvidenceScenarioSubsystem.cpp",
        "GTTFarmCargoScenario",
        "FARM_CARGO_SCENARIO_BEGIN version=1",
        "Pickup->Interact_Implementation(PlayerPawn)",
        "HillTerminal->Interact_Implementation(PlayerPawn)",
        "FinalTerminal->Interact_Implementation(PlayerPawn)",
        "WRONG_VEHICLE_REJECT",
        "PAYOUT_REPUTATION",
        "SaveProgress",
        "FARM_CARGO_SCENARIO_COMPLETE result=PASS",
    )
    if "AddCash(" in scenario_cpp or "RecordCargoSuccess(" in scenario_cpp:
        raise AssertionError("runtime evidence scenario must observe economy/reputation, not fabricate it")
    _ = scenario_h

    smoke = require(
        "Scripts/smoke_test_farm_cargo_windows.ps1",
        "GTTFarmCargoScenario",
        "GTT_FARM_CARGO_RUNTIME.log",
        "FARM_CARGO_SMOKE.json",
        "gtt.farm-cargo-smoke.v1",
        "FARM_CARGO_SCENARIO_COMPLETE result=PASS",
        "FarmCargoRuntimeUser",
    )
    evaluator = require(
        "Scripts/evaluate_farm_cargo_runtime.ps1",
        "FARM_CARGO_SMOKE.json",
        "FARM_CARGO_RUNTIME.json",
        "gtt.farm-cargo-runtime.v1",
        "wrong_vehicle_rejection_passed",
        "max_accepted_handoff_speed_kmh",
        "max_accepted_handoff_distance_cm",
        "cash_delta",
        "cargo_runs_delta",
    )
    if "result='PASS'" not in evaluator:
        raise AssertionError("Farm Cargo evaluator does not emit a PASS manifest after validation")
    _ = smoke

    candidate = require(
        "Scripts/evaluate_demo_candidate.ps1",
        "FARM_CARGO_RUNTIME.json",
        "gtt.farm-cargo-runtime.v1",
        "farm_cargo_runtime='PASS'",
        "schema=7",
        "farm_cargo_wrong_vehicle_rejection",
        "farm_cargo_cash_delta",
    )
    if "schema=6 was the previous gate revision" not in candidate or "schema=5" not in candidate:
        raise AssertionError("technical gate lost historical schema-6 compatibility note")

    workflow = require(
        ".github/workflows/win64-package-evidence.yml",
        "default: '0.1.28'",
        "Run packaged Farm Cargo vertical-slice runtime exercise",
        "smoke_test_farm_cargo_windows.ps1",
        "evaluate_farm_cargo_runtime.ps1",
        "FARM_CARGO_RUNTIME.json",
        "GTT_FARM_CARGO_RUNTIME.log",
        "runs-on: [self-hosted, windows, x64, unreal-5.8]",
    )
    order = [
        "evaluate_authored_trailer_runtime.ps1",
        "smoke_test_farm_cargo_windows.ps1",
        "evaluate_farm_cargo_runtime.ps1",
        "evaluate_demo_candidate.ps1",
    ]
    positions = [workflow.find(token) for token in order]
    if any(p < 0 for p in positions) or positions != sorted(positions):
        raise AssertionError("Win64 gate order must be trailer -> Farm Cargo smoke -> Farm Cargo evaluator -> technical candidate")

    roadmap = read("Docs/ROADMAP.md")
    checked = len(re.findall(r"^\s*-\s*\[x\]", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    unchecked = len(re.findall(r"^\s*-\s*\[ \]", roadmap, flags=re.MULTILINE))
    if (checked, unchecked, checked + unchecked) != (125, 5, 130):
        raise AssertionError(f"roadmap truth changed without runtime evidence: {checked}/{checked + unchecked}, open={unchecked}")
    for token in (
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
        "ROADMAP-96.2%25",
        "DONE-125%2F130",
        "## 📊 Overall progress",
        "███████████████████░ 96.2%",
        "| **125** | **5** | **130** | **96.2%** |",
        "../assets/readme/progress-mini.svg",
    ):
        if token not in roadmap:
            raise AssertionError(f"roadmap standard/progress SVG drift: {token}")

    readme = read("README.md")
    for token in (
        "<!-- SWIR-README-STANDARD:v2 -->",
        "assets/readme/progress-card.svg",
        "Roadmap completion: **125 / 130 (96.2%)**",
        "No public demo release is available yet",
        "## 🔎 Search Keywords",
        "0.1.28",
    ):
        if token not in readme:
            raise AssertionError(f"README progress/release truth drift: {token}")

    subprocess.run([sys.executable, str(ROOT / "Scripts/generate_progress_svg.py"), "--check"], cwd=ROOT, check=True)
    for svg_path in (
        ROOT / "assets/readme/progress-card.svg",
        ROOT / "assets/readme/progress-mini.svg",
        ROOT / "assets/readme/progress-template.svg",
    ):
        root = ET.parse(svg_path).getroot()
        if "viewBox" not in root.attrib:
            raise AssertionError(f"{svg_path.name} missing viewBox")
        text = svg_path.read_text(encoding="utf-8")
        for color in ("#02050A", "#62E5FF"):
            if color not in text:
                raise AssertionError(f"{svg_path.name} missing SWIR progress palette color {color}")
    template = read("assets/readme/progress-template.svg")
    if "TEMPLATE" not in template or "not project data" not in template.lower():
        raise AssertionError("progress template is not clearly labelled TEMPLATE / NOT PROJECT DATA")

    playtest = require(
        "Docs/PLAYTEST_0.1.28.md",
        "exact loaded vehicle",
        "decoy",
        "3.0 km/h",
        "750 cm",
        "FARM_CARGO_RUNTIME.json",
        "Win64",
        "NOT a packaged-build verification",
        "progress-card.svg",
    )
    if len(re.findall(r"^\d+\.", playtest, flags=re.MULTILINE)) < 30:
        raise AssertionError("0.1.28 playtest must contain at least 30 explicit scenarios")

    require(
        "Docs/FARM_CARGO_RUNTIME_ACCEPTANCE.md",
        "GTTFarmCargoScenario",
        "FARM_CARGO_RUNTIME.json",
        "wrong-vehicle",
        "payout",
        "does not prove",
    )
    require(
        "CHANGELOG.d/0.1.28.md",
        "0.1.28",
        "exact loaded vehicle",
        "FARM_CARGO_RUNTIME.json",
        "SWIR Progress SVG PRO",
        "no public demo release",
    )

    print("GTT 0.1.28 Farm Cargo authority + packaged vertical-slice contract: PASS")
    print("Verified source wiring: exact load lock -> terminal rejection/acceptance -> payout/reputation/save observation -> Win64 evidence gate.")
    print("SWIR Progress SVG PRO: PASS — roadmap remains exactly 125/130 (96.2%); release readiness remains separate.")


if __name__ == "__main__":
    main()
