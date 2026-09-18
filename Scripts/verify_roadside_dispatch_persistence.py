#!/usr/bin/env python3
"""Source-level verifier for GTT 0.1.39 persistent roadside dispatch checkpoints.

This proves persistence wiring/invariants only. It does not claim an Unreal compile,
packaged executable, runtime smoke test, or demo readiness.
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


def require(text: str, token: str, where: str) -> None:
    assert token in text, f"{where}: missing {token!r}"


road_h = read("Source/GTT/Public/Vehicles/GTTRoadsideRecoverySubsystem.h")
road_cpp = read("Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp")
restore_cpp = read("Source/GTT/Private/Vehicles/GTTRoadsideRecoveryPersistence.cpp")
save_h = read("Source/GTT/Public/Save/GTTRoadsideDispatchSaveGame.h")
persist_h = read("Source/GTT/Public/Vehicles/GTTRoadsideDispatchPersistenceSubsystem.h")
persist_cpp = read("Source/GTT/Private/Vehicles/GTTRoadsideDispatchPersistenceSubsystem.cpp")
roadmap = read("Docs/ROADMAP.md")
readme = read("README.md")
changelog = read("CHANGELOG.d/0.1.39.md")
playtest = read("Docs/PLAYTEST_0.1.39.md")

for token in (
    "EGTTRoadsideRecoveryRestoreResult",
    "WaitingForVehicle",
    "Restored",
    "Rejected",
    "RestorePendingRecoveryCheckpoint",
):
    require(road_h, token, "roadside recovery persistence API")

for token in (
    "PersistedTowDispatchSeconds = 2.5f",
    "PersistedPatchDispatchSeconds = 3.0f",
    "TActorIterator<AGTTRoadVehicleNativePawn>",
    "Candidate->GetPersistentVehicleId() != PersistentVehicleId",
    "ExactMatchCount > 1",
    "IsLegacyTakeoverActive()",
    "GetDriverPawn()",
    "IsPlayerRecoveryChoiceEligible(TargetVehicle)",
    "Decision->CanEmergencyPatch(TargetVehicle)",
    "Runtime.PendingPersistentVehicleId = PersistentVehicleId",
    "Runtime.PendingPatchQuote = LockedQuote",
    "Runtime.PendingTowQuote = LockedQuote",
    "Runtime.StrandedSeconds = DispatchDuration - ClampedRemaining",
    "NATIVE_ROADSIDE_DISPATCH_RESTORED",
    "exact_id=YES charged=NO",
):
    require(restore_cpp, token, "restore implementation")

for forbidden in ("SpendCash(", "AddCash(", "ChargeFine("):
    assert forbidden not in restore_cpp, f"restore path must never mutate economy: {forbidden}"

for token in (
    "PlayerTowDispatchSeconds = 2.5f",
    "PlayerPatchDispatchSeconds = 3.0f",
):
    require(road_cpp, token, "production dispatch timing")

for token in (
    "UGTTRoadsideDispatchSaveGame",
    "SchemaVersion = 1",
    "bDispatchPending",
    "RecoveryMode",
    "PersistentVehicleId",
    "LockedQuote",
    "SecondsRemaining",
    "PrimaryWorldStateRevision",
):
    require(save_h, token, "transaction checkpoint schema")

for token in (
    "UGTTRoadsideDispatchPersistenceSubsystem",
    "LoadCheckpointOnce",
    "TryRestoreLoadedCheckpoint",
    "CaptureLiveCheckpoint",
    "ClearCheckpoint",
    "IsCargoVehicleCompatible",
):
    require(persist_h, token, "checkpoint subsystem API")

for token in (
    'RoadsideDispatchSlot(TEXT("GTT_RoadsideDispatch_01"))',
    'PrimaryWorldSlot(TEXT("GTT_Prototype_01"))',
    "CheckpointIntervalSeconds = 0.25f",
    "RemainingEtaWriteThresholdSeconds = 0.35f",
    "Save->SchemaVersion != 1",
    "NON_VOLUNTARY_MODE",
    "FARM_CARGO_ID_MISMATCH",
    "Roadside->RestorePendingRecoveryCheckpoint(",
    "Result == EGTTRoadsideRecoveryRestoreResult::WaitingForVehicle",
    "Roadside->HasPendingRoadsideService(Candidate)",
    "PendingCount > 1",
    "Roadside->GetPendingRecoveryMode(PendingVehicle)",
    "Roadside->GetPendingRecoveryVehicleId(PendingVehicle)",
    "Roadside->GetPendingRecoveryQuote(PendingVehicle)",
    "Roadside->GetPendingRecoverySecondsRemaining(PendingVehicle)",
    "UGameplayStatics::SaveGameToSlot(Save, RoadsideDispatchSlot, SaveUserIndex)",
    "UGameplayStatics::DeleteGameInSlot(RoadsideDispatchSlot, SaveUserIndex)",
    "Primary->FarmCargoBoundVehicleId == VehicleId",
    "NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_LOADED",
    "NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_REBOUND",
    "NATIVE_ROADSIDE_DISPATCH_CHECKPOINT_SAVED",
):
    require(persist_cpp, token, "checkpoint subsystem implementation")

for forbidden in ("SpendCash(", "AddCash(", "ChargeFine(", "PoliceImpound"):
    assert forbidden not in persist_cpp, f"checkpoint subsystem contains forbidden authority: {forbidden}"

for token in (
    "GetPendingRecoveryMode",
    "GetPendingRecoveryQuote",
    "GetPendingRecoverySecondsRemaining",
    "GetPendingRecoveryVehicleId",
):
    require(road_h, token, "authoritative production dispatch API")
    require(persist_cpp, f"Roadside->{token}", "checkpoint capture authority")

checks = re.findall(r"^-\s*\[(x| )\]\s+", roadmap, flags=re.MULTILINE | re.IGNORECASE)
done = sum(item.lower() == "x" for item in checks)
assert (done, len(checks)) == (125, 130), f"0.1.39 must not close runtime/art gates: {done}/{len(checks)}"
for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "../assets/readme/progress-mini.svg",
    "**125** | **5** | **130** | **96.2%**",
):
    require(roadmap, token, "ROADMAP")
assert roadmap.count("../assets/readme/progress-mini.svg") == 1, "ROADMAP must embed exactly one mini progress SVG"
assert not re.search(r"[█▓▒░]{4,}|\[[#=\-]{6,}\]", roadmap), "retired character progress meter returned"

for token in (
    "<!-- SWIR-README-STANDARD:v2 -->",
    "assets/readme/progress-card.svg",
    "## 🔎 Search Keywords",
    "Release readiness: **NOT READY**",
):
    require(readme, token, "README")
assert readme.count("assets/readme/progress-card.svg") == 1, "README must embed exactly one progress card"
assert "progress-mini.svg" not in readme, "README must not duplicate card + mini"

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
    "GTT 0.1.39",
    "persistent roadside dispatch",
    "locked quote",
    "remaining ETA",
    "exact",
    "Farm Cargo",
    "does not prove",
):
    assert token.lower() in changelog.lower(), f"0.1.39 changelog missing {token!r}"

scenarios = len(re.findall(r"^\d+\.\s+", playtest, flags=re.MULTILINE))
assert scenarios >= 64, f"PLAYTEST_0.1.39.md must contain at least 64 numbered scenarios, found {scenarios}"

print("GTT 0.1.39 persistent roadside dispatch checkpoint source contract: PASS")
print(f"Roadmap remains {done}/{len(checks)} = {done / len(checks) * 100:.1f}% until real Win64/Chaos/trailer/visual evidence exists")
print("NOTE: source verifier only; no UE compile/package/runtime claim.")
