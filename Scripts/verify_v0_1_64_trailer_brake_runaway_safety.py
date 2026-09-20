#!/usr/bin/env python3
"""Deterministic source/math verifier for GTT 0.1.64 trailer brake runaway safety."""

from __future__ import annotations

import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterChaosMovementComponent.cpp"
HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterChaosMovementComponent.h"

cpp = CPP.read_text(encoding="utf-8")
header = HEADER.read_text(encoding="utf-8")

required_cpp = [
    "TrailerBrakeHotEnterHeat = 0.50f",
    "TrailerBrakeHotExitHeat = 0.42f",
    "TrailerBrakeFadeExitHeat = 0.56f",
    "TrailerBrakeCriticalEnterHeat = 0.88f",
    "TrailerBrakeCriticalExitHeat = 0.78f",
    "RunawayMinimumTowLoad = 0.50f",
    "RunawayMinimumGradeDegrees = 8.0f",
    "RunawayMinimumSpeedKmh = 24.0f",
    "RunawayFullSpeedKmh = 40.0f",
    "RunawaySafetyBrakeMin = 0.08f",
    "RunawaySafetyBrakeMax = 0.20f",
    "UpdateTrailerBrakeThermalState()",
    "TrailerBrakeThermalState == EGTTTrailerBrakeThermalState::Critical",
    "TrailerRunawaySafetyBrake = FMath::Lerp",
    "HillHaulBrake = FMath::Max(HillHaulBrake, TrailerRunawaySafetyBrake)",
    "bTrailerRunawayMitigationActive = true",
]
for token in required_cpp:
    assert token in cpp, f"missing 0.1.64 source contract: {token}"

required_header = [
    "enum class EGTTTrailerBrakeThermalState",
    "GetTrailerBrakeThermalState",
    "IsTrailerRunawayMitigationActive",
    "GetTrailerRunawaySafetyBrake",
]
for token in required_header:
    assert token in header, f"missing 0.1.64 telemetry contract: {token}"

# Preserve earlier ownership/safety contracts.
assert "SetTargetGear(" not in cpp, "Fieldmaster movement must not steal final drivetrain direction authority"
assert "RequestedDownhillTowBrake * TrailerBrakeAuthority" in cpp, "0.1.63 thermal fade must remain authoritative"
assert "FMath::Max(BaseBrake, HillHaulBrake)" in cpp, "base brake must remain outside thermal fade"
assert "TrailerBrakeThermalMaxDeltaSeconds = 0.10f" in cpp, "thermal integration hitch cap must remain"

HOT_ENTER = 0.50
HOT_EXIT = 0.42
FADE_ENTER = 0.62
FADE_EXIT = 0.56
CRITICAL_ENTER = 0.88
CRITICAL_EXIT = 0.78
RUNAWAY_LOAD = 0.50
RUNAWAY_GRADE = 8.0
RUNAWAY_SPEED = 24.0
RUNAWAY_FULL_SPEED = 40.0
RUNAWAY_BRAKE_MIN = 0.08
RUNAWAY_BRAKE_MAX = 0.20

NORMAL, HOT, FADING, CRITICAL = range(4)


def clamp(value: float, low: float = 0.0, high: float = 1.0) -> float:
    return max(low, min(high, value))


def update_state(state: int, heat: float) -> int:
    if state == CRITICAL:
        if heat < CRITICAL_EXIT:
            if heat >= FADE_EXIT:
                return FADING
            if heat >= HOT_EXIT:
                return HOT
            return NORMAL
        return CRITICAL
    if state == FADING:
        if heat >= CRITICAL_ENTER:
            return CRITICAL
        if heat < FADE_EXIT:
            return HOT if heat >= HOT_EXIT else NORMAL
        return FADING
    if state == HOT:
        if heat >= CRITICAL_ENTER:
            return CRITICAL
        if heat >= FADE_ENTER:
            return FADING
        if heat < HOT_EXIT:
            return NORMAL
        return HOT
    if heat >= CRITICAL_ENTER:
        return CRITICAL
    if heat >= FADE_ENTER:
        return FADING
    if heat >= HOT_ENTER:
        return HOT
    return NORMAL


def runaway_brake(*, heat: float, tow_load: float, grade: float, speed: float, downhill_active: bool) -> float:
    state = update_state(NORMAL, heat)
    if not (
        downhill_active
        and state == CRITICAL
        and tow_load >= RUNAWAY_LOAD
        and grade <= -RUNAWAY_GRADE
        and speed >= RUNAWAY_SPEED
    ):
        return 0.0
    heat_alpha = clamp((heat - CRITICAL_ENTER) / (1.0 - CRITICAL_ENTER))
    grade_alpha = clamp((abs(grade) - RUNAWAY_GRADE) / (12.0 - RUNAWAY_GRADE))
    speed_alpha = clamp((speed - RUNAWAY_SPEED) / (RUNAWAY_FULL_SPEED - RUNAWAY_SPEED))
    severity = max(heat_alpha, grade_alpha, speed_alpha)
    return (RUNAWAY_BRAKE_MIN + (RUNAWAY_BRAKE_MAX - RUNAWAY_BRAKE_MIN) * severity) * clamp(tow_load)


# Hysteresis: entering/exiting each band must not chatter on the same threshold.
state = NORMAL
state = update_state(state, 0.51)
assert state == HOT
assert update_state(state, 0.49) == HOT
state = update_state(state, 0.63)
assert state == FADING
assert update_state(state, 0.59) == FADING
state = update_state(state, 0.89)
assert state == CRITICAL
assert update_state(state, 0.82) == CRITICAL
state = update_state(state, 0.77)
assert state == FADING
state = update_state(state, 0.55)
assert state == HOT
state = update_state(state, 0.41)
assert state == NORMAL

# Runaway mitigation is narrow and bounded.
assert runaway_brake(heat=0.95, tow_load=1.0, grade=-10.0, speed=32.0, downhill_active=True) > 0.0
assert runaway_brake(heat=0.95, tow_load=0.49, grade=-10.0, speed=32.0, downhill_active=True) == 0.0
assert runaway_brake(heat=0.95, tow_load=1.0, grade=-7.9, speed=32.0, downhill_active=True) == 0.0
assert runaway_brake(heat=0.95, tow_load=1.0, grade=-10.0, speed=23.9, downhill_active=True) == 0.0
assert runaway_brake(heat=0.95, tow_load=1.0, grade=-10.0, speed=32.0, downhill_active=False) == 0.0
full = runaway_brake(heat=1.0, tow_load=1.0, grade=-12.0, speed=40.0, downhill_active=True)
assert math.isclose(full, RUNAWAY_BRAKE_MAX, abs_tol=1e-9)
assert full <= 0.20

print("GTT 0.1.64 trailer brake runaway safety: source contract OK")
print("GTT 0.1.64 trailer brake runaway safety: hysteresis + bounded intervention math OK")
print("Preserved: 0.1.63 thermal fade, 0.1.62 hill-haul, base brake and drivetrain ownership")
