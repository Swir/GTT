#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.42 packaged garage/workshop recovery evidence."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def text(path: str) -> str:
    p = ROOT / path
    if not p.is_file():
        raise AssertionError(f"missing required file: {path}")
    return p.read_text(encoding="utf-8")


def require(source: str, needle: str, label: str) -> None:
    if needle not in source:
        raise AssertionError(f"missing {label}: {needle}")


def require_min_win64_window(source: str, minimum_alive: int, minimum_timeout: int) -> None:
    match = re.search(r"smoke_test_windows\.ps1[^\n]*-MinimumAliveSeconds\s+(\d+)\s+-LaunchTimeoutSeconds\s+(\d+)", source)
    if not match:
        raise AssertionError("could not parse Win64 smoke runtime window")
    alive, timeout = map(int, match.groups())
    if alive < minimum_alive or timeout < minimum_timeout or timeout <= alive:
        raise AssertionError(f"Win64 smoke runtime window regressed: alive={alive}, timeout={timeout}")


def require_min_candidate_version(source: str, minimum=(0, 1, 42)) -> None:
    match = re.search(r"default:\s*'([0-9]+)\.([0-9]+)\.([0-9]+)'", source)
    if not match:
        raise AssertionError("could not parse Win64 default version")
    version = tuple(map(int, match.groups()))
    if version < minimum:
        raise AssertionError(f"Win64 default version regressed below 0.1.42: {version}")


def require_min_final_gate_schema(source: str, minimum: int) -> None:
    schemas = [int(value) for value in re.findall(r"gate\.schema\s+-ne\s+(\d+)", source)]
    if not schemas or max(schemas) < minimum:
        raise AssertionError(f"final technical gate schema regressed below {minimum}: {schemas}")


