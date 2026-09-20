#!/usr/bin/env python3
"""Deterministic source/math verifier for GTT 0.1.62 Fieldmaster hill-haul control."""

from __future__ import annotations

import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterChaosMovementComponent.cpp"
HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterChaosMovementComponent.h"

cpp = CPP.read_text(encoding="utf-8")
header = HEADER.read_text(encoding="utf-8")

required_cpp = [
    "MinimumTerrainThrottleAuthority = 0.52f",
    "HillControlMinimumGradeDegrees = 4.0f",
    "HillControlFullGradeDegrees = 12.0f",
    "HillControlMinimumTowLoad = 0.15f",
    "HillHoldMaxSpeedKmh = 2.5f",
    "HillHoldBrakeMin = 0.30f",
    "HillHoldBrakeMax = 0.65f",
    "DownhillTowBrakeStartSpeedKmh = 10.0f",
    "DownhillTowBrakeFullSpeedKmh = 28.0f",
    "DownhillTowBrakeMax = 0.46f",
    "TerrainThrottleAuthority = FMath::Lerp",
    "TravelGradeDegrees = ForwardGradeDegrees * TravelDirection",
    "bHillHoldActive = true",
    "bDownhillTowBrakeActive = true",
    "FMath::Max(BaseBrake, HillHaulBrake)",
    "TowThrottleAuthority * TerrainThrottleAuthority",
]
for token in required_cpp:
    assert token in cpp, f"missing 0.1.62 source contract: {token}"

required_header = [
    "GetTerrainThrottleAuthority",
    "GetTravelGradeDegrees",
    "GetHillHaulBrake",
    "IsHillHoldActive",
    "IsDownhillTowBrakeActive",
]
for token in required_header:
    assert token in header, f"missing 0.1.62 telemetry contract: {token}"

# Preserve the automatic gearbox ownership rule established before 0.1.62.
assert "SetTargetGear" not in cpp, "Fieldmaster movement must not steal shared drivetrain direction authority"

MIN_TERRAIN = 0.52
MAX_TOW_THROTTLE_PENALTY = 0.30
MAX_TOW_STEERING_PENALTY = 0.12
GRADE_MIN = 4.0
GRADE_FULL = 12.0
TOW_LOAD_MIN = 0.15
HILL_HOLD_MAX_SPEED = 2.5
HILL_HOLD_BRAKE_MIN = 0.30
HILL_HOLD_BRAKE_MAX = 0.65
DOWNHILL_START_SPEED = 10.0
DOWNHILL_FULL_SPEED = 28.0
DOWNHILL_BRAKE_MIN = 0.18
DOWNHILL_BRAKE_MAX = 0.46
THROTTLE_DEADZONE = 0.10


def clamp(value: float, low: float = 0.0, high: float = 1.0) -> float:
    return max(low, min(high, value))


def model(*, tire: float, terrain: float, tow: float, throttle: float, grade: float, speed: float):
    tire = clamp(tire)
    terrain = clamp(terrain)
    tow = clamp(tow)
    combined_traction = math.sqrt(clamp(tire * terrain))
    terrain_authority = MIN_TERRAIN + (1.0 - MIN_TERRAIN) * combined_traction
    tow_throttle = 1.0 - tow * MAX_TOW_THROTTLE_PENALTY
    tow_steering = 1.0 - tow * MAX_TOW_STEERING_PENALTY
    effective_throttle = abs(clamp(throttle, -1.0, 1.0)) * tow_throttle * terrain_authority

    grade_alpha = clamp((abs(grade) - GRADE_MIN) / (GRADE_FULL - GRADE_MIN))
    loaded = tow >= TOW_LOAD_MIN
    released = abs(throttle) <= THROTTLE_DEADZONE
    brake = 0.0
    hill_hold = False
    downhill = False

    if loaded and released and abs(grade) >= GRADE_MIN and speed <= HILL_HOLD_MAX_SPEED:
        hill_hold = True
        load_scale = 0.75 + 0.25 * tow
        brake = (HILL_HOLD_BRAKE_MIN + (HILL_HOLD_BRAKE_MAX - HILL_HOLD_BRAKE_MIN) * grade_alpha) * load_scale

    if loaded and released and grade <= -GRADE_MIN and speed >= DOWNHILL_START_SPEED:
        downhill = True
        speed_alpha = clamp((speed - DOWNHILL_START_SPEED) / (DOWNHILL_FULL_SPEED - DOWNHILL_START_SPEED))
        severity = max(grade_alpha, speed_alpha)
        downhill_brake = (DOWNHILL_BRAKE_MIN + (DOWNHILL_BRAKE_MAX - DOWNHILL_BRAKE_MIN) * severity) * tow
        brake = max(brake, downhill_brake)

    return terrain_authority, tow_throttle, tow_steering, effective_throttle, brake, hill_hold, downhill


road_empty = model(tire=1.0, terrain=1.0, tow=0.0, throttle=1.0, grade=0.0, speed=20.0)
assert math.isclose(road_empty[0], 1.0, abs_tol=1e-9)
assert math.isclose(road_empty[3], 1.0, abs_tol=1e-9), "firm-ground no-trailer throttle must stay at 100%"

road_full_tow = model(tire=1.0, terrain=1.0, tow=1.0, throttle=1.0, grade=0.0, speed=20.0)
assert math.isclose(road_full_tow[1], 0.70, abs_tol=1e-9), "0.1.59 full-load throttle cap regressed"
assert math.isclose(road_full_tow[2], 0.88, abs_tol=1e-9), "0.1.59 full-load steering cap regressed"
assert math.isclose(road_full_tow[3], 0.70, abs_tol=1e-9)

mud_full_tow = model(tire=1.0, terrain=0.40, tow=1.0, throttle=1.0, grade=0.0, speed=12.0)
assert 0.52 < mud_full_tow[0] < 1.0
assert 0.0 < mud_full_tow[3] < 0.70, "mud must reduce propulsion below the heavy-haul road cap"

hill_hold = model(tire=1.0, terrain=1.0, tow=1.0, throttle=0.0, grade=10.0, speed=1.0)
assert hill_hold[5] and not hill_hold[6]
assert 0.30 < hill_hold[4] <= HILL_HOLD_BRAKE_MAX

downhill = model(tire=1.0, terrain=1.0, tow=1.0, throttle=0.0, grade=-12.0, speed=28.0)
assert downhill[6] and not downhill[5]
assert math.isclose(downhill[4], DOWNHILL_BRAKE_MAX, abs_tol=1e-9)

# Driver throttle must immediately release the automatic tow brakes; the feature
# is assistance, not an invisible speed controller fighting deliberate input.
driver_override = model(tire=1.0, terrain=1.0, tow=1.0, throttle=0.25, grade=-12.0, speed=28.0)
assert not driver_override[5] and not driver_override[6] and math.isclose(driver_override[4], 0.0, abs_tol=1e-9)

# A trailer below the documented load threshold must not activate hill assistance.
light_tow = model(tire=1.0, terrain=1.0, tow=0.10, throttle=0.0, grade=-12.0, speed=28.0)
assert not light_tow[5] and not light_tow[6]

print("GTT 0.1.62 Fieldmaster hill-haul control: source contract OK")
print("GTT 0.1.62 Fieldmaster hill-haul control: deterministic math OK")
print("Preserved: no trailer road throttle=1.00; full trailer road throttle=0.70 steering=0.88")
