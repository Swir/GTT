#!/usr/bin/env python3
"""Deterministic source-contract verifier for GTT 0.1.54 civilian incident dispatch."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

PATHS = {
    "dispatch_h": ROOT / "Source/GTT/Public/Traffic/GTTCivilianIncidentDispatchSubsystem.h",
    "dispatch_cpp": ROOT / "Source/GTT/Private/Traffic/GTTCivilianIncidentDispatchSubsystem.cpp",
    "traffic_h": ROOT / "Source/GTT/Public/Traffic/GTTTrafficCarPawn.h",
    "traffic_cpp": ROOT / "Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp",
    "save_h": ROOT / "Source/GTT/Public/Save/GTTCivilianIncidentSaveGame.h",
    "playtest": ROOT / "Docs/PLAYTEST-0.1.54.md",
    "changelog": ROOT / "CHANGELOG.d/0.1.54-civilian-incident-dispatch.md",
}

errors: list[str] = []


def read(name: str) -> str:
    path = PATHS[name]
    if not path.is_file():
        errors.append(f"missing required file: {path.relative_to(ROOT)}")
        return ""
    return path.read_text(encoding="utf-8")


def require(text: str, label: str, *tokens: str) -> None:
    for token in tokens:
        if token not in text:
            errors.append(f"{label}: missing token {token!r}")


def require_order(text: str, label: str, *tokens: str) -> None:
    cursor = -1
    for token in tokens:
        pos = text.find(token, cursor + 1)
        if pos < 0:
            errors.append(f"{label}: missing ordered token {token!r}")
            return
        if pos <= cursor:
            errors.append(f"{label}: token order broken at {token!r}")
            return
        cursor = pos


dispatch_h = read("dispatch_h")
dispatch_cpp = read("dispatch_cpp")
traffic_h = read("traffic_h")
traffic_cpp = read("traffic_cpp")
save_h = read("save_h")
playtest = read("playtest")
changelog = read("changelog")

require(
    dispatch_h,
    "dispatch header",
    "UGTTCivilianIncidentDispatchSubsystem : public UTickableWorldSubsystem",
    "EGTTCivilianIncidentDispatchState",
    "GetPresentationSnapshot",
    "OrphanExpirySeconds = 180.0f",
    "RebindRadiusCm = 950.0f",
    "RangerAuthorityRadiusCm = 1400.0f",
    "SupersedeSeverityDelta = 0.20f",
)

require(
    save_h,
    "dispatch save sidecar",
    "UGTTCivilianIncidentSaveGame : public USaveGame",
    "SchemaVersion = 1",
    "IncidentId",
    "IncidentLocation",
    "Severity",
    "bAssistanceWasInProgress",
)

require(
    traffic_h,
    "traffic public observation/control surface",
    "GetLastIncidentSeverity()",
    "IsIncidentDisabled()",
    "IsRoadsideAssistanceActive()",
    "WasRoadsideAssistanceCompletedForIncident()",
    "CancelRoadsideAssistanceForTrafficControl()",
    'CancelRoadsideAssistance(TEXT("ranger-traffic-control-priority"))',
)

require(
    dispatch_cpp,
    "dispatch implementation",
    'TEXT("GTT_CivilianIncident_01")',
    "TActorIterator<AGTTTrafficCarPawn>",
    "Candidate->IsIncidentDisabled()",
    "Candidate->WasRoadsideAssistanceCompletedForIncident()",
    "Candidate->GetLastIncidentSeverity()",
    "FGuid::NewGuid()",
    "ROADSIDE DISPATCH",
    "ROADSIDE SOS",
    "UTextRenderComponent",
    "SaveGameToSlot",
    "LoadGameFromSlot",
    "DeleteGameInSlot",
    "TryRebindSavedIncident",
    "RebindRadiusCm",
    "OrphanExpirySeconds",
    "HasTrafficControl()",
    "GetPullOverTargetLocation()",
    "RangerAuthorityRadiusCm",
    "CancelRoadsideAssistanceForTrafficControl()",
    "Economy->PushMessage",
)

if "Economy->AddCash" in dispatch_cpp or "Economy->SpendCash" in dispatch_cpp or "ChargeFine" in dispatch_cpp:
    errors.append("dispatch implementation must not own cash, spend, payout, or fine authority")

require(
    traffic_cpp,
    "0.1.53 roadside authority regression",
    "void AGTTTrafficCarPawn::CompleteRoadsideAssistance()",
    "RepairVehicle(MaxCondition * RoadsideRepairFraction);",
    'Economy->AddCash(Payout, TEXT("Roadside traffic assistance"));',
    "bRoadsideAssistanceCompletedForIncident = true;",
)
complete_start = traffic_cpp.find("void AGTTTrafficCarPawn::CompleteRoadsideAssistance()")
tick_start = traffic_cpp.find("void AGTTTrafficCarPawn::Tick(", complete_start)
complete_body = traffic_cpp[complete_start:tick_start if tick_start > complete_start else None]
require_order(
    complete_body,
    "repair before payout",
    "RepairVehicle(MaxCondition * RoadsideRepairFraction);",
    'Economy->AddCash(Payout, TEXT("Roadside traffic assistance"));',
    "bRoadsideAssistanceCompletedForIncident = true;",
)

interact_start = traffic_cpp.find("void AGTTTrafficCarPawn::Interact_Implementation")
interaction_body = traffic_cpp[interact_start:] if interact_start >= 0 else ""
if "Super::Interact_Implementation" in interaction_body:
    errors.append("traffic interaction regression: ambient civilian car would regain base entry/theft path")

load_start = dispatch_cpp.find("void UGTTCivilianIncidentDispatchSubsystem::LoadCheckpoint()")
clear_start = dispatch_cpp.find("void UGTTCivilianIncidentDispatchSubsystem::ClearCheckpoint()", load_start)
load_body = dispatch_cpp[load_start:clear_start if clear_start > load_start else None]
require(
    load_body,
    "load safety",
    "State = EGTTCivilianIncidentDispatchState::Active;",
    "TrackedVehicle.Reset();",
    "bRestoredFromCheckpoint = true;",
)
if "AssistanceInProgress" in load_body:
    errors.append("load safety: helper work state must not be restored as AssistanceInProgress")

require_order(
    dispatch_cpp,
    "warden priority lifecycle",
    "if (Vehicle->IsRoadsideAssistanceActive() && IsWardenTrafficControlBlocking(ActiveLocation))",
    "Vehicle->CancelRoadsideAssistanceForTrafficControl()",
    "State = EGTTCivilianIncidentDispatchState::Active;",
    "SaveCheckpoint();",
)

case_count = len(re.findall(r"^\|\s*\d+\s*\|", playtest, flags=re.MULTILINE))
if case_count < 48:
    errors.append(f"playtest matrix too small: expected >=48 numbered cases, found {case_count}")

lower_playtest = playtest.lower()
for phrase in (
    "source-contract verification",
    "not a packaged win64 runtime proof",
    "does not persist an actor pointer",
    "helper identity",
    "roadmap completion is unchanged",
):
    if phrase not in lower_playtest:
        errors.append(f"playtest missing verification-boundary phrase: {phrase!r}")

require(
    changelog,
    "changelog fragment",
    "0.1.54",
    "Civilian Incident Dispatch",
    "GTT_CivilianIncident_01",
    "ROADSIDE SOS",
    "warden",
    "source-contract",
    "Win64",
)

if errors:
    print("GTT 0.1.54 civilian incident dispatch sanity: FAILED")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print(f"GTT 0.1.54 civilian incident dispatch sanity: PASS ({case_count} playtest cases)")
print(" - dispatch lifecycle/persistence: verified by source contract")
print(" - ranger traffic-control priority: verified by source contract")
print(" - economy ownership: existing 0.1.53 repair/payout path preserved")
print(" - packaged Win64 runtime proof: NOT CLAIMED")
