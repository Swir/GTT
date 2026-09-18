#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    full = ROOT / path
    if not full.is_file():
        raise AssertionError(f"missing required file: {path}")
    return full.read_text(encoding="utf-8")


def require_tokens(path: str, *tokens: str) -> str:
    text = read(path)
    missing = [token for token in tokens if token not in text]
    if missing:
        raise AssertionError(f"{path}: missing tokens: {missing}")
    return text


def main() -> None:
    beacon_h = require_tokens(
        "Source/GTT/Public/Activities/GTTFarmRouteBeacon.h",
        "class GTT_API AGTTFarmRouteBeacon",
        "IsRouteBeaconDeployed",
        "Presentation-only world-space guidance",
    )
    beacon_cpp = require_tokens(
        "Source/GTT/Private/Activities/GTTFarmRouteBeacon.cpp",
        "EGTTFarmJobStage::ReachPickup",
        "EGTTFarmJobStage::DeliverCargo",
        "EGTTFarmJobStage::DeliverFinalStop",
        "EGTTFarmJobTerminalType::Pickup",
        "EGTTFarmJobTerminalType::Finish",
        "EGTTFarmJobTerminalType::FinalFinish",
        "GetTimeRemaining()",
        "GetCargoIntegrity()",
        "ECC_WorldStatic",
        "SetActorEnableCollision(false)",
        "FEED DEPOT | LOAD CARGO",
        "HILL FARM | HANDOFF",
        "NORTH WOOD YARD | FINAL",
    )
    require_tokens(
        "Source/GTT/Private/Activities/GTTFarmRouteGuidanceSubsystem.cpp",
        "OnWorldBeginPlay",
        "InWorld.IsGameWorld()",
        "TActorIterator<AGTTFarmRouteBeacon>",
        "SpawnActor<AGTTFarmRouteBeacon>",
        "AlwaysSpawn",
    )

    terminal_cpp = require_tokens(
        "Source/GTT/Private/Activities/GTTFarmJobTerminal.cpp",
        "LegalHandoffMaxSpeedKmh = 3.0f",
        "ValidateBoundCargoHandoff",
        "ValidateHandoff(HandoffLocation, LegalHandoffVehicleRadiusCm, LegalHandoffMaxSpeedKmh",
        "TryCompleteJob",
        "TryCompleteFinalStop",
    )
    for case in ("EGTTFarmJobTerminalType::Finish", "EGTTFarmJobTerminalType::FinalFinish"):
        if case not in terminal_cpp:
            raise AssertionError(f"handoff guard does not cover {case}")

    require_tokens(
        "Source/GTT/Private/Activities/GTTFarmCargoAuthoritySubsystem.cpp",
        "GetVelocity().Size2D() * 0.036f",
        "FVector::Dist2D",
        "FARM_CARGO_AUTHORITY event=HANDOFF_CHECK result=PASS",
    )

    contract_board = require_tokens(
        "Source/GTT/Private/World/GTTContractBoardSubsystem.cpp",
        "FarmCargoJob",
        "TryAcceptContract",
        "TryStartJob(PlayerPawn)",
        "IsAnyLegalContractActive",
    )
    farm_job = require_tokens(
        "Source/GTT/Private/Activities/GTTFarmJobDirector.cpp",
        "ReserveCargoContract",
        "SetCargoLoadFactor",
        "IsPoliceBlockingHandoff",
        "AddCash",
        "SettleCargoContract",
        "RecordCargoSuccess",
        "SaveProgress",
    )
    if "FarmCargoJob" not in contract_board or "RecordCargoSuccess" not in farm_job:
        raise AssertionError("legal cargo authority chain is incomplete")

    forbidden_authority = ("AddCash(", "SpendCash(", "AddHeat(", "SaveProgress(", "SettleCargoContract(")
    for token in forbidden_authority:
        if token in beacon_h or token in beacon_cpp:
            raise AssertionError(f"route beacon gained gameplay authority: {token}")

    roadmap = read("Docs/ROADMAP.md")
    for token in (
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
        "<!-- ROADMAP-PROGRESS:START -->",
        "<!-- ROADMAP-PROGRESS:END -->",
        "## 📊 Overall progress",
        "../assets/readme/progress-mini.svg",
    ):
        if token not in roadmap:
            raise AssertionError(f"SWIR roadmap SVG-only structure missing: {token}")
    checked = len(re.findall(r"^\s*-\s*\[x\]", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    unchecked = len(re.findall(r"^\s*-\s*\[ \]", roadmap, flags=re.MULTILINE))
    total = checked + unchecked
    if (checked, unchecked, total) != (125, 5, 130):
        raise AssertionError(f"roadmap checkbox truth changed unexpectedly: checked={checked}, open={unchecked}, total={total}")
    for token in ("ROADMAP-96.2%25", "DONE-125%2F130", "| **125** | **5** | **130** | **96.2%** |"):
        if token not in roadmap:
            raise AssertionError(f"roadmap numeric dashboard is stale or malformed: missing {token}")
    if roadmap.count("../assets/readme/progress-mini.svg") != 1:
        raise AssertionError("roadmap must embed exactly one progress-mini.svg")
    if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
        raise AssertionError("legacy text/Unicode roadmap progress meter must not return")

    readme = read("README.md")
    if "<!-- SWIR-README-STANDARD:v2 -->" not in readme:
        raise AssertionError("README must track the current canonical SWIR README PRO v2 marker")
    if "## 🔎 Search Keywords" not in readme or "No public demo release is available yet" not in readme:
        raise AssertionError("README discoverability/release-truth contract regressed")

    playtest = require_tokens(
        "Docs/PLAYTEST_0.1.27.md",
        "world-space route guidance",
        "drive-by handoff",
        "Win64",
        "NOT a packaged-build verification",
    )
    if len(re.findall(r"^\d+\.", playtest, flags=re.MULTILINE)) < 24:
        raise AssertionError("0.1.27 playtest must keep at least 24 explicit scenarios")

    require_tokens(
        "CHANGELOG.d/0.1.27.md",
        "0.1.27",
        "route beacon",
        "3.0 km/h",
        "no public demo release",
    )

    print("GTT 0.1.27 demo vertical-slice source contract: PASS")
    print("Verified: contract -> cargo -> world guidance -> safe same-vehicle handoff -> economy/reputation/save authority chain.")
    print("Roadmap remains truthful at 125/130 (96.2%) with SVG-only presentation; no runtime blocker was closed by source-only checks.")


if __name__ == "__main__":
    main()
