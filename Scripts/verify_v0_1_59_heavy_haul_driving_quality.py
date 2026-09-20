#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.59 heavy-haul driving quality."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "Source/GTT/Private/Activities/GTTHeavyHaulDirector.cpp"
HDR = ROOT / "Source/GTT/Public/Activities/GTTHeavyHaulDirector.h"

cpp = CPP.read_text(encoding="utf-8")
hdr = HDR.read_text(encoding="utf-8")

required_header = [
    "GetSmoothHaulSeconds",
    "GetRoughHaulSeconds",
    "IsSmoothHaulBonusArmed",
    "UpdateDrivingQuality",
    "ResetDrivingQuality",
    "SmoothHaulTargetSeconds = 60.0f",
    "RoughHaulAllowanceSeconds = 12.0f",
    "SmoothHaulBonus = 180",
    "SmoothMinSpeedKmh = 16.0f",
    "SmoothMaxSpeedKmh = 52.0f",
    "RoughSpeedKmh = 65.0f",
    "SmoothMaxHitchLoad = 0.32f",
    "RoughHitchLoad = 0.58f",
]
required_cpp = [
    "void AGTTHeavyHaulDirector::UpdateDrivingQuality(float DeltaSeconds)",
    "Stage != EGTTHeavyHaulStage::DeliverHillFarm",
    "!Trailer->HasCargo()",
    "!Trailer->IsAttached()",
    "SpeedKmh >= SmoothMinSpeedKmh",
    "SpeedKmh <= SmoothMaxSpeedKmh",
    "HitchLoad <= SmoothMaxHitchLoad",
    "RollDegrees <= SmoothMaxRollDegrees",
    "PitchDegrees <= SmoothMaxPitchDegrees",
    "SpeedKmh > RoughSpeedKmh",
    "HitchLoad > RoughHitchLoad",
    "SmoothHaulSeconds = FMath::Min(SmoothHaulTargetSeconds",
    "RoughHaulSeconds += DeltaSeconds",
    "SMOOTH HAUL READY",
    "rough-driving allowance exceeded",
    "bSmoothHaul ? SmoothHaulBonus : 0",
    "HEAVY_HAUL_DRIVING_QUALITY event=DELIVER",
    "smooth %.0f/%.0fs",
    "rough %.0f/%.0fs",
    "BONUS ARMED",
    "GetLockedRoadsideRepairQuote",
]

missing = [token for token in required_header if token not in hdr]
missing += [token for token in required_cpp if token not in cpp]
if missing:
    raise SystemExit("0.1.59 source contract missing: " + ", ".join(missing))

def smooth(speed, hitch, roll, pitch, axle=True, trailer=1.0, cargo=1.0):
    return (
        speed >= 16.0
        and speed <= 52.0
        and hitch <= 0.32
        and abs(roll) <= 10.0
        and abs(pitch) <= 8.0
        and axle
        and trailer >= 0.75
        and cargo >= 0.88
    )

def rough(speed, hitch, roll, pitch, axle=True):
    return (
        speed > 65.0
        or hitch > 0.58
        or abs(roll) > 22.0
        or abs(pitch) > 17.0
        or not axle
    )

cases = [
    ("cruise", smooth(32, 0.10, 2, 1), True),
    ("minimum-speed", smooth(16, 0.32, 10, 8), True),
    ("too-slow", smooth(15.9, 0.10, 2, 1), False),
    ("too-fast-for-smooth", smooth(52.1, 0.10, 2, 1), False),
    ("hitch-too-loaded", smooth(32, 0.321, 2, 1), False),
    ("damaged-cargo", smooth(32, 0.10, 2, 1, True, 1.0, 0.87), False),
    ("rough-speed", rough(65.1, 0.10, 2, 1), True),
    ("rough-hitch", rough(40, 0.59, 2, 1), True),
    ("rough-roll", rough(40, 0.10, 22.1, 1), True),
    ("rough-pitch", rough(40, 0.10, 2, 17.1), True),
    ("lost-wheel", rough(25, 0.10, 2, 1, False), True),
    ("normal-not-rough", rough(40, 0.20, 8, 5), False),
]
for name, actual, expected in cases:
    if actual != expected:
        raise SystemExit(f"0.1.59 math case failed: {name}: {actual} != {expected}")

if "SmoothHaulSeconds >= SmoothHaulTargetSeconds" not in cpp:
    raise SystemExit("bonus missing smooth-time gate")
if "RoughHaulSeconds <= RoughHaulAllowanceSeconds" not in cpp:
    raise SystemExit("bonus missing rough-exposure gate")
if "Trailer->GetCargoIntegrity() >= 0.90f" not in cpp:
    raise SystemExit("bonus missing cargo-quality gate")

print(f"GTT 0.1.59 heavy-haul driving-quality source contract: PASS ({len(cases)} math cases)")
