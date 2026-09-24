#!/usr/bin/env python3
"""Static contract verifier for GTT 0.1.60 authored trailer UE import automation."""
from __future__ import annotations

import json
import py_compile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
UPROJECT = ROOT / "GTT.uproject"
IMPORTER = ROOT / "Scripts/Unreal/import_gtt_farm_trailer.py"
WRAPPER = ROOT / "Scripts/import_gtt_farm_trailer_unreal.ps1"
CANDIDATE = ROOT / "Scripts/run_win64_candidate_acceptance.ps1"
GENERATOR = ROOT / "Scripts/generate_gtt_farm_trailer_gltf.py"
SOURCE_VERIFY = ROOT / "Scripts/verify_v0_1_60_authored_trailer_source_rig.py"
ROADMAP = ROOT / "Docs/ROADMAP.md"


def require(value, message: str) -> None:
    if not value:
        raise SystemExit(message)


def main() -> int:
    for path in (UPROJECT, IMPORTER, WRAPPER, CANDIDATE, GENERATOR, SOURCE_VERIFY, ROADMAP):
        require(path.is_file(), f"missing required file: {path.relative_to(ROOT)}")

    py_compile.compile(str(IMPORTER), doraise=True)
    project = json.loads(UPROJECT.read_text(encoding="utf-8"))
    enabled = {p["Name"] for p in project.get("Plugins", []) if p.get("Enabled")}
    require("PythonScriptPlugin" in enabled, "PythonScriptPlugin must be enabled for headless import")
    require("EditorScriptingUtilities" in enabled, "EditorScriptingUtilities must be enabled for asset save helpers")
    require(project.get("EngineAssociation") == "5.8", "import automation is locked to UE 5.8 project target")

    importer = IMPORTER.read_text(encoding="utf-8")
    wrapper = WRAPPER.read_text(encoding="utf-8")
    candidate = CANDIDATE.read_text(encoding="utf-8")
    generator = GENERATOR.read_text(encoding="utf-8")
    source_verify = SOURCE_VERIFY.read_text(encoding="utf-8")
    roadmap = ROADMAP.read_text(encoding="utf-8")

    required_importer = (
        "InterchangeManager.get_interchange_manager_scripted",
        "InterchangeManager.create_source_data",
        "InterchangeProjectSettingsScript.get_pipeline_stack_from_source_data",
        "InterchangeGenericAssetsPipeline",
        '"import_skeletal_meshes", True',
        '"import_static_meshes", False',
        '"create_physics_asset", True',
        '"import_sockets", True',
        "ImportAssetParameters(",
        "is_automated=True",
        "pipeline_paths = [unreal.SoftObjectPath(pipeline.get_path_name()) for pipeline in pipelines]",
        "override_pipelines=pipeline_paths",
        "manager.import_asset",
        "EditorAssetLibrary.list_assets(",
        "EditorAssetLibrary.rename_asset(mesh.get_path_name(), ASSET_PATH)",
        "/Game/GTT/Vehicles/Trailer",
        "SK_GTT_FarmTrailer",
        "SkeletalMeshEditorSubsystem.create_physics_asset",
        "mesh.rename_socket",
        "AUTHORED_TRAILER_IMPORT result=PASS",
        "AUTHORED_TRAILER_IMPORT result=FAIL",
        "AUTHORED_TRAILER_EDITOR_ACCEPTANCE.json",
        "gtt.authored-trailer-editor-acceptance.v1",
        "source_gltf_sha256",
        "verified_bones",
        "verified_sockets",
        "physics_asset_object_path",
        "hashlib.sha256",
    )
    for token in required_importer:
        require(token in importer, f"UE import contract missing: {token}")

    for token in (
        "socket_hitch", "socket_cargo", "socket_axle_l", "socket_axle_r",
        "SOCKET_{name}",
    ):
        require(token in generator, f"generator Interchange-socket contract missing: {token}")
        require(token in source_verify or token == "SOCKET_{name}",
                f"source verifier socket contract missing: {token}")

    required_wrapper = (
        "generate_gtt_farm_trailer_gltf.py",
        "verify_v0_1_60_authored_trailer_source_rig.py",
        "-ExecutePythonScript=",
        "-ScriptErrorsAreFatal",
        "-abslog=",
        "import_gtt_farm_trailer.py",
        "-unattended",
        "-NullRHI",
        "AUTHORED_TRAILER_IMPORT result=PASS",
        "AUTHORED_TRAILER_EDITOR_ACCEPTANCE.json",
        "gtt.authored-trailer-editor-acceptance.v1",
        "Get-FileHash -Algorithm SHA256",
        "required_bones",
        "verified_bones",
        "required_sockets",
        "verified_sockets",
        "GITHUB_SHA",
    )
    for token in required_wrapper:
        require(token in wrapper, f"PowerShell import wrapper contract missing: {token}")

    required_candidate = (
        "AUTHORED_TRAILER_EDITOR_ACCEPTANCE.json",
        "gtt.authored-trailer-editor-acceptance.v1",
        "editor_acceptance",
        "source_gltf_sha256",
        "physics_asset_object_path",
        "Copy-Item -Force $EditorImportEvidenceFile",
    )
    for token in required_candidate:
        require(token in candidate, f"exact-candidate authored trailer evidence contract missing: {token}")

    forbidden_importer = (
        "SetSimulatePhysics",
        "AddForce",
        "AddTorque",
        "SetActorLocation",
        "Economy",
        "Cash",
        "Wanted",
        "Roadmap truth",
    )
    for token in forbidden_importer:
        require(token not in importer, f"editor import script must not own gameplay/release authority: {token}")

    require(roadmap.count("- [ ] ") == 5, "roadmap blocker count changed without runtime evidence")
    require("125" in roadmap and "130" in roadmap and "96.2%" in roadmap,
            "roadmap truth drifted while adding source/editor import automation")
    require("Authored skeletal trailer wheel assets and final hitch sockets" in roadmap,
            "authored trailer acceptance checkbox disappeared")
    print("GTT 0.1.60 authored trailer UE import automation source contract: PASS")
    print("Editor evidence is fail-closed and exact-candidate-bound; packaged Win64/runtime/visual gates remain open.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