def main() -> int:
    evidence_h = text("Source/GTT/Public/Core/GTTFarmCargoWorkshopRecoveryEvidenceSubsystem.h")
    evidence_cpp = text("Source/GTT/Private/Core/GTTFarmCargoWorkshopRecoveryEvidenceSubsystem.cpp")
    service_h = text("Source/GTT/Public/World/GTTServiceTerminal.h")
    service_cpp = text("Source/GTT/Private/World/GTTServiceTerminal.cpp")
    garage_h = text("Source/GTT/Public/World/GTTGarageFleetSubsystem.h")
    garage_slot = text("Source/GTT/Private/World/GTTGarageSlotTerminal.cpp")
    roadside = text("Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp")
    evaluator = text("Scripts/evaluate_farm_cargo_workshop_recovery_runtime.ps1")
    promote = text("Scripts/promote_demo_gate_workshop_recovery.ps1")
    smoke = text("Scripts/smoke_test_windows.ps1")
    win64 = text(".github/workflows/win64-package-evidence.yml")
    roadmap = text("Docs/ROADMAP.md")
    readme = text("README.md")

    # Runtime route must be opt-in and start after the existing 0.1.40 evidence window.
    require(evidence_h, "UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem", "workshop runtime subsystem")
    require(evidence_cpp, 'TEXT("GTTFarmCargoWorkshopRecoveryScenario")', "dedicated smoke flag")
    require(evidence_cpp, "constexpr float StartDelaySeconds = 356.0f", "ordered evidence window")
    require(evidence_cpp, "constexpr float GlobalDeadlineSeconds = 382.0f", "bounded evidence deadline")
    require(evidence_cpp, "FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME_BEGIN version=1", "runtime begin marker")
    require(evidence_cpp, "FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME_COMPLETE", "runtime completion marker")

    # Production tow + exact cargo authority, not a manufactured teleport/payment shortcut.
    require(evidence_cpp, "Roadside->RequestRoadsideTow(NativeMulebox.Get())", "production tow request")
    require(evidence_cpp, "Roadside->GetPendingRecoveryQuote", "locked tow quote")
    require(evidence_cpp, "Roadside->GetPendingRecoveryVehicleId", "pinned recovery vehicle")
    require(evidence_cpp, "Authority->GetBoundCargoVehicleId() == LoadedVehicleId", "exact cargo identity")
    require(evidence_cpp, "bTowSingleCharge", "single tow charge proof")
    require(evidence_cpp, "bTowDamagePreserved", "tow damage continuity proof")
    require(evidence_cpp, "bWorkshopDestination", "workshop destination proof")
    if "Economy->AddCash(" in evidence_cpp or "Economy->SpendCash(" in evidence_cpp:
        raise AssertionError("runtime route must observe production economy mutations, not perform its own payout/service debit")

    # 0.1.41 hold must be observed and the real garage recall must fail closed before service.
    require(garage_h, "IsVehicleOnWorkshopHold(FName VehicleId) const", "fleet workshop-hold API")
    require(evidence_cpp, "GarageFleet->IsVehicleOnWorkshopHold(LoadedVehicleId)", "runtime hold observation")
    require(evidence_cpp, "Slot->Interact_Implementation(PlayerPawn.Get())", "real garage recall attempt")
    require(evidence_cpp, "bGarageRecallNoCharge", "garage no-charge proof")
    require(evidence_cpp, "bGarageRecallNoMove", "garage no-move proof")
    require(garage_slot, "GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(Snapshot)", "garage hold policy")

    # Workshop evidence uses the real terminal and existing authoritative repair/refuel service.
    require(service_h, "EGTTServiceType GetServiceType() const", "service terminal type query")
    require(evidence_cpp, "WorkshopTerminal->GetNativeRoadRepairQuote(NativeMulebox.Get())", "authoritative workshop quote")
    require(evidence_cpp, "WorkshopTerminal->Interact_Implementation(PlayerPawn.Get())", "production workshop interaction")
    require(service_cpp, "ApplyNativeWorkshopService()", "authoritative native workshop service")
    require(service_cpp, "GameMode->SaveProgress()", "workshop persistence checkpoint")
    require(evidence_cpp, "!GarageFleet->IsVehicleOnWorkshopHold(LoadedVehicleId)", "hold clear proof")
    require(evidence_cpp, "bWorkshopSingleCharge", "single workshop charge proof")
    require(evidence_cpp, "bMechanicalRepaired", "mechanical repair proof")
    require(evidence_cpp, "bRefuelled", "refuel proof")

    # Cargo must continue through wrong-vehicle rejection and both real handoffs.
    require(evidence_cpp, "SpawnDecoyVan", "wrong-vehicle decoy")
    require(evidence_cpp, "HillTerminal->Interact_Implementation(PlayerPawn.Get())", "Hill Farm handoff")
    require(evidence_cpp, "FinalTerminal->Interact_Implementation(PlayerPawn.Get())", "North Wood Yard handoff")
    require(evidence_cpp, "CargoRunsDelta == 1", "single completion proof")
    require(evidence_cpp, "GameMode->SaveProgress()", "final save proof")

    # Evaluator and demo gate promotion must make this packaged proof mandatory for future candidates.
    require(evaluator, "gtt.farm-cargo-workshop-recovery-runtime.v1", "runtime evidence schema")
    require(evaluator, "FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json", "runtime evidence output")
    require(evaluator, "NATIVE_ROADSIDE_TOW_COMPLETE", "production tow marker validation")
    require(evaluator, "garage_recall_blocked=$true", "garage bypass manifest gate")
    require(evaluator, "workshop_hold_cleared=$true", "hold-clear manifest gate")
    require(promote, "schema=13", "technical gate schema 13")
    require(promote, "farm_cargo_workshop_recovery_runtime='PASS'", "technical gate workshop PASS")
    require(promote, "FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json", "promotion manifest dependency")

    require(smoke, "-GTTFarmCargoWorkshopRecoveryScenario", "packaged smoke launch flag")
    require(smoke, "farm_cargo_workshop_recovery_runtime_scenario = $true", "smoke evidence field")
    require_min_candidate_version(win64)
    require_min_win64_window(win64, 386, 415)
    require(win64, "evaluate_farm_cargo_workshop_recovery_runtime.ps1", "Win64 workshop evaluator")
    require(win64, "promote_demo_gate_workshop_recovery.ps1", "Win64 gate promotion")
    if win64.count("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json") < 4:
        raise AssertionError("workshop runtime manifest must be validated and retained in candidate/diagnostic paths")
    require_min_final_gate_schema(win64, 13)

    # Existing roadside contract remains damage preserving and workshop directed.
    require(roadside, "damage_preserved=%s", "tow damage marker")
    require(roadside, "identity_preserved=%s", "tow identity marker")
    require(roadside, "serviced=NO destination=WORKSHOP", "ordinary tow remains unserviced")

    # SWIR progress truth is unchanged: source/evidence plumbing does not close native/runtime gates.
    require(roadmap, "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "roadmap structure marker")
    require(roadmap, "../assets/readme/progress-mini.svg", "roadmap progress mini")
    require(roadmap, "| **125** | **5** | **130** | **96.2%** |", "authoritative roadmap count")
    require(readme, "<!-- SWIR-README-STANDARD:v2 -->", "README v2 marker")
    require(readme, "assets/readme/progress-card.svg", "README progress card")
    require(readme, "## 🔎 Search Keywords", "README search keywords")
    progress_block = roadmap.split("<!-- ROADMAP-PROGRESS:START -->", 1)[1].split("<!-- ROADMAP-PROGRESS:END -->", 1)[0]
    legacy_patterns = [r"[█▓▒░]{4,}", r"\[[#=\-]{5,}\]"]
    if any(re.search(pattern, progress_block) for pattern in legacy_patterns):
        raise AssertionError("legacy text progress meter returned to ROADMAP-PROGRESS")

    print("GTT 0.1.42 packaged garage/workshop recovery evidence: PASS")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, IndexError, ValueError) as exc:
        print(f"GTT 0.1.42 packaged garage/workshop recovery evidence: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
