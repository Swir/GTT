#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.60 authored trailer runtime presentation bridge."""
from __future__ import annotations
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HDR = ROOT / "Source/GTT/Public/Vehicles/GTTAuthoredTrailerPresentationSubsystem.h"
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTAuthoredTrailerPresentationSubsystem.cpp"
ROADMAP = ROOT / "Docs/ROADMAP.md"
SOURCE_RIG = ROOT / "Scripts/generate_gtt_farm_trailer_gltf.py"
SOURCE_VERIFY = ROOT / "Scripts/verify_v0_1_60_authored_trailer_source_rig.py"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def main() -> int:
    for path in (HDR, CPP, ROADMAP, SOURCE_RIG, SOURCE_VERIFY):
        require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")

    hdr = HDR.read_text(encoding="utf-8")
    cpp = CPP.read_text(encoding="utf-8")
    roadmap = ROADMAP.read_text(encoding="utf-8")

    required_header = [
        "UGTTAuthoredTrailerPresentationSubsystem : public UTickableWorldSubsystem",
        "TryActivateAuthoredPresentation",
        "ValidateAuthoredAsset",
        "UpdateWheelPose",
        "TWeakObjectPtr<UPoseableMeshComponent>",
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
        "HidePlaceholderPresentation",
        "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
        "AUTHORED_TRAILER_PRESENTATION event=ACTIVATED",
        "AUTHORED_TRAILER_PRESENTATION event=REJECTED",
    ]
    for token in required_header:
        require(token in hdr, f"runtime bridge header contract missing: {token}")
    for token in required_cpp:
        require(token in cpp, f"runtime bridge source contract missing: {token}")

    for forbidden in ("/Engine/BasicShapes/", "SetSimulatePhysics(true)", "SetConstrainedComponents("):
        require(forbidden not in cpp, f"presentation bridge must not own physics/placeholder authority: {forbidden}")

    open_gate = "- [ ] Authored skeletal trailer wheel assets and final hitch sockets"
    require(open_gate in roadmap, "authored trailer roadmap gate was closed without packaged UE acceptance")
    require("125" in roadmap and "130" in roadmap and "96.2%" in roadmap,
            "roadmap truth changed unexpectedly while runtime bridge remains source-only")

    print("GTT 0.1.60 authored trailer runtime presentation bridge source contract: PASS")
    print("Roadmap gate intentionally remains open pending UE 5.8 import, PhysicsAsset/socket, Win64 runtime and visual acceptance.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
