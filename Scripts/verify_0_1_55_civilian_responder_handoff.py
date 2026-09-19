#!/usr/bin/env python3
"""Deterministic source-contract verifier for GTT 0.1.55 responder handoff."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATHS = {
    "sub_h": ROOT / "Source/GTT/Public/Traffic/GTTCivilianIncidentResponderSubsystem.h",
    "sub_cpp": ROOT / "Source/GTT/Private/Traffic/GTTCivilianIncidentResponderSubsystem.cpp",
    "vehicle_h": ROOT / "Source/GTT/Public/Traffic/GTTRoadsideResponderVehicle.h",
    "vehicle_cpp": ROOT / "Source/GTT/Private/Traffic/GTTRoadsideResponderVehicle.cpp",
    "traffic_h": ROOT / "Source/GTT/Public/Traffic/GTTTrafficCarPawn.h",
    "traffic_cpp": ROOT / "Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp",
    "save_h": ROOT / "Source/GTT/Public/Save/GTTCivilianResponderSaveGame.h",
    "dispatch_h": ROOT / "Source/GTT/Public/Traffic/GTTCivilianIncidentDispatchSubsystem.h",
    "dispatch_cpp": ROOT / "Source/GTT/Private/Traffic/GTTCivilianIncidentDispatchSubsystem.cpp",
    "playtest": ROOT / "Docs/PLAYTEST-0.1.55.md",
    "changelog": ROOT / "CHANGELOG.d/0.1.55-civilian-responder-handoff.md",
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

def function_body(text: str, signature: str, next_signatures: tuple[str, ...]) -> str:
    start = text.find(signature)
    if start < 0:
        errors.append(f"missing function signature {signature!r}")
        return ""
    ends = [text.find(sig, start + len(signature)) for sig in next_signatures]
    ends = [pos for pos in ends if pos > start]
    end = min(ends) if ends else len(text)
    return text[start:end]

sub_h = read("sub_h")
sub_cpp = read("sub_cpp")
vehicle_h = read("vehicle_h")
vehicle_cpp = read("vehicle_cpp")
traffic_h = read("traffic_h")
traffic_cpp = read("traffic_cpp")
save_h = read("save_h")
dispatch_h = read("dispatch_h")
dispatch_cpp = read("dispatch_cpp")
playtest = read("playtest")
changelog = read("changelog")

require(sub_h, "responder subsystem header", "UGTTCivilianIncidentResponderSubsystem : public UTickableWorldSubsystem", "EGTTCivilianResponderPhase", "AuthorityVehicle", "ClearResponderSceneAuthority", "SevereIncidentThreshold = 0.72f", "PlayerAssistGraceSeconds = 18.0f", "ResponderSceneHoldSeconds = 7.0f", "ResponderSpawnDistanceCm = 1900.0f")
require(save_h, "responder sidecar", "UGTTCivilianResponderSaveGame : public USaveGame", "SchemaVersion = 2", "IncidentId", "Phase", "PlayerGraceElapsed", "SceneHoldRemaining")
if "TObjectPtr" in save_h or "TWeakObjectPtr" in save_h or "AActor*" in save_h:
    errors.append("responder sidecar must not persist actor/object pointers")
require(sub_cpp, "schema compatibility", "Save->SchemaVersion != 1 && Save->SchemaVersion != 2", "Save->SchemaVersion >= 2", "EGTTCivilianResponderPhase::OnScene")
require(vehicle_h, "physical responder header", "AGTTRoadsideResponderVehicle : public AGTTVehicleBase", "InitializeIncidentResponse", "IsParkedAtScene", "GetAssignedIncidentId")
require(vehicle_cpp, "physical responder implementation", 'TEXT("/Engine/BasicShapes/Cube.Cube")', 'TEXT("/Engine/BasicShapes/Sphere.Sphere")', "VehicleMesh->AddForce", "VehicleMesh->AddTorqueInRadians", '"ROAD SERVICE"', "bIllegalToTake = true")
if "MarkOwnedByPlayer" in vehicle_cpp or "Super::Interact_Implementation" in vehicle_cpp:
    errors.append("responder vehicle must remain service-owned and non-enterable")
require(dispatch_h, "0.1.54 authoritative dispatch surface", "GetActiveIncidentId()", "GetActiveIncidentSeverity()", "IsTrackedVehicle", "GetPresentationSnapshot")
require(dispatch_cpp, "0.1.54 dispatch authority regression", 'TEXT("GTT_CivilianIncident_01")', "ROADSIDE SOS", "ResolveDispatch")
require(sub_cpp, "responder subsystem implementation", 'TEXT("GTT_CivilianResponder_01")', "GetSubsystem<UGTTCivilianIncidentDispatchSubsystem>()", "Dispatch->GetActiveIncidentId()", "Dispatch->GetActiveIncidentSeverity()", "Dispatch->GetPresentationSnapshot(nullptr)", "Dispatch->IsTrackedVehicle(Candidate)", "DispatchPresentation.bWardenTrafficControl", "Vehicle->IsRoadsideAssistanceActive()", "SpawnActor<AGTTRoadsideResponderVehicle>", "AuthorityVehicle = Vehicle", "ClearResponderSceneAuthority();", "Vehicle->SetRoadsideResponderSceneAuthority(true)", "Vehicle->CompleteRoadsideResponderRecovery()", "SaveGameToSlot", "LoadGameFromSlot", "DeleteGameInSlot")
for forbidden in ("AddCash(", "SpendCash(", "ChargeFine(", "SetWanted", "AddWanted"):
    if forbidden in sub_cpp:
        errors.append(f"responder subsystem must not own economy/Wanted authority: found {forbidden!r}")

clear_authority = function_body(sub_cpp, "void UGTTCivilianIncidentResponderSubsystem::ClearResponderSceneAuthority()", ("void UGTTCivilianIncidentResponderSubsystem::DestroyResponderVehicle",))
require(clear_authority, "stale responder authority cleanup", "AuthorityVehicle.Get()", "AuthorityVehicle.Reset()", "TActorIterator<AGTTTrafficCarPawn>", "Candidate->IsRoadsideResponderSceneAuthority()", "Candidate->SetRoadsideResponderSceneAuthority(false)")
require(traffic_h, "traffic responder API", "IsRoadsideResponderSceneAuthority()", "SetRoadsideResponderSceneAuthority(bool bActive)", "CompleteRoadsideResponderRecovery()", "ResponderRecoveryFraction = 0.38f", "ResponderPostRecoveryLimpSeconds = 18.0f")

begin_assist = function_body(traffic_cpp, "bool AGTTTrafficCarPawn::BeginRoadsideAssistance(AActor* Helper)", ("void AGTTTrafficCarPawn::CancelRoadsideAssistance",))
require(begin_assist, "player-first/scene-authority guard", "bRoadsideResponderSceneAuthority", "County road service has scene authority", "return false;")
if begin_assist.find("bRoadsideResponderSceneAuthority") > begin_assist.find("RoadsideHelper = Helper") >= 0:
    errors.append("scene authority guard must run before player assistance is committed")

player_complete = function_body(traffic_cpp, "void AGTTTrafficCarPawn::CompleteRoadsideAssistance()", ("void AGTTTrafficCarPawn::SetRoadsideResponderSceneAuthority",))
require(player_complete, "existing player payout authority", "RepairVehicle(MaxCondition * RoadsideRepairFraction);", 'Economy->AddCash(Payout, TEXT("Roadside traffic assistance"));', "bRoadsideAssistanceCompletedForIncident = true;")
if player_complete.find("RepairVehicle(MaxCondition * RoadsideRepairFraction);") > player_complete.find("Economy->AddCash"):
    errors.append("player roadside authority regression: payout must remain after repair")

responder_complete = function_body(traffic_cpp, "bool AGTTTrafficCarPawn::CompleteRoadsideResponderRecovery()", ("void AGTTTrafficCarPawn::Tick(",))
require(responder_complete, "no-payout responder recovery", "bRoadsideResponderSceneAuthority", "RepairVehicle(MaxCondition * ResponderRecoveryFraction);", "ResponderPostRecoveryLimpSeconds", "bRoadsideResponderSceneAuthority = false;", "payout=NONE")
for forbidden in ("AddCash(", "SpendCash(", "Payout ="):
    if forbidden in responder_complete:
        errors.append(f"responder recovery must not create a player payout: found {forbidden!r}")
require(traffic_cpp, "traffic lifecycle integration", "bRoadsideResponderSceneAuthority = false;", "bRoadsideResponderSceneAuthority || bYieldingForRangerStop", "County road service - recovery in progress")

case_count = len(re.findall(r"^\|\s*\d+\s*\|", playtest, flags=re.MULTILINE))
if case_count < 56:
    errors.append(f"playtest matrix too small: expected >=56 numbered cases, found {case_count}")
for phrase in ("source-contract verification", "not a packaged win64 runtime proof", "player-first grace", "no player reward", "warden traffic control", "roadmap completion is unchanged"):
    if phrase not in playtest.lower():
        errors.append(f"playtest missing verification-boundary phrase: {phrase!r}")
require(changelog, "changelog fragment", "0.1.55", "Responder Handoff", "GTT_CivilianResponder_01", "18-second", "ROAD SERVICE", "warden", "source-contract", "Win64")

if errors:
    print("GTT 0.1.55 civilian incident responder handoff sanity: FAILED")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print(f"GTT 0.1.55 civilian incident responder handoff sanity: PASS ({case_count} playtest cases)")
print(" - authoritative 0.1.54 dispatch remains the incident owner")
print(" - physical responder/grace/on-scene handoff: verified by source contract")
print(" - schema-2 responder sidecar keeps schema-1 compatibility for legacy phases")
print(" - player payout remains only in existing 0.1.53 assistance path")
print(" - responder persistence/ranger priority: verified by source contract")
print(" - packaged Win64 runtime proof: NOT CLAIMED")
