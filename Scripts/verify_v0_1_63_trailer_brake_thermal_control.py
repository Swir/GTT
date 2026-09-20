#!/usr/bin/env python3
"""Deterministic source/math verifier for GTT 0.1.63 trailer brake thermal control."""

from __future__ import annotations

import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterChaosMovementComponent.cpp"
HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterChaosMovementComponent.h"

cpp = CPP.read_text(encoding="utf-8")
header = HEADER.read_text(encoding="utf-8")

required_cpp = [
    "TrailerBrakeHeatBuildPerSecond = 0.18f",
    "TrailerBrakeCoolingPerSecond = 0.09f",
    "TrailerBrakeFadeStartHeat = 0.62f",
    "TrailerBrakeFadeFullHeat = 0.92f",
    "TrailerBrakeMinimumAuthority = 0.55f",
    "TrailerBrakeThermalMaxDeltaSeconds = 0.10f",
    "RequestedDownhillTowBrake",
    "TrailerBrakeHeat01 = FMath::Clamp",
    "TrailerBrakeAuthority = FMath::Lerp",
    "bTrailerBrakeFadeActive = FadeAlpha > KINDA_SMALL_NUMBER",
    "RequestedDownhillTowBrake * TrailerBrakeAuthority",
    "FMath::Max(HillHaulBrake, ThermallyLimitedDownhillBrake)",
]
for token in required_cpp:
    assert token in cpp, f"missing 0.1.63 source contract: {token}"

required_header = [
    "GetTrailerBrakeHeat01",
    "GetTrailerBrakeAuthority",
    "IsTrailerBrakeFadeActive",
    "IsTrailerBrakeCoolingActive",
]
for token in required_header:
    assert token in header, f"missing 0.1.63 telemetry contract: {token}"

# Preserve pre-existing ownership and safety contracts.
assert "SetTargetGear(" not in cpp, "Fieldmaster movement must not steal final drivetrain direction authority"
assert "FMath::Max(BaseBrake, HillHaulBrake)" in cpp, "manual/base brake authority must remain outside thermal fade"
assert "bHillHoldActive = true" in cpp, "hill hold must remain present"

BUILD_PER_SECOND = 0.18
COOL_PER_SECOND = 0.09
FADE_START = 0.62
FADE_FULL = 0.92
MIN_AUTHORITY = 0.55
MAX_DT = 0.10
DOWNHILL_MAX_BRAKE = 0.46


def clamp(value: float, low: float = 0.0, high: float = 1.0) -> float:
    return max(low, min(high, value))


def authority(heat: float) -> float:
    fade_alpha = clamp((heat - FADE_START) / (FADE_FULL - FADE_START))
    return 1.0 + (MIN_AUTHORITY - 1.0) * fade_alpha


def step(heat: float, *, requested_brake: float, tow_load: float, dt: float, downhill_active: bool):
    dt = clamp(dt, 0.0, MAX_DT)
    heat = clamp(heat)
    if downhill_active and requested_brake > 0.0:
        demand = clamp(requested_brake / DOWNHILL_MAX_BRAKE)
        load_scale = 0.65 + 0.35 * clamp(tow_load)
        heat = clamp(heat + BUILD_PER_SECOND * demand * load_scale * dt)
    elif heat > 0.0:
        heat = clamp(heat - COOL_PER_SECOND * dt)
    auth = authority(heat)
    applied = requested_brake * auth if requested_brake > 0.0 else 0.0
    return heat, auth, applied

# Cold brakes retain the exact 0.1.62 requested downhill authority.
heat, auth, applied = step(0.0, requested_brake=DOWNHILL_MAX_BRAKE, tow_load=1.0, dt=0.1, downhill_active=True)
assert auth == 1.0
assert math.isclose(applied, DOWNHILL_MAX_BRAKE, abs_tol=1e-9)

# Sustained full-demand downhill braking must eventually enter fade, remain
# bounded, and never erase all trailer-assist braking.
heat = 0.0
for _ in range(60):  # six seconds at the capped 100 ms integration step
    heat, auth, applied = step(heat, requested_brake=DOWNHILL_MAX_BRAKE, tow_load=1.0, dt=0.1, downhill_active=True)
assert heat > FADE_FULL
assert math.isclose(auth, MIN_AUTHORITY, abs_tol=1e-9)
assert math.isclose(applied, DOWNHILL_MAX_BRAKE * MIN_AUTHORITY, abs_tol=1e-9)

# Light trailer demand heats slower than a full load.
light_heat = 0.0
full_heat = 0.0
for _ in range(20):
    light_heat, *_ = step(light_heat, requested_brake=0.20, tow_load=0.25, dt=0.1, downhill_active=True)
    full_heat, *_ = step(full_heat, requested_brake=0.20, tow_load=1.0, dt=0.1, downhill_active=True)
assert 0.0 < light_heat < full_heat

# Leaving the downhill-brake condition must cool and restore full authority.
heat = 1.0
for _ in range(120):
    heat, auth, applied = step(heat, requested_brake=0.0, tow_load=1.0, dt=0.1, downhill_active=False)
assert math.isclose(heat, 0.0, abs_tol=1e-9)
assert math.isclose(auth, 1.0, abs_tol=1e-9)
assert applied == 0.0

# A one-second frame hitch must not integrate as a full second; the source cap
# makes thermal state deterministic and resistant to transient stalls.
hitch_heat, *_ = step(0.0, requested_brake=DOWNHILL_MAX_BRAKE, tow_load=1.0, dt=1.0, downhill_active=True)
normal_heat, *_ = step(0.0, requested_brake=DOWNHILL_MAX_BRAKE, tow_load=1.0, dt=0.1, downhill_active=True)
assert math.isclose(hitch_heat, normal_heat, abs_tol=1e-9)

print("GTT 0.1.63 trailer brake thermal control: source contract OK")
print("GTT 0.1.63 trailer brake thermal control: deterministic heat/fade/recovery math OK")
print("Preserved: cold 0.1.62 downhill brake authority; base brake and hill hold are not thermally faded")
