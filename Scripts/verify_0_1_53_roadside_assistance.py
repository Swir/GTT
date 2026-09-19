#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "Source/GTT/Public/Traffic/GTTTrafficCarPawn.h"
CPP = ROOT / "Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp"
PLAYTEST = ROOT / "Docs/PLAYTEST-0.1.53.md"

header = HEADER.read_text(encoding="utf-8")
cpp = CPP.read_text(encoding="utf-8")
playtest = PLAYTEST.read_text(encoding="utf-8")

errors = []

def require(text, token, label):
    if token not in text:
        errors.append(f"missing {label}: {token}")

def require_order(text, tokens, label):
    positions = [text.find(token) for token in tokens]
    if any(pos < 0 for pos in positions) or positions != sorted(positions):
        errors.append(f"invalid order for {label}: {tokens}")

# Player-facing contract.
for token in (
    "BeginRoadsideAssistance",
    "RoadsideAssistanceDurationSeconds = 6.0f",
    "RoadsideAssistanceMaxDistance = 500.0f",
    "RoadsideRepairFraction = 0.45f",
    "RoadsideBasePayout = 65",
    "RoadsideSeverityBonus = 45",
    "bRoadsideAssistanceCompletedForIncident",
):
    require(header, token, "roadside header contract")

# Real traffic damage must automatically become an incident instead of relying on a disconnected helper call.
require_order(cpp, [
    "const float ConditionDrop = LastObservedConditionPercent - CurrentConditionPercent;",
    "if (ConditionDrop >= 0.04f)",
    "RegisterCollisionIncident(EstimatedImpactSpeedKmh",
], "damage-to-incident wiring")

# Nearby traffic must react to a real incident, but ReactToNearbyIncident must not recurse into registration.
require(cpp, "TActorIterator<AGTTTrafficCarPawn>", "nearby traffic fan-out")
require(cpp, "OtherTraffic->ReactToNearbyIncident", "nearby traffic response")
react_body = cpp.split("void AGTTTrafficCarPawn::ReactToNearbyIncident", 1)[1].split("bool AGTTTrafficCarPawn::BeginRoadsideAssistance", 1)[0]
if "RegisterCollisionIncident(" in react_body:
    errors.append("nearby incident response recursively registers a new collision incident")

# Assistance is distance/time gated and cannot pre-charge the player.
for token in (
    "RoadsideAssistanceRemaining = RoadsideAssistanceDurationSeconds;",
    "FVector::DistSquared2D(Helper->GetActorLocation(), GetActorLocation()) > FMath::Square(RoadsideAssistanceMaxDistance)",
    "RoadsideAssistanceRemaining = FMath::Max(0.0f, RoadsideAssistanceRemaining - DeltaSeconds);",
    "CompleteRoadsideAssistance();",
):
    require(cpp, token, "timed roadside assist")
if "SpendCash(" in cpp:
    errors.append("roadside assistance must not spend player cash")

# Payout happens only in completion path, after the field repair succeeds.
complete = cpp.split("void AGTTTrafficCarPawn::CompleteRoadsideAssistance", 1)[1].split("void AGTTTrafficCarPawn::Tick", 1)[0]
require_order(complete, [
    "RepairVehicle(MaxCondition * RoadsideRepairFraction);",
    "if (bIncidentDisabled)",
    "Economy->AddCash(Payout, TEXT(\"Roadside traffic assistance\"));",
    "bRoadsideAssistanceCompletedForIncident = true;",
], "repair-before-payout")
if complete.count("AddCash(") != 1:
    errors.append("completion path must contain exactly one cash payout")

# A newly disabled incident resets one-time payout authority; successful assist marks it consumed.
require_order(cpp, [
    "if (bIncidentDisabled && !bWasDisabled)",
    "bRoadsideAssistanceCompletedForIncident = false;",
], "new-incident payout reset")

# Ambient traffic remains non-stealable through this interaction path.
interact = cpp.split("void AGTTTrafficCarPawn::Interact_Implementation", 1)[1].split("FText AGTTTrafficCarPawn::GetInteractionText_Implementation", 1)[0]
if "Super::Interact_Implementation" in interact:
    errors.append("traffic interaction must not call the drivable vehicle theft/entry path")
require(interact, "BeginRoadsideAssistance(Interactor)", "disabled traffic interaction")

# Assistance must keep traffic in the critical simulation budget and physically stopped while work is active.
require(cpp, "bIncidentDisabled || bRoadsideAssistanceActive || bYieldingForRangerStop", "critical tick budget")
require(cpp, "bIncidentDisabled || IncidentStopRemaining > 0.0f || bRoadsideAssistanceActive", "assist stop authority")

# Documentation should describe all player-visible gates without claiming packaged runtime proof.
for token in (
    "6 seconds",
    "500 cm",
    "$65",
    "$110",
    "single payout",
    "source-contract verification",
    "not a packaged Win64 runtime proof",
):
    require(playtest, token, "0.1.53 playtest evidence")

# Keep the matrix meaningful rather than a token smoke document.
case_count = len(re.findall(r"^\|\s*\d+\s*\|", playtest, flags=re.MULTILINE))
if case_count < 36:
    errors.append(f"playtest matrix too small: {case_count} cases (need >= 36)")

if errors:
    print("GTT 0.1.53 roadside assistance verification FAILED")
    for error in errors:
        print(f" - {error}")
    raise SystemExit(1)

print(f"GTT 0.1.53 roadside assistance verification PASS ({case_count} documented playtest cases)")
