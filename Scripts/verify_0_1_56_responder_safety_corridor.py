#!/usr/bin/env python3
"""Deterministic source-contract verifier for GTT 0.1.56 responder safety corridor."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATHS = {
    "safety_h": ROOT / "Source/GTT/Public/Traffic/GTTRoadsideSceneSafetySubsystem.h",
    "safety_cpp": ROOT / "Source/GTT/Private/Traffic/GTTRoadsideSceneSafetySubsystem.cpp",
    "vehicle_h": ROOT / "Source/GTT/Public/Traffic/GTTRoadsideResponderVehicle.h",
    "vehicle_cpp": ROOT / "Source/GTT/Private/Traffic/GTTRoadsideResponderVehicle.cpp",
    "traffic_h": ROOT / "Source/GTT/Public/Traffic/GTTTrafficCarPawn.h",
    "traffic_cpp": ROOT / "Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp",
    "playtest": ROOT / "Docs/PLAYTEST-0.1.56.md",
    "changelog": ROOT / "CHANGELOG.d/0.1.56-responder-safety-corridor.md",
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


safety_h = read("safety_h")
safety_cpp = read("safety_cpp")
vehicle_h = read("vehicle_h")
vehicle_cpp = read("vehicle_cpp")
traffic_h = read("traffic_h")
traffic_cpp = read("traffic_cpp")
playtest = read("playtest")
changelog = read("changelog")

require(
    safety_h,
    "safety subsystem header",
    "UGTTRoadsideSceneSafetySubsystem : public UTickableWorldSubsystem",
    "HasActiveSafetyCorridor",
    "GetLastYieldCount",
    "SafetyRadiusCm = 1800.0f",
    "InnerPassRadiusCm = 320.0f",
    "ReYieldCooldownSeconds = 4.5f",
    "YieldSeverity = 0.38f",
)
require(
    safety_cpp,
    "safety subsystem implementation",
    "TActorIterator<AGTTRoadsideResponderVehicle>",
    "IsParkedAtScene()",
    "IsSafetyCorridorDeployed()",
    "TActorIterator<AGTTTrafficCarPawn>",
    "IsIncidentDisabled()",
    "IsRoadsideAssistanceActive()",
    "IsRoadsideResponderSceneAuthority()",
    "IsYieldingForRangerStop()",
    "ReactToNearbyIncident(SceneLocation, YieldSeverity)",
    "LastYieldTimeByCar",
    "ROADSIDE_SAFETY_CORRIDOR_ACTIVE",
)
for forbidden in ("AddCash(", "SpendCash(", "RepairVehicle(", "SetWanted", "AddWanted", "ChargeFine("):
    if forbidden in safety_cpp:
        errors.append(f"safety corridor must not own economy/repair/Wanted authority: found {forbidden!r}")

require(
    vehicle_h,
    "responder safety presentation header",
    "IsSafetyCorridorDeployed()",
    "SetSafetyCorridorDeployed(bool bDeployed)",
    "SafetyConeFrontLeft",
    "SafetyConeFrontRight",
    "SafetyConeRearLeft",
    "SafetyConeRearRight",
)
require(
    vehicle_cpp,
    "responder safety presentation implementation",
    'TEXT("/Engine/BasicShapes/Cone.Cone")',
    "ResponderSafetyConeFrontLeft",
    "ResponderSafetyConeFrontRight",
    "ResponderSafetyConeRearLeft",
    "ResponderSafetyConeRearRight",
    "SetSafetyCorridorDeployed(bStartAtScene)",
    "SetSafetyCorridorDeployed(true)",
    "ROAD SERVICE - SAFE CORRIDOR",
)
if "MarkOwnedByPlayer" in vehicle_cpp or "Super::Interact_Implementation" in vehicle_cpp:
    errors.append("road-service responder must remain service-owned and non-enterable")

require(
    traffic_h,
    "existing traffic authority API",
    "ReactToNearbyIncident",
    "IsIncidentDisabled()",
    "IsRoadsideAssistanceActive()",
    "IsYieldingForRangerStop()",
)
require(
    traffic_cpp,
    "existing traffic response remains authoritative",
    "void AGTTTrafficCarPawn::ReactToNearbyIncident",
    "IncidentStopRemaining",
    "IncidentSteerBias",
    "TrafficDriveForce",
)

case_count = len(re.findall(r"^\|\s*\d+\s*\|", playtest, flags=re.MULTILINE))
if case_count < 48:
    errors.append(f"playtest matrix too small: expected >=48 numbered cases, found {case_count}")

for phrase in (
    "source-contract verification",
    "not a packaged win64 runtime proof",
    "cooldown-limited",
    "warden priority",
    "no economy authority",
    "roadmap completion is unchanged",
):
    if phrase not in playtest.lower():
        errors.append(f"playtest missing verification-boundary phrase: {phrase!r}")

require(
    changelog,
    "changelog fragment",
    "0.1.56",
    "Safety Corridor",
    "Cone.Cone",
    "cooldown",
    "warden",
    "source-contract",
    "Win64",
)

if errors:
    print("GTT 0.1.56 responder safety corridor sanity: FAILED")
    for error in errors:
        print(f" - {error}")
    sys.exit(1)

print(f"GTT 0.1.56 responder safety corridor sanity: PASS ({case_count} playtest cases)")
print(" - on-scene responder deploys four visible runtime-built safety cones")
print(" - nearby ambient traffic receives bounded cooldown-limited incident yielding")
print(" - ranger/player/incident authority remains outside the safety subsystem")
print(" - 0.1.55 responder ownership and 0.1.53 payout authority remain unchanged")
print(" - packaged Win64 runtime proof: NOT CLAIMED")
