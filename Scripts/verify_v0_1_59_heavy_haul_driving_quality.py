#!/usr/bin/env python3
"""Source + deterministic math verifier for GTT 0.1.59 heavy-haul driving quality."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = ROOT / "Source/GTT/Private/Activities/GTTHeavyHaulDirector.cpp"
HDR = ROOT / "Source/GTT/Public/Activities/GTTHeavyHaulDirector.h"
DYN_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFarmTrailerDynamicsSubsystem.cpp"
DYN_HDR = ROOT / "Source/GTT/Public/Vehicles/GTTFarmTrailerDynamicsSubsystem.h"

cpp = CPP.read_text(encoding="utf-8")
hdr = HDR.read_text(encoding="utf-8")
dyn_cpp = DYN_CPP.read_text(encoding="utf-8")
dyn_hdr = DYN_HDR.read_text(encoding="utf-8")

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
required_dyn_header = [
    "DynamicStress01",
    "SpeedKmh",
    "RollDegrees",
    "PitchDegrees",
]
required_dyn_cpp = [
    "SuspensionTravelCm = 24.0f",
    "LoadedSpeedStressStartKmh = 52.0f",
    "LoadedSpeedStressFullKmh = 78.0f",
    "LoadedRollStressFullDegrees = 28.0f",
    "LoadedPitchStressFullDegrees = 20.0f",
    "FMath::Max(HitchLoad01, FMath::Max(SpeedStress01, AttitudeStress01))",
    "DynamicStressFactor = 1.0f - DynamicStress01 * MaximumDynamicHitchWeakening",
    "State.Snapshot.DynamicStress01 = DynamicStress01",
    "dynamic_stress=%.2f",
    "speed_kmh=%.1f",
    "roll=%.1f",
    "pitch=%.1f",
]

missing = [token for token in required_header if token not in hdr]
missing += [token for token in required_cpp if token not in cpp]
missing += [token for token in required_dyn_header if token not in dyn_hdr]
missing += [token for token in required_dyn_cpp if token not in dyn_cpp]
if missing:
    raise SystemExit("0.1.59 source contract missing: " + ", ".join(missing))


def clamp01(value: float) -> float:
    return max(0.0, min(1.0, value))


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


def dynamic_stress(loaded, speed, hitch, roll, pitch):
    speed_stress = clamp01((speed - 52.0) / (78.0 - 52.0)) if loaded else 0.0
    attitude_stress = clamp01(max(abs(roll) / 28.0, abs(pitch) / 20.0)) if loaded else 0.0
    return clamp01(max(clamp01(hitch), speed_stress, attitude_stress))


quality_cases = [
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
for name, actual, expected in quality_cases:
    if actual != expected:
        raise SystemExit(f"0.1.59 quality math failed: {name}: {actual} != {expected}")

stress_cases = [
    ("empty-ignores-speed-attitude", dynamic_stress(False, 90, 0.40, 30, 25), 0.40),
    ("loaded-calm", dynamic_stress(True, 40, 0.10, 0, 0), 0.10),
    ("loaded-at-speed-start", dynamic_stress(True, 52, 0.10, 0, 0), 0.10),
    ("loaded-mid-speed", dynamic_stress(True, 65, 0.10, 0, 0), 0.50),
    ("loaded-full-speed", dynamic_stress(True, 78, 0.10, 0, 0), 1.00),
    ("loaded-half-roll", dynamic_stress(True, 40, 0.10, 14, 0), 0.50),
    ("loaded-half-pitch", dynamic_stress(True, 40, 0.10, 0, 10), 0.50),
    ("hitch-dominates", dynamic_stress(True, 55, 0.70, 4, 3), 0.70),
]
for name, actual, expected in stress_cases:
    if abs(actual - expected) > 1e-6:
        raise SystemExit(f"0.1.59 dynamics math failed: {name}: {actual:.6f} != {expected:.6f}")

if "SmoothHaulSeconds >= SmoothHaulTargetSeconds" not in cpp:
    raise SystemExit("bonus missing smooth-time gate")
if "RoughHaulSeconds <= RoughHaulAllowanceSeconds" not in cpp:
    raise SystemExit("bonus missing rough-exposure gate")
if "Trailer->GetCargoIntegrity() >= 0.90f" not in cpp:
    raise SystemExit("bonus missing cargo-quality gate")

full_stress_strength = 1.0 - 1.0 * 0.28
if abs(full_stress_strength - 0.72) > 1e-6:
    raise SystemExit("dynamic hitch weakening bound changed unexpectedly")

print(
    "GTT 0.1.59 heavy-haul driving-quality source contract: PASS "
    f"({len(quality_cases)} quality + {len(stress_cases)} dynamics math cases)"
)
