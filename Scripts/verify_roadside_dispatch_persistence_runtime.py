#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.40 packaged roadside-dispatch persistence evidence.

The verifier intentionally accepts later, stronger Win64 evidence windows and later technical-gate
schemas while preserving every 0.1.40 persistence guarantee. It never treats source checks as a
packaged Unreal runtime PASS.
"""
from __future__ import annotations

import re
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.is_file(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")


def require(source: str, token: str, where: str) -> None:
    assert token in source, f"{where}: missing {token!r}"


def yaml_int(source: str, pattern: str, label: str) -> int:
    match = re.search(pattern, source)
    assert match, f"Win64 evidence workflow: cannot resolve {label}"
    return int(match.group(1))


persist_h = read("Source/GTT/Public/Vehicles/GTTRoadsideDispatchPersistenceSubsystem.h")
persist_cpp = read("Source/GTT/Private/Vehicles/GTTRoadsideDispatchPersistenceSubsystem.cpp")
evidence_h = read("Source/GTT/Public/Core/GTTFarmCargoDispatchPersistenceEvidenceSubsystem.h")
evidence_cpp = read("Source/GTT/Private/Core/GTTFarmCargoDispatchPersistenceEvidenceSubsystem.cpp")
smoke = read("Scripts/smoke_test_windows.ps1")
evaluator = read("Scripts/evaluate_farm_cargo_dispatch_persistence_runtime.ps1")
demo_gate = read("Scripts/evaluate_demo_candidate.ps1")
workflow = read(".github/workflows/win64-package-evidence.yml")
roadmap = read("Docs/ROADMAP.md")
readme = read("README.md")
changelog = read("CHANGELOG.d/0.1.40.md")
playtest = read("Docs/PLAYTEST_0.1.40.md")

for token in (
    "ReloadCheckpointForRuntimeEvidence", "ResetInMemoryCheckpointState",
    "GTTFarmCargoDispatchPersistenceScenario", "EVIDENCE_FLAG_REQUIRED",
    "DoesSaveGameExist(RoadsideDispatchSlot", "LoadCheckpointOnce()",
    "NATIVE_ROADSIDE_DISPATCH_EVIDENCE_RELOAD", "checkpoint_pending=%s charged=NO",
):
    require(persist_h + persist_cpp, token, "production persistence evidence hook")

reload_start = persist_cpp.index(
    "bool UGTTRoadsideDispatchPersistenceSubsystem::ReloadCheckpointForRuntimeEvidence()"
)
reload_end = persist_cpp.index(
    "\nvoid UGTTRoadsideDispatchPersistenceSubsystem::LoadCheckpointOnce()", reload_start
)
reload_block = persist_cpp[reload_start:reload_end]
for forbidden in ("SpendCash(", "AddCash(", "ChargeFine(", "SaveGameToSlot("):
    assert forbidden not in reload_block, f"evidence re-arm contains forbidden authority: {forbidden}"
for token in ("FParse::Param", "ResetInMemoryCheckpointState", "LoadCheckpointOnce"):
    require(reload_block, token, "guarded evidence reload")

for token in (
    "UGTTFarmCargoDispatchPersistenceEvidenceSubsystem", "StartDelaySeconds = 326.0f",
    "GlobalDeadlineSeconds = 350.0f", "GTTFarmCargoDispatchPersistenceScenario",
    "GTT_RoadsideDispatch_01", "ReadDispatchCheckpoint", "SchemaVersion == 2",
    "RequestRoadsideTow", "RequestEmergencyRoadsidePatch", "CancelPendingRoadsideService",
    "GameMode->SaveProgress()", "GameMode->LoadProgress()", "ReloadCheckpointForRuntimeEvidence()",
    "Wanted->AddHeat", "WANTED_REJECT", "PATCH_RESTORED", "TOW_RESTORED",
    "PATCH_COMPLETE", "single_charge=%d", "wrong_vehicle_rejected=%d",
    "FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME_COMPLETE", "CargoRunsDelta == 1",
):
    require(evidence_h + evidence_cpp, token, "0.1.40 packaged persistence route")

for forbidden in ("CompleteCargoContract(", "AddCash(", "RecordCargoSuccess(Payout", "RestorePendingRecoveryCheckpoint("):
    assert forbidden not in evidence_cpp, f"runtime evidence harness bypasses production authority: {forbidden}"

for token in (
    "-GTTFarmCargoDispatchPersistenceScenario",
    "farm_cargo_dispatch_persistence_runtime_scenario = $true",
):
    require(smoke, token, "packaged smoke flags")

for token in (
    "gtt.farm-cargo-dispatch-persistence-runtime.v1", "tow_checkpoint_saved", "tow_primary_save",
    "tow_primary_load", "tow_restore_rearmed", "tow_restored", "tow_quote_preserved",
    "tow_eta_preserved", "tow_exact_vehicle", "tow_cancel_no_charge",
    "wanted_restore_rejected_no_charge", "wanted_sidecar_cleared", "patch_checkpoint_saved",
    "patch_primary_save", "patch_primary_load", "patch_restore_rearmed", "patch_restored",
    "patch_quote_preserved", "patch_eta_preserved", "patch_exact_vehicle",
    "patch_no_charge_before_arrival", "patch_single_charge", "cargo_timer_continued",
    "cargo_integrity_not_improved", "wrong_vehicle_rejected", "diagnostic_failure_count",
):
    require(evaluator, token, "0.1.40 runtime evaluator")

# The historical evaluator must still emit schema 12 with all 0.1.40 fields. A later workflow may
# strengthen that already-passed gate (for example schema 13) only after additional evidence.
for token in (
    "FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME.json", "schema=12",
    "gtt.farm-cargo-dispatch-persistence-runtime.v1", "farm_cargo_dispatch_persistence_runtime='PASS'",
    "farm_cargo_dispatch_persistence_patch_single_charge", "farm_cargo_dispatch_persistence_wanted_rejected",
):
    require(demo_gate, token, "demo technical gate schema 12 foundation")
assert "schema=11" not in demo_gate, "demo gate still emits schema 11"

# Later milestones are allowed to extend the same packaged run. They must never shrink the 0.1.40
# runtime window or remove its evaluator/manifest/hash gates.
require(workflow, "evaluate_farm_cargo_dispatch_persistence_runtime.ps1", "Win64 persistence evaluator")
require(workflow, "FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME.json", "Win64 persistence manifest")
require(workflow, "Get-FileHash -Algorithm SHA256 -Path $zip", "Win64 release ZIP hash")
assert "-Path $zip.FullName" not in workflow, "release ZIP hash still dereferences a string as FullName"
version_match = re.search(r"default:\s*'([0-9]+\.[0-9]+\.[0-9]+)'", workflow)
assert version_match, "Win64 workflow version default missing"
version_tuple = tuple(int(part) for part in version_match.group(1).split("."))
assert version_tuple >= (0, 1, 40), f"Win64 workflow regressed below 0.1.40: {version_match.group(1)}"
minimum_alive = yaml_int(workflow, r"-MinimumAliveSeconds\s+(\d+)", "MinimumAliveSeconds")
launch_timeout = yaml_int(workflow, r"-LaunchTimeoutSeconds\s+(\d+)", "LaunchTimeoutSeconds")
minimum_runtime = yaml_int(workflow, r"-MinimumRuntimeSeconds\s+(\d+)", "MinimumRuntimeSeconds")
assert minimum_alive >= 354, f"Win64 smoke window regressed below 354s: {minimum_alive}"
assert launch_timeout >= 385, f"Win64 launch timeout regressed below 385s: {launch_timeout}"
assert launch_timeout > minimum_alive, "Win64 launch timeout must exceed minimum alive window"
assert minimum_runtime >= 354, f"packaged gameplay runtime regressed below 354s: {minimum_runtime}"

checks = re.findall(r"^-\s*\[(x| )\]\s+", roadmap, flags=re.MULTILINE | re.IGNORECASE)
done = sum(item.lower() == "x" for item in checks)
assert (done, len(checks)) == (125, 130), f"source work must not close runtime/art gates: {done}/{len(checks)}"
for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "<!-- ROADMAP-PROGRESS:START -->", "<!-- ROADMAP-PROGRESS:END -->",
    "../assets/readme/progress-mini.svg", "**125** | **5** | **130** | **96.2%**",
):
    require(roadmap, token, "ROADMAP")
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(r"[█▓▒░]{4,}|\[[#=\-]{6,}\]", roadmap), "retired character progress meter returned"

for token in (
    "<!-- SWIR-README-STANDARD:v2 -->", "assets/readme/progress-card.svg", "## 🔎 Search Keywords",
    "Release readiness: **NOT READY**",
):
    require(readme, token, "README")
# README may advance to later milestones; it must never falsely claim an earlier source-only route is packaged proof.
assert re.search(r"0\.1\.(?:4[0-9]|[5-9][0-9]|[1-9][0-9]{2,})", readme), "README milestone regressed below 0.1.40"
assert readme.count("assets/readme/progress-card.svg") == 1
assert "progress-mini.svg" not in readme
assert not re.search(r"[█▓▒░]{4,}|\[[#=\-]{6,}\]", readme), "retired README progress meter returned"

for path, needles in (
    ("assets/readme/progress-card.svg", ("96.2%", "125 / 130", "NOT READY", "#02050A", "#07111C", "#0088FF", "#62E5FF")),
    ("assets/readme/progress-mini.svg", ("96.2%", "125 / 130", "#02050A", "#0088FF", "#62E5FF")),
    ("assets/readme/progress-template.svg", ("TEMPLATE", "N/A")),
):
    data = read(path)
    ET.fromstring(data)
    for needle in needles:
        require(data, needle, path)
require(read("Scripts/generate_progress_svg.py"), "SWIR-PROGRESS-SVG-PRO:v1", "progress generator")

for token in (
    "GTT 0.1.40", "dispatch persistence", "SaveGame", "locked quote", "ETA", "Wanted",
    "single-charge", "schema 12", "does not prove",
):
    assert token.lower() in changelog.lower(), f"0.1.40 changelog missing {token!r}"

scenarios = len(re.findall(r"^\d+\.\s+", playtest, flags=re.MULTILINE))
assert scenarios >= 80, f"PLAYTEST_0.1.40.md must contain at least 80 numbered scenarios, found {scenarios}"

print("GTT 0.1.40 packaged roadside dispatch persistence source contract: PASS")
print(f"Roadmap remains {done}/{len(checks)} = {done / len(checks) * 100:.1f}% until real Win64/Chaos/trailer/visual evidence exists")
print(f"Later packaged window accepted safely: version={version_match.group(1)} alive={minimum_alive}s runtime={minimum_runtime}s timeout={launch_timeout}s")
print("NOTE: source verifier only; no Unreal compile/package/runtime claim.")
