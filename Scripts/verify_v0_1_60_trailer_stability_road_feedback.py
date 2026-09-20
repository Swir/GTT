#!/usr/bin/env python3
"""Source + deterministic math verifier for GTT 0.1.60 trailer stability and road feedback."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HDR = ROOT / "Source/GTT/Public/Vehicles/GTTTrailerRoadFeedbackSubsystem.h"
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTTrailerRoadFeedbackSubsystem.cpp"
TRAILER_HDR = ROOT / "Source/GTT/Public/Vehicles/GTTFarmTrailer.h"

hdr = HDR.read_text(encoding="utf-8")
cpp = CPP.read_text(encoding="utf-8")
trailer_hdr = TRAILER_HDR.read_text(encoding="utf-8")

required_header = [
    "UGTTTrailerRoadFeedbackSubsystem",
    "StabilityStartSpeedKmh = 25.0f",
    "StabilityFullSpeedKmh = 70.0f",
    "MaximumStabilityAuthority = 0.45f",
    "MaximumLateralStabilityForce = 650000.0f",
    "MaximumYawStabilityTorque = 5000000.0f",
    "BrakeDecelerationThresholdKmhPerSecond = 6.0f",
    "ReverseLightThresholdKmh = -2.0f",
    "CriticalIntegrityThreshold = 0.45f",
]
required_cpp = [
    "TActorIterator<AGTTFarmTrailer>",
    "RoadFeedback_TailLeft",
    "RoadFeedback_TailRight",
    "RoadFeedback_ReverseLeft",
    "RoadFeedback_ReverseRight",
    "RoadFeedback_HazardLeft",
    "RoadFeedback_HazardRight",
    "Trailer->IsRoadsideRepairPending()",
    "Trailer->GetLostWheelCount() > 0",
    "Trailer->GetHitchIntegrity() < CriticalIntegrityThreshold",
    "Trailer->GetTrailerIntegrity() < CriticalIntegrityThreshold",
    "DecelerationKmhPerSecond >= BrakeDecelerationThresholdKmhPerSecond",
    "LongitudinalSpeedKmh <= ReverseLightThresholdKmh",
    "!Trailer->IsAttached() || !Trailer->HasCargo() || Trailer->GetLostWheelCount() > 0",
    "Trailer->GetTowLoadFactor()",
    "Body->AddForce(Right * LateralForceMagnitude)",
    "Body->AddTorqueInRadians",
    "MaximumStabilityAuthority",
    "MaximumLateralStabilityForce",
    "MaximumYawStabilityTorque",
]
required_trailer_api = [
    "bool IsAttached() const",
    "bool HasCargo() const",
    "bool IsRoadsideRepairPending() const",
    "float GetTrailerIntegrity() const",
    "float GetHitchIntegrity() const",
    "int32 GetLostWheelCount() const",
    "float GetTowLoadFactor() const",
]

missing = [token for token in required_header if token not in hdr]
missing += [token for token in required_cpp if token not in cpp]
missing += [token for token in required_trailer_api if token not in trailer_hdr]
if missing:
    raise SystemExit("0.1.60 source contract missing: " + ", ".join(missing))


def clamp(value: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, value))


def stability_authority(speed_kmh: float, load: float, trailer_integrity: float, hitch_integrity: float, lost_wheels: int = 0) -> float:
    if lost_wheels > 0:
        return 0.0
    speed = clamp((speed_kmh - 25.0) / (70.0 - 25.0), 0.0, 1.0)
    load = clamp(load, 0.0, 1.0)
    integrity = min(trailer_integrity, hitch_integrity)
    health = clamp((integrity - 0.20) / 0.80, 0.15, 1.0)
    return speed * load * health * 0.45


def light_state(attached: bool, speed_kmh: float, decel_kmh_s: float, integrity: float = 1.0, hitch: float = 1.0, lost_wheels: int = 0, repair=False):
    moving = abs(speed_kmh) > 3.0
    brake = attached and moving and decel_kmh_s >= 6.0
    reverse = attached and speed_kmh <= -2.0
    hazard = lost_wheels > 0 or hitch < 0.45 or integrity < 0.45 or repair
    return brake, reverse, hazard

stability_cases = [
    ("below-start", stability_authority(24.9, 1.0, 1.0, 1.0), 0.0),
    ("at-start", stability_authority(25.0, 1.0, 1.0, 1.0), 0.0),
    ("mid-speed-full-load", stability_authority(47.5, 1.0, 1.0, 1.0), 0.225),
    ("full-speed-full-load", stability_authority(70.0, 1.0, 1.0, 1.0), 0.45),
    ("half-load", stability_authority(70.0, 0.5, 1.0, 1.0), 0.225),
    ("damaged-hitch-reduces-assist", stability_authority(70.0, 1.0, 1.0, 0.60), 0.225),
    ("lost-wheel-disables", stability_authority(70.0, 1.0, 1.0, 1.0, 1), 0.0),
]
for name, actual, expected in stability_cases:
    if abs(actual - expected) > 1e-6:
        raise SystemExit(f"0.1.60 stability math failed: {name}: {actual:.6f} != {expected:.6f}")

light_cases = [
    ("cruise", light_state(True, 35.0, 0.0), (False, False, False)),
    ("braking", light_state(True, 35.0, 6.0), (True, False, False)),
    ("reverse", light_state(True, -3.0, 0.0), (False, True, False)),
    ("detached-no-brake-reverse", light_state(False, -20.0, 20.0), (False, False, False)),
    ("hitch-critical", light_state(True, 0.0, 0.0, hitch=0.44), (False, False, True)),
    ("trailer-critical", light_state(True, 0.0, 0.0, integrity=0.44), (False, False, True)),
    ("lost-wheel-hazard", light_state(True, 0.0, 0.0, lost_wheels=1), (False, False, True)),
    ("repair-hazard", light_state(True, 0.0, 0.0, repair=True), (False, False, True)),
]
for name, actual, expected in light_cases:
    if actual != expected:
        raise SystemExit(f"0.1.60 lighting math failed: {name}: {actual} != {expected}")

if "bCriticalTrailerState ? 2200.0f * HazardPulse : 0.0f" not in cpp:
    raise SystemExit("hazard output is not bounded by authoritative critical state")
if "FMath::Clamp(" not in cpp or "-MaximumLateralStabilityForce" not in cpp:
    raise SystemExit("anti-sway force bound missing")
if "-MaximumYawStabilityTorque" not in cpp:
    raise SystemExit("anti-sway torque bound missing")

print(
    "GTT 0.1.60 trailer stability + road-feedback source contract: PASS "
    f"({len(stability_cases)} stability + {len(light_cases)} lighting cases)"
)
