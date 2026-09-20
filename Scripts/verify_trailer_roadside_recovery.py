#!/usr/bin/env python3
"""Source-level regression contract for GTT 0.1.58 trailer roadside recovery.

This intentionally does not claim Unreal/Win64 runtime evidence. It verifies that the
player-facing trailer service, suspension and Heavy Haul authority wiring stay coherent.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
TRAILER_H = (ROOT / "Source/GTT/Public/Vehicles/GTTFarmTrailer.h").read_text(encoding="utf-8")
TRAILER_CPP = (ROOT / "Source/GTT/Private/Vehicles/GTTFarmTrailer.cpp").read_text(encoding="utf-8")
HAUL_H = (ROOT / "Source/GTT/Public/Activities/GTTHeavyHaulDirector.h").read_text(encoding="utf-8")
HAUL_CPP = (ROOT / "Source/GTT/Private/Activities/GTTHeavyHaulDirector.cpp").read_text(encoding="utf-8")
ROADMAP = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
README = (ROOT / "README.md").read_text(encoding="utf-8")
PLAYTEST = (ROOT / "Docs/PLAYTEST-0.1.58-TRAILER-ROADSIDE-RECOVERY.md").read_text(encoding="utf-8")
CHANGELOG = (ROOT / "CHANGELOG.d/0.1.58.md").read_text(encoding="utf-8")


def require(text: str, token: str, label: str) -> None:
    if token not in text:
        raise AssertionError(f"missing {label}: {token}")


def function_body(source: str, signature: str) -> str:
    start = source.find(signature)
    if start < 0:
        raise AssertionError(f"missing function: {signature}")
    brace = source.find("{", start)
    if brace < 0:
        raise AssertionError(f"missing body: {signature}")
    depth = 0
    for i in range(brace, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[brace + 1:i]
    raise AssertionError(f"unterminated body: {signature}")


# Existing interaction path now reaches the physical trailer.
for token in (
    "public AActor, public IGTTInteractable",
    "Interact_Implementation(AActor* Interactor)",
    "GetInteractionText_Implementation() const",
    "TryBeginRoadsideRepair(APawn* RepairPawn)",
    "GetLockedRoadsideRepairQuote() const",
):
    require(TRAILER_H, token, "trailer interaction/recovery API")

for token in (
    "TRAILER FIELD REPAIR STARTED",
    "TRAILER FIELD REPAIR COMPLETE",
    "TRAILER_ROADSIDE_RECOVERY event=START",
    "TRAILER_ROADSIDE_RECOVERY event=COMPLETE",
    "LockedRoadsideRepairQuote = GetRoadsideRepairQuote();",
    "LockedRoadsideRepairDuration = GetRoadsideRepairDuration();",
    "RoadsideRepairTimeRemaining = FMath::Max(0.0f, RoadsideRepairTimeRemaining - DeltaSeconds);",
):
    require(TRAILER_CPP, token, "timed repair contract")

# Cancellation must remain no-charge for distance, movement, hitch tension, impact and manual cancel.
for token in (
    "RoadsideRepairMaxDistance",
    "RoadsideRepairMaxSpeedKmh",
    "RoadsideRepairMaxHitchLoad",
    "impact interrupted the repair; no charge",
    "TRAILER FIELD REPAIR CANCELLED: no charge",
):
    require(TRAILER_CPP + TRAILER_H, token, "safe cancellation path")

# Checkout is centralized on the trailer and rollback is explicit.
complete = function_body(TRAILER_CPP, "void AGTTFarmTrailer::CompleteRoadsideRepair()")
require(complete, "Economy->SpendCash(CompletedQuote, TEXT(\"Trailer roadside field repair\"))", "single authoritative checkout")
require(complete, "Economy->AddCash(CompletedQuote, TEXT(\"Trailer roadside field repair refund\"))", "checkout rollback")
require(complete, "++RoadsideRepairCount;", "successful repair escalation")
if complete.count("SpendCash(") != 1:
    raise AssertionError("trailer completion must contain exactly one cash-debit call")

# Production repair restores physical trailer state without repairing cargo.
repair = function_body(TRAILER_CPP, "bool AGTTFarmTrailer::PerformRoadsideRepair(float IntegrityRestore)")
for token in (
    "RestoreWheel(LeftWheel",
    "RestoreWheel(RightWheel",
    "ConfigureHitchConstraint();",
    "AttachToNativeFieldmaster(Native)",
    "AttachToVehicle(Legacy)",
):
    require(repair, token, "physical repair behavior")
if re.search(r"CargoIntegrity\s*=", repair):
    raise AssertionError("roadside repair must preserve damaged cargo integrity")

# 0.1.58 fixes the axle from Z-locked to bounded spring/damper travel and retunes loaded mass.
for token in (
    "SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, TrailerSuspensionTravelCm)",
    "SetLinearPositionDrive(false, false, true)",
    "SetLinearVelocityDrive(false, false, true)",
    "TrailerLoadedSuspensionSpring",
    "TrailerLoadedSuspensionDamping",
    "SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free",
    "SetLinearBreakable(true, WheelBreakForce)",
    "SetAngularBreakable(true, WheelBreakTorque)",
    "if (LeftWheelConstraint && LeftWheel && !bLeftWheelLost) ConfigureWheelAxle",
    "if (RightWheelConstraint && RightWheel && !bRightWheelLost) ConfigureWheelAxle",
):
    require(TRAILER_CPP, token, "bounded/load-aware trailer suspension")

# Heavy Haul no longer owns a second cash mutation path; it delegates start and observes completion.
for token in (
    "Trailer->TryBeginRoadsideRepair(PlayerPawn)",
    "Trailer->GetRoadsideRepairCount() > RoadsideRepairCount",
    "RoadsideRepairTimePenalty * CompletedRepairs",
    "FIELD REPAIR",
):
    require(HAUL_CPP, token, "Heavy Haul repair integration")
try_repair = function_body(HAUL_CPP, "bool AGTTHeavyHaulDirector::TryRoadsideRepair(APawn* PlayerPawn)")
if "SpendCash(" in try_repair or "PerformRoadsideRepair(" in try_repair:
    raise AssertionError("Heavy Haul must delegate repair authority instead of charging/mutating independently")
require(HAUL_H, "RoadsideRepairTimePenalty = 22.0f", "existing Heavy Haul time consequence")

# Documentation must describe the same milestone and remain honest about release gates.
for token in ("40-case", "125/130 (96.2%)", "Win64"):
    require(CHANGELOG, token, "0.1.58 changelog")
for token in ("## Acceptance matrix", "Contract time consequence", "Release honesty"):
    require(PLAYTEST, token, "0.1.58 playtest")
require(README, "<!-- SWIR-README-STANDARD:v2 -->", "README standard marker")
require(ROADMAP, "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "roadmap standard marker")
require(ROADMAP, "../assets/readme/progress-mini.svg", "roadmap progress SVG")

checked = len(re.findall(r"^\s*- \[x\]", ROADMAP, flags=re.MULTILINE | re.IGNORECASE))
uncheck = len(re.findall(r"^\s*- \[ \]", ROADMAP, flags=re.MULTILINE))
total = checked + uncheck
if (checked, uncheck, total) != (125, 5, 130):
    raise AssertionError(f"roadmap truth changed unexpectedly: checked={checked}, unchecked={uncheck}, total={total}")
if "96.2%" not in ROADMAP:
    raise AssertionError("roadmap must retain verified 96.2% progress")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", ROADMAP, flags=re.MULTILINE):
    raise AssertionError("legacy text/Unicode roadmap progress meter must not return")

print("PASS: GTT 0.1.58 trailer roadside recovery + suspension source contract")
print(f"roadmap: {checked}/{total} = {checked / total * 100:.1f}%")
