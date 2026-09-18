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

    # 0.1.32 must be a real auto-running world subsystem, not a test-only helper.
    require(header, "UGTTFarmCargoBreakdownRecoverySubsystem : public UTickableWorldSubsystem", "runtime subsystem")
    require(header, "TowPending", "runtime state")
    require(header, "PoliceImpoundPending", "runtime state")
    require(header, "AwaitingExactVehicle", "runtime state")
    require(cpp, "World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>()", "authority integration")
    require(cpp, "World->GetSubsystem<UGTTBreakdownDecisionSubsystem>()", "breakdown integration")
    require(cpp, "World->GetSubsystem<UGTTRoadsideRecoverySubsystem>()", "roadside integration")
    require(cpp, "IsRoadsideTowPending", "tow integration")

    # Exact identity remains owned by the existing cargo authority. Recovery may rebind only
    # that persistent ID; there must be no 'nearest vehicle inherits cargo' escape hatch.
    require(authority_h, "FName GetBoundCargoVehicleId()", "cargo authority")
    require(authority_h, "bool TryRebindBoundVehicle()", "cargo authority")
    require(authority_cpp, "ResolveVehicleByPersistentId", "cargo authority")
    require(cpp, "CargoAuthority->TryRebindBoundVehicle()", "exact-ID recovery")
    require(cpp, "NativeVehicle->GetPersistentVehicleId() == ExpectedId", "post-recovery identity proof")
    require(cpp, "transfer_allowed=NO", "anti-transfer evidence")
    forbid(cpp, "BindLoadedVehicle(Driver", "recovery must not manufacture a new cargo binding")

    # Player-authorized roadside tow stays the existing authority for cost, movement and damage.
    require(roadside_h, "RequestRoadsideTow", "roadside authority")
    require(roadside_cpp, "Economy->SpendCash", "tow economy authority")
    require(roadside_cpp, "damage_preserved", "tow damage contract")
    require(cpp, "CheckpointPrimarySave(TEXT(\"cargo-roadside-tow-pre-move\"))", "pre-tow checkpoint")
    require(cpp, "CheckpointPrimarySave(TEXT(\"cargo-police-impound-pre-move\"))", "pre-impound checkpoint")
    require(cpp, "CheckpointPrimarySave(TEXT(\"cargo-recovery-post-move\"))", "post-recovery checkpoint")
    require(cpp, "GameMode->SaveProgress()", "primary save integration")

    # Recovery is a consequence, not a pause/teleport cheat: the Farm Job director remains the
    # contract/timer/integrity authority and the new layer explicitly records that time continues.
    require(cpp, "timer_paused=NO", "timer continuity")
    forbid(cpp, "SetActorTransform", "cargo policy must not perform the tow itself")
    forbid(cpp, "SpendCash", "cargo policy must not duplicate tow billing")
    forbid(cpp, "CargoIntegrity =", "cargo policy must not own cargo integrity")
    forbid(cpp, "AddCash", "cargo policy must not own payout")
    forbid(cpp, "AddReputation", "cargo policy must not own reputation")

    # Preserve the already-established packaged save/load evidence rather than replacing it.
    require(recovery_runtime, "FARM_CARGO_RECOVERY_RUNTIME", "0.1.31 recovery evidence")

    # Documentation standards remain locked to the current SWIR contracts.
    require(readme, "<!-- SWIR-README-STANDARD:v2 -->", "README standard")
    require(readme, "## 🔎 Search Keywords", "README SEO")
    require(roadmap, "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "roadmap standard")
    require(roadmap, "📊 Overall progress", "roadmap dashboard")
    require(roadmap, "ROADMAP-PROGRESS", "roadmap progress block")

    done, total, pct = roadmap_progress(roadmap)
    if (done, total) != (125, 130):
        raise AssertionError(f"roadmap changed without runtime evidence: {done}/{total}")
    if pct != 96.2:
        raise AssertionError(f"roadmap math mismatch: {pct}")
    require(roadmap, "███████████████████░ 96.2%", "roadmap ASCII bar")

    print("GTT 0.1.32 cargo breakdown/tow recovery contract: PASS")
    print(f"roadmap={done}/{total} ({pct:.1f}%) release_readiness=NOT_READY")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
