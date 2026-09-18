#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing required file: {rel}")
    return path.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise AssertionError(f"{label}: missing {needle!r}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise AssertionError(f"{label}: forbidden {needle!r}")


def roadmap_progress(text: str) -> tuple[int, int, float]:
    done = len(re.findall(r"^\s*- \[x\]", text, flags=re.MULTILINE | re.IGNORECASE))
    open_ = len(re.findall(r"^\s*- \[ \]", text, flags=re.MULTILINE))
    total = done + open_
    if total <= 0:
        raise AssertionError("roadmap: no checklist items")
    return done, total, round(done * 100.0 / total, 1)


def main() -> int:
    header = read("Source/GTT/Public/Activities/GTTFarmCargoBreakdownRecoverySubsystem.h")
    cpp = read("Source/GTT/Private/Activities/GTTFarmCargoBreakdownRecoverySubsystem.cpp")
    authority_h = read("Source/GTT/Public/Activities/GTTFarmCargoAuthoritySubsystem.h")
    authority_cpp = read("Source/GTT/Private/Activities/GTTFarmCargoAuthoritySubsystem.cpp")
    roadside_h = read("Source/GTT/Public/Vehicles/GTTRoadsideRecoverySubsystem.h")
    roadside_cpp = read("Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp")
    recovery_runtime = read("Scripts/verify_farm_cargo_recovery_runtime.py")
    roadmap = read("Docs/ROADMAP.md")
    readme = read("README.md")

    require(header, "UGTTFarmCargoBreakdownRecoverySubsystem : public UTickableWorldSubsystem", "runtime subsystem")
    require(header, "TowPending", "runtime state")
    require(header, "PoliceImpoundPending", "runtime state")
    require(header, "AwaitingExactVehicle", "runtime state")
    require(cpp, "World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>()", "authority integration")
    require(cpp, "World->GetSubsystem<UGTTBreakdownDecisionSubsystem>()", "breakdown integration")
    require(cpp, "World->GetSubsystem<UGTTRoadsideRecoverySubsystem>()", "roadside integration")
    require(cpp, "IsRoadsideTowPending", "tow integration")

    require(authority_h, "FName GetBoundCargoVehicleId()", "cargo authority")
    require(authority_h, "bool TryRebindBoundVehicle()", "cargo authority")
    require(authority_cpp, "ResolveVehicleByPersistentId", "cargo authority")
    require(cpp, "CargoAuthority->TryRebindBoundVehicle()", "exact-ID recovery")
    require(cpp, "NativeVehicle->GetPersistentVehicleId() == ExpectedId", "post-recovery identity proof")
    require(cpp, "transfer_allowed=NO", "anti-transfer evidence")
    forbid(cpp, "BindLoadedVehicle(Driver", "recovery must not manufacture a new cargo binding")

    require(roadside_h, "RequestRoadsideTow", "roadside authority")
    require(roadside_cpp, "Economy->SpendCash", "tow economy authority")
    require(roadside_cpp, "damage_preserved", "tow damage contract")
    require(cpp, "CheckpointPrimarySave(TEXT(\"cargo-roadside-tow-pre-move\"))", "pre-tow checkpoint")
    require(cpp, "CheckpointPrimarySave(TEXT(\"cargo-police-impound-pre-move\"))", "pre-impound checkpoint")
    require(cpp, "CheckpointPrimarySave(TEXT(\"cargo-recovery-post-move\"))", "post-recovery checkpoint")
    require(cpp, "GameMode->SaveProgress()", "primary save integration")

    require(cpp, "timer_paused=NO", "timer continuity")
    forbid(cpp, "SetActorTransform", "cargo policy must not perform the tow itself")
    forbid(cpp, "SpendCash", "cargo policy must not duplicate tow billing")
    forbid(cpp, "CargoIntegrity =", "cargo policy must not own cargo integrity")
    forbid(cpp, "AddCash", "cargo policy must not own payout")
    forbid(cpp, "AddReputation", "cargo policy must not own reputation")

    require(recovery_runtime, "FARM_CARGO_RECOVERY_RUNTIME", "0.1.31 recovery evidence")

    require(readme, "<!-- SWIR-README-STANDARD:v2 -->", "README standard")
    require(readme, "## 🔎 Search Keywords", "README SEO")
    for token in (
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
        "<!-- ROADMAP-PROGRESS:START -->",
        "<!-- ROADMAP-PROGRESS:END -->",
        "📊 Overall progress",
        "ROADMAP-PROGRESS",
        "../assets/readme/progress-mini.svg",
    ):
        require(roadmap, token, "roadmap SVG-only standard")

    done, total, pct = roadmap_progress(roadmap)
    if (done, total) != (125, 130):
        raise AssertionError(f"roadmap changed without runtime evidence: {done}/{total}")
    if pct != 96.2:
        raise AssertionError(f"roadmap math mismatch: {pct}")
    for token in ("ROADMAP-96.2%25", "DONE-125%2F130", "| **125** | **5** | **130** | **96.2%** |"):
        require(roadmap, token, "roadmap numeric dashboard")
    if roadmap.count("../assets/readme/progress-mini.svg") != 1:
        raise AssertionError("roadmap must embed exactly one progress-mini.svg")
    if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
        raise AssertionError("legacy text/Unicode roadmap progress meter must not return")

    print("GTT 0.1.32 cargo breakdown/tow recovery contract: PASS")
    print(f"roadmap={done}/{total} ({pct:.1f}%) release_readiness=NOT_READY presentation=SVG_ONLY")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
