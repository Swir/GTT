#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.60 authored trailer runtime/evidence bridge."""
from __future__ import annotations

import math
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HDR = ROOT / "Source/GTT/Public/Vehicles/GTTAuthoredTrailerPresentationSubsystem.h"
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTAuthoredTrailerPresentationSubsystem.cpp"
ROAD_FEEDBACK_HDR = ROOT / "Source/GTT/Public/Vehicles/GTTTrailerRoadFeedbackSubsystem.h"
ROADMAP = ROOT / "Docs/ROADMAP.md"
SOURCE_RIG = ROOT / "Scripts/generate_gtt_farm_trailer_gltf.py"
SOURCE_VERIFY = ROOT / "Scripts/verify_v0_1_60_authored_trailer_source_rig.py"
RUNTIME_EVALUATOR = ROOT / "Scripts/evaluate_authored_trailer_runtime.ps1"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.1.60_AUTHORED_TRAILER_RUNTIME_BRIDGE.md"
CHANGELOG = ROOT / "CHANGELOG.d/0.1.60-authored-trailer-runtime-bridge.md"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: Path) -> str:
    require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def parse_cpp_float(text: str, name: str) -> float:
    match = re.search(
        rf"(?:static\s+)?constexpr\s+float\s+{re.escape(name)}\s*=\s*([0-9]+(?:\.[0-9]+)?)f\s*;",
        text,
    )
    require(match is not None, f"missing constexpr float {name}")
    return float(match.group(1))


def jackknife_risk(angle_deg: float, speed_kmh: float, load: float, c: dict[str, float]) -> float:
    angle = min(max((angle_deg - c["JackknifeWarningAngleDegrees"]) /
                    (c["JackknifeCriticalAngleDegrees"] - c["JackknifeWarningAngleDegrees"]), 0.0), 1.0)
    speed = min(max((speed_kmh - c["JackknifeStartSpeedKmh"]) /
                    (c["JackknifeFullSpeedKmh"] - c["JackknifeStartSpeedKmh"]), 0.0), 1.0)
    return min(max(angle * speed * min(max(load, 0.35), 1.0), 0.0), 1.0)


def stability_authority(
    speed_kmh: float,
    load: float,
    trailer_integrity: float,
    hitch_load: float,
    risk: float,
    c: dict[str, float],
) -> float:
    speed = min(max((speed_kmh - c["StabilityStartSpeedKmh"]) /
                    (c["StabilityFullSpeedKmh"] - c["StabilityStartSpeedKmh"]), 0.0), 1.0)
    hitch_reserve = 1.0 - min(max(hitch_load, 0.0), 1.0)
    integrity = min(trailer_integrity, hitch_reserve)
    integrity_authority = min(max((integrity - 0.20) / 0.80, 0.15), 1.0)
    boost = 1.0 + (c["MaximumJackknifeAssistMultiplier"] - 1.0) * min(max(risk, 0.0), 1.0)
    return min(
        speed * min(max(load, 0.0), 1.0) * integrity_authority *
        c["MaximumStabilityAuthority"] * boost,
        c["MaximumStabilityAuthority"],
    )


