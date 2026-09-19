#!/usr/bin/env python3
"""Deterministic source-contract verifier for GTT 0.1.57 scene clearance."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATHS = {
    "responder_h": ROOT / "Source/GTT/Public/Traffic/GTTCivilianIncidentResponderSubsystem.h",
    "responder_cpp": ROOT / "Source/GTT/Private/Traffic/GTTCivilianIncidentResponderSubsystem.cpp",
    "vehicle_h": ROOT / "Source/GTT/Public/Traffic/GTTRoadsideResponderVehicle.h",
    "vehicle_cpp": ROOT / "Source/GTT/Private/Traffic/GTTRoadsideResponderVehicle.cpp",
    "safety_h": ROOT / "Source/GTT/Public/Traffic/GTTRoadsideSceneSafetySubsystem.h",
    "safety_cpp": ROOT / "Source/GTT/Private/Traffic/GTTRoadsideSceneSafetySubsystem.cpp",
    "save_h": ROOT / "Source/GTT/Public/Save/GTTCivilianResponderSaveGame.h",
    "playtest": ROOT / "Docs/PLAYTEST-0.1.57.md",
    "changelog": ROOT / "CHANGELOG.d/0.1.57-scene-clearance-continuity.md",
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

responder_h = read("responder_h")
responder_cpp = read("responder_cpp")
vehicle_h = read("vehicle_h")
vehicle_cpp = read("vehicle_cpp")
safety_h = read("safety_h")
safety_cpp = read("safety_cpp")
save_h = read("save_h")
playtest = read("playtest")
changelog = read("changelog")

require(
    responder_h,
    "responder header",
    "ClearingScene",
    "BeginSceneClearance",
    "AdvanceSceneClearance",
    "RestoreClearingResponder",
    "SceneClearanceRemaining",
    "ResponderSceneClearanceSeconds = 9.0f",
)
require(
    responder_cpp,
    "responder implementation",
    "Phase == EGTTCivilianResponderPhase::ClearingScene",
    "BeginSceneClearance(Vehicle);",
    "CIVILIAN_RESPONDER_CLEARANCE_BEGIN",
    "CIVILIAN_RESPONDER_CLEARANCE_COMPLETE",
    "CIVILIAN_RESPONDER_CLEARANCE_RESTORED",
    "Save->SchemaVersion = 2",
    "Save->SceneClearanceRemaining",
    "Save->SceneLocation",
    "Save->SchemaVersion != 1 && Save->SchemaVersion != 2",
)
complete_block = responder_cpp.split("if (Vehicle->CompleteRoadsideResponderRecovery())", 1)
if len(complete_block) != 2:
    errors.append("recovery completion block missing")
else:
    tail = complete_block[1].split("SceneHoldRemaining = 2.0f;", 1)[0]
    require_order(tail, "recovery to clearance order", "CIVILIAN_RESPONDER_HANDOFF_COMPLETE", "BeginSceneClearance(Vehicle);")
    if "DestroyResponderVehicle();" in tail:
        errors.append("recovery must not destroy responder before ClearingScene")

require(
    save_h,
    "schema-2 responder save",
    "SchemaVersion = 2",
    "SceneClearanceRemaining",
    "FVector SceneLocation",
)
require(
    vehicle_h,
    "responder presentation header",
    "BeginSceneClearance",
    "IsSceneClearing()",
    "bSceneClearing",
)
require(
    vehicle_cpp,
    "responder presentation implementation",
    "ROAD SERVICE - LANE REOPENING",
    "bSafetyCorridorDeployed && !bSceneClearing",
    "SafetyConeRearLeft->SetVisibility(bSafetyCorridorDeployed",
    "SafetyConeRearRight->SetVisibility(bSafetyCorridorDeployed",
    "County road service - lane reopening",
)
require(
    safety_h,
    "safety reopening constants",
    "ReopeningRadiusCm = 980.0f",
    "ReopeningYieldSeverity = 0.20f",
    "SafetyRadiusCm = 1800.0f",
    "InnerPassRadiusCm = 320.0f",
    "ReYieldCooldownSeconds = 4.5f",
)
require(
    safety_cpp,
    "safety reopening implementation",
    "Responder->IsSceneClearing()",
    "bLaneReopening ? ReopeningRadiusCm : SafetyRadiusCm",
    "bLaneReopening ? ReopeningYieldSeverity : YieldSeverity",
    "ReactToNearbyIncident(SceneLocation, EffectiveYieldSeverity)",
    'bLaneReopening ? TEXT("REOPENING") : TEXT("ACTIVE")',
)
for forbidden in ("AddCash(", "SpendCash(", "RepairVehicle(", "SetWanted", "AddWanted", "ChargeFine("):
    if forbidden in safety_cpp:
        errors.append(f"clearance safety layer must not own economy/repair/Wanted authority: found {forbidden!r}")

case_count = len(re.findall(r"^\|\s*\d+\s*\|", playtest, flags=re.MULTILINE))
if case_count < 64:
    errors.append(f"playtest matrix too small: expected >=64 numbered cases, found {case_count}")

for phrase in (
    "source-contract verification",
    "not a packaged win64 runtime proof",
    "no economy authority",
    "roadmap completion is unchanged",
    "clearingscene",
    "lane reopening",
):
    if phrase not in playtest.lower():
        errors.append(f"playtest missing verification phrase: {phrase!r}")

require(
    changelog,
    "changelog fragment",
    "0.1.57",
    "ClearingScene",
    "nine-second",
    "980 cm",
    "schema 2",
    "Schema-1",
    "no cash",
    "Win64",
)

if errors:
    print("GTT 0.1.57 scene clearance sanity: FAILED")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print(f"GTT 0.1.57 scene clearance sanity: PASS ({case_count} playtest cases)")
print(" - successful county recovery transitions into a bounded ClearingScene phase")
print(" - responder stays visible while lane-reopening cones and corridor taper")
print(" - schema-2 persistence restores post-recovery clearance without replaying repair/payout")
print(" - schema-1 responder saves remain accepted for legacy phases")
print(" - economy/Wanted/ranger/dispatch authority remains outside clearance")
print(" - packaged Win64 runtime proof: NOT CLAIMED")
