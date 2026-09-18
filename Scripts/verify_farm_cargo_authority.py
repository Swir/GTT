#!/usr/bin/env python3
from __future__ import annotations

import re
import subprocess
import sys
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
    authority_h = require_tokens(
        "Source/GTT/Public/Activities/GTTFarmCargoAuthoritySubsystem.h",
        "UGTTFarmCargoAuthoritySubsystem",
        "UTickableWorldSubsystem",
        "BindLoadedVehicle",
        "ValidateHandoff",
        "BoundCargoVehicle",
        "BoundCargoVehicleId",
        "TWeakObjectPtr<APawn>",
    )
    authority_cpp = require_tokens(
        "Source/GTT/Private/Activities/GTTFarmCargoAuthoritySubsystem.cpp",
        "DepotVehicleSearchRadiusCm = 700.0f",
        "AGTTMuleboxNativePawn",
        "Mirror AGTTFarmJobDirector::TryPickupCargo exactly",
        "FindNearbyLegacyWorkVehicle",
        "GetPersistentVehicleId()",
        "FARM_CARGO_AUTHORITY event=BIND result=PASS",
        "FARM_CARGO_AUTHORITY event=HANDOFF_CHECK result=PASS",
        "FVector::Dist2D",
        "GetVelocity().Size2D() * 0.036f",
        "contract-idle",
    )
    terminal_cpp = require_tokens(
        "Source/GTT/Private/Activities/GTTFarmJobTerminal.cpp",
        "LegalHandoffMaxSpeedKmh = 3.0f",
        "LegalHandoffVehicleRadiusCm = 750.0f",
        "GetSubsystem<UGTTFarmCargoAuthoritySubsystem>",
        "Director->TryPickupCargo(Pawn)",
        "Authority->BindLoadedVehicle(Pawn, Summary)",
        "ValidateBoundCargoHandoff",
        "Director->TryCompleteJob(Pawn)",
        "Director->TryCompleteFinalStop(Pawn)",
        "direct-route-complete",
        "extended-route-complete",
    )

    pickup_pos = terminal_cpp.index("Director->TryPickupCargo(Pawn)")
    bind_pos = terminal_cpp.index("Authority->BindLoadedVehicle(Pawn, Summary)")
    if bind_pos <= pickup_pos:
        raise AssertionError("cargo vehicle must only bind after the authoritative pickup succeeds")

    finish_case = terminal_cpp.index("case EGTTFarmJobTerminalType::Finish:")
    final_case = terminal_cpp.index("case EGTTFarmJobTerminalType::FinalFinish:")
    if "ValidateBoundCargoHandoff" not in terminal_cpp[finish_case:final_case]:
        raise AssertionError("Hill Farm handoff is not guarded by physical cargo-vehicle authority")
    if "ValidateBoundCargoHandoff" not in terminal_cpp[final_case:]:
        raise AssertionError("North Wood Yard handoff is not guarded by physical cargo-vehicle authority")

    forbidden_authority = (
        "AddCash(", "SpendCash(", "AddHeat(", "SaveProgress(", "SettleCargoContract(",
        "RecordCargoSuccess(", "ReserveCargoContract(",
    )
    for token in forbidden_authority:
        if token in authority_h or token in authority_cpp:
            raise AssertionError(f"physical cargo authority gained forbidden economy/law/contract power: {token}")

    director_cpp = require_tokens(
        "Source/GTT/Private/Activities/GTTFarmJobDirector.cpp",
        "ReserveCargoContract",
        "SetCargoLoadFactor",
        "IsPoliceBlockingHandoff",
        "AddCash",
        "SettleCargoContract",
        "RecordCargoSuccess",
        "SaveProgress",
    )
    if "SettleCargoContract" not in director_cpp or "RecordCargoSuccess" not in director_cpp:
        raise AssertionError("FarmJobDirector must remain the payout/reputation authority")

    require_tokens(
        "Docs/DEMO_VERTICAL_SLICE.md",
        "same physical vehicle",
        "UGTTFarmCargoAuthoritySubsystem",
        "verified Unreal Engine 5.8 Win64 compile/cook/package",
        "no public demo release",
    )
    playtest = require_tokens(
        "Docs/PLAYTEST_0.1.28.md",
        "different vehicle",
        "same loaded vehicle",
        "3.0 km/h",
        "Win64",
        "NOT a packaged-build verification",
    )
    if len(re.findall(r"^\d+\.", playtest, flags=re.MULTILINE)) < 30:
        raise AssertionError("0.1.28 playtest must keep at least 30 explicit scenarios")

    require_tokens(
        "CHANGELOG.d/0.1.28.md",
        "0.1.28",
        "physical cargo vehicle",
        "progress-card.svg",
        "no public demo release",
    )

    readme = read("README.md")
    if "<!-- SWIR-README-STANDARD:v2 -->" not in readme:
        raise AssertionError("README v2 marker missing or downgraded")
    for token in (
        "assets/readme/progress-card.svg",
        "UGTTFarmCargoAuthoritySubsystem",
        "exact loaded vehicle",
        "No public demo release is available yet.",
    ):
        if token not in readme:
            raise AssertionError(f"README regressed the 0.1.28 physical cargo authority contract: {token}")
    if "## 🔎 Search Keywords" not in readme:
        raise AssertionError("README Search Keywords section regressed")

    roadmap = read("Docs/ROADMAP.md")
    checked = len(re.findall(r"^\s*-\s*\[x\]", roadmap, flags=re.MULTILINE | re.IGNORECASE))
    unchecked = len(re.findall(r"^\s*-\s*\[ \]", roadmap, flags=re.MULTILINE))
    if (checked, unchecked) != (125, 5):
        raise AssertionError(f"runtime blockers were changed without evidence: checked={checked}, open={unchecked}")
    for token in (
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
        "<!-- ROADMAP-PROGRESS:START -->",
        "<!-- ROADMAP-PROGRESS:END -->",
        "## 📊 Overall progress",
        "| **125** | **5** | **130** | **96.2%** |",
        "../assets/readme/progress-mini.svg",
    ):
        if token not in roadmap:
            raise AssertionError(f"roadmap standard/progress presentation regressed: {token}")
    if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
        raise AssertionError("legacy text/Unicode roadmap progress meter must not return")

    progress_check = subprocess.run(
        [sys.executable, str(ROOT / "Scripts" / "generate_progress_svg.py"), "--check"],
        cwd=ROOT,
        text=True,
        capture_output=True,
    )
    if progress_check.returncode != 0:
        raise AssertionError("SWIR Progress SVG PRO check failed:\n" + progress_check.stdout + progress_check.stderr)

    print(progress_check.stdout.strip())
    print("GTT 0.1.28 Farm Cargo physical-vehicle authority: PASS")
    print("Verified: exact loaded-vehicle binding -> same-vehicle stopped handoffs -> existing payout/reputation/save authority remains in FarmJobDirector.")
    print("Roadmap remains 125/130 (96.2%); release readiness is still NOT READY until real Win64/runtime/visual gates pass.")


if __name__ == "__main__":
    main()