def main() -> int:
    hdr = read(HDR)
    cpp = read(CPP)
    road_hdr = read(ROAD_FEEDBACK_HDR)
    roadmap = read(ROADMAP)
    source_rig = read(SOURCE_RIG)
    source_verify = read(SOURCE_VERIFY)
    evaluator = read(RUNTIME_EVALUATOR)
    playtest = read(PLAYTEST)
    changelog = read(CHANGELOG)

    required_header = [
        "UGTTAuthoredTrailerPresentationSubsystem : public UTickableWorldSubsystem",
        "TryActivateAuthoredPresentation",
        "ValidateAuthoredAsset",
        "UpdateWheelPose",
        "EmitRuntimeEvidence",
        "TWeakObjectPtr<UPoseableMeshComponent>",
        "TWeakObjectPtr<UStaticMeshComponent> HitchCoupler",
        "ScenarioDistanceCm",
        "ScenarioDualContactSamples",
        "ScenarioSafeSamples",
    ]
    required_cpp = [
        "/Game/GTT/Vehicles/Trailer/SK_GTT_FarmTrailer.SK_GTT_FarmTrailer",
        "LoadObject<USkeletalMesh>",
        "FindBoneIndex",
        "FindSocket",
        "GetPhysicsAsset()",
        "UPoseableMeshComponent",
        "SetBoneTransformByName",
        "LeftWheel",
        "RightWheel",
        "HitchCoupler",
        "HidePlaceholderPresentation",
        "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
        "AUTHORED_TRAILER_PRESENTATION event=ACTIVATED",
        "AUTHORED_TRAILER_PRESENTATION event=REJECTED",
        "LineTraceSingleByChannel",
        "AUTHORED_TRAILER_RUNTIME_EVIDENCE trailer=%s active=1 nativeTow=%d",
        "NATIVE_TRAILER_SCENARIO_SAMPLE speed_kmh=%.2f distance_cm=%.1f loaded=1 attached=1 active=1",
        "NATIVE_TRAILER_SCENARIO_COMPLETE result=PASS route=loaded-authored-tow",
        "NATIVE_TRAILER_SCENARIO phase=DIAGNOSTIC result=FAIL reason=HITCH_ENVELOPE",
        "ScenarioMinimumDistanceCm = 900.0f",
        "ScenarioMinimumSafeSamples = 8",
        "ScenarioSafeHitchErrorCm = 80.0f",
        "ScenarioHardHitchErrorCm = 110.0f",
    ]
    for token in required_header:
        require(token in hdr, f"runtime bridge header contract missing: {token}")
    for token in required_cpp:
        require(token in cpp, f"runtime evidence source contract missing: {token}")

    for forbidden in (
        "/Engine/BasicShapes/",
        "SetSimulatePhysics(true)",
        "SetConstrainedComponents(",
        "AddForce(",
        "AddTorque",
        "PerformRoadsideRepair(",
        "SpendCash(",
        "AddCash(",
    ):
        require(forbidden not in cpp, f"presentation/evidence bridge must stay observational: {forbidden}")

    # Existing packaged evaluator and the new runtime emitter must use the same markers/schema.
    for marker in (
        "AUTHORED_TRAILER_RUNTIME_EVIDENCE",
        "NATIVE_TRAILER_SCENARIO_SAMPLE",
        "NATIVE_TRAILER_SCENARIO_COMPLETE",
        "NATIVE_TRAILER_SCENARIO phase=DIAGNOSTIC result=FAIL",
        "gtt.native-trailer-runtime.v1",
    ):
        require(marker in evaluator, f"authored trailer evaluator missing marker/schema: {marker}")
    require("$OutputPath = Join-Path $PackageDirectory 'NATIVE_TRAILER_RUNTIME.json'" in evaluator,
            "authored trailer evaluator no longer emits NATIVE_TRAILER_RUNTIME.json")
    require("same-SHA" in playtest or "same-SHA" in changelog,
            "acceptance docs must retain same-SHA packaged evidence requirement")

    # The evidence-only copy of handling math must stay tied to the authoritative road-feedback constants.
    shared_constants = [
        "StabilityStartSpeedKmh",
        "StabilityFullSpeedKmh",
        "MaximumStabilityAuthority",
        "JackknifeWarningAngleDegrees",
        "JackknifeCriticalAngleDegrees",
        "JackknifeStartSpeedKmh",
        "JackknifeFullSpeedKmh",
        "JackknifeWarningRiskThreshold",
        "MaximumJackknifeAssistMultiplier",
    ]
    evidence_constants: dict[str, float] = {}
    for name in shared_constants:
        evidence_value = parse_cpp_float(cpp, name)
        authoritative_value = parse_cpp_float(road_hdr, name)
        require(
            math.isclose(evidence_value, authoritative_value, rel_tol=0.0, abs_tol=1e-9),
            f"runtime evidence math drift: {name} evidence={evidence_value} road_feedback={authoritative_value}",
        )
        evidence_constants[name] = evidence_value

    # Deterministic boundary checks protect evaluator telemetry from accidental threshold drift.
    require(jackknife_risk(31.9, 70.0, 1.0, evidence_constants) == 0.0,
            "jackknife evidence should be zero below warning angle")
    require(jackknife_risk(62.0, 58.0, 1.0, evidence_constants) == 1.0,
            "jackknife evidence should reach one at critical angle/full speed/full load")
    mid_risk = jackknife_risk(47.0, 43.0, 0.70, evidence_constants)
    require(0.0 < mid_risk < 1.0, "jackknife evidence mid-case must stay bounded")
    authority = stability_authority(120.0, 1.0, 1.0, 0.0, 1.0, evidence_constants)
    require(math.isclose(authority, 0.45, rel_tol=0.0, abs_tol=1e-9),
            f"stability evidence exceeded/changed 45% cap: {authority}")
    require(stability_authority(20.0, 1.0, 1.0, 0.0, 1.0, evidence_constants) == 0.0,
            "stability evidence must be zero below start speed")

    # Source art remains original/deterministic; v2 adds Interchange-ready SOCKET_ source anchors
    # while the runtime bridge continues to require the same lowercase final UE mesh socket names.
    for token in (
        "gtt.farm-trailer-source-rig.v2",
        "Project-owned original source art",
        "required_sockets",
        "SOCKET_socket_hitch",
        "SOCKET_socket_cargo",
        "SOCKET_socket_axle_l",
        "SOCKET_socket_axle_r",
    ):
        require(token in source_rig, f"authored source rig contract missing: {token}")
    require("source-rig candidate only" in source_rig, "source rig must remain labelled as candidate-only")
    require("gtt.farm-trailer-source-rig.v2" in source_verify,
            "source-rig verifier must validate the current Interchange-ready canonical asset contract")
    require("SOCKET_NODE_NAMES" in source_verify,
            "source-rig verifier must retain Interchange socket-name validation")

    open_gate = "- [ ] Authored skeletal trailer wheel assets and final hitch sockets"
    require(open_gate in roadmap, "authored trailer roadmap gate was closed without packaged UE acceptance")
    checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
    unchecked = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
    require((checked, checked + unchecked) == (125, 130),
            f"roadmap truth changed unexpectedly while runtime evidence remains source-only: {checked}/{checked + unchecked}")
    require("96.2%" in roadmap, "roadmap percentage must remain 96.2% until acceptance evidence closes a gate")

    require("passive packaged-runtime evidence" in playtest.lower(),
            "playtest must document the passive packaged-runtime evidence path")
    require("runtime evidence" in changelog.lower(),
            "changelog must document the runtime evidence bridge")

    print("GTT 0.1.60 authored trailer runtime/evidence bridge source contract: PASS")
    print("Emitter schema matches packaged evaluator; handling telemetry mirrors authoritative constants.")
    print("Roadmap remains 125/130 (96.2%) pending real UE 5.8 import, Win64 runtime and visual acceptance.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
