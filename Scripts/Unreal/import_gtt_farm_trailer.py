#!/usr/bin/env python3
"""UE 5.8 editor-side authored farm-trailer import gate.

Run only inside UnrealEditor-Cmd with PythonScriptPlugin enabled. This script imports the
deterministically generated project-owned glTF through Interchange, normalizes final mesh
socket names, creates/assigns a PhysicsAsset when needed, validates the exact runtime
contract and saves only after all checks pass.

It intentionally does not mark roadmap/demo gates complete. Packaged Win64 runtime and
rendered visual acceptance remain separate evidence.
"""
from __future__ import annotations

import hashlib
import json
import os
import re
from pathlib import Path

import unreal

DESTINATION = "/Game/GTT/Vehicles/Trailer"
ASSET_NAME = "SK_GTT_FarmTrailer"
ASSET_PATH = f"{DESTINATION}/{ASSET_NAME}"
SOURCE_RELATIVE = Path("Intermediate/GTT/AuthoredTrailer/GTT_FarmTrailer_Rig.gltf")
EVIDENCE_RELATIVE = Path(
    "Intermediate/GTT/AuthoredTrailer/AUTHORED_TRAILER_EDITOR_ACCEPTANCE.json"
)

REQUIRED_BONES = ("body", "wheel_l", "wheel_r")
REQUIRED_SOCKETS = ("socket_hitch", "socket_cargo", "socket_axle_l", "socket_axle_r")
SOURCE_SOCKET_NAMES = {name: f"SOCKET_{name}" for name in REQUIRED_SOCKETS}


def fail(reason: str) -> None:
    unreal.log_error(f"AUTHORED_TRAILER_IMPORT result=FAIL reason={reason}")
    raise RuntimeError(reason)


def project_root() -> Path:
    return Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()


def project_source() -> str:
    return os.fspath((project_root() / SOURCE_RELATIVE).resolve())


def evidence_path() -> Path:
    return (project_root() / EVIDENCE_RELATIVE).resolve()


def configure_pipelines(source_data):
    pipelines = list(
        unreal.InterchangeProjectSettingsScript.get_pipeline_stack_from_source_data(False, source_data)
    )
    generic = next(
        (p for p in pipelines if isinstance(p, unreal.InterchangeGenericAssetsPipeline)),
        None,
    )
    if generic is None:
        fail("GENERIC_ASSET_PIPELINE_MISSING")

    generic.set_editor_property("asset_name", ASSET_NAME)
    generic.set_editor_property("use_source_name_for_asset", False)
    generic.set_editor_property("asset_type_sub_folders", False)

    mesh_pipeline = generic.get_editor_property("mesh_pipeline")
    mesh_pipeline.set_editor_property("import_skeletal_meshes", True)
    mesh_pipeline.set_editor_property("import_static_meshes", False)
    mesh_pipeline.set_editor_property("combine_skeletal_meshes", True)
    mesh_pipeline.set_editor_property("create_physics_asset", True)

    common_meshes = generic.get_editor_property("common_meshes_properties")
    common_meshes.set_editor_property("import_sockets", True)

    common_skeletal = generic.get_editor_property(
        "common_skeletal_meshes_and_animations_properties"
    )
    common_skeletal.set_editor_property("import_only_animations", False)
    common_skeletal.set_editor_property("import_meshes_in_bone_hierarchy", False)
    return pipelines


def socket_names(mesh) -> set[str]:
    result: set[str] = set()
    for index in range(mesh.num_sockets()):
        socket = mesh.get_socket_by_index(index)
        if socket:
            result.add(str(socket.get_editor_property("socket_name")))
    return result


def normalize_sockets(mesh) -> None:
    existing = socket_names(mesh)
    for desired in REQUIRED_SOCKETS:
        if desired in existing:
            continue
        prefixed = SOURCE_SOCKET_NAMES[desired]
        if prefixed not in existing:
            fail(f"SOCKET_MISSING_{desired}")
        if not mesh.rename_socket(unreal.Name(prefixed), unreal.Name(desired)):
            fail(f"SOCKET_RENAME_FAILED_{desired}")
        existing.remove(prefixed)
        existing.add(desired)


def validate_bones(mesh) -> tuple[str, ...]:
    body_children = {str(name) for name in mesh.get_bone_children(unreal.Name("body"))}
    verified = ["body"]
    for wheel in ("wheel_l", "wheel_r"):
        if wheel not in body_children:
            parent = str(mesh.get_bone_parent(unreal.Name(wheel)))
            if parent != "body":
                fail(f"BONE_HIERARCHY_{wheel}_PARENT_{parent or 'NONE'}")
        verified.append(wheel)
    return tuple(verified)


def ensure_physics_asset(mesh):
    existing = mesh.get_editor_property("physics_asset")
    if existing is not None:
        return existing
    created = unreal.SkeletalMeshEditorSubsystem.create_physics_asset(mesh, True, 0)
    physics_asset = mesh.get_editor_property("physics_asset")
    if created is None or physics_asset is None:
        fail("PHYSICS_ASSET_CREATE_FAILED")
    return physics_asset


def candidate_git_sha() -> str | None:
    value = os.environ.get("GITHUB_SHA", "").strip().lower()
    if not value:
        return None
    if re.fullmatch(r"[0-9a-f]{40}", value) is None:
        fail("GITHUB_SHA_INVALID")
    return value


def write_editor_acceptance(
    source_path: str,
    mesh,
    skeleton,
    physics_asset,
    verified_bones: tuple[str, ...],
    verified_sockets: set[str],
) -> Path:
    source_hash = hashlib.sha256(Path(source_path).read_bytes()).hexdigest()
    output = evidence_path()
    output.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "git_sha": candidate_git_sha(),
        "physics_asset_object_path": physics_asset.get_path_name(),
        "required_bones": sorted(REQUIRED_BONES),
        "required_sockets": sorted(REQUIRED_SOCKETS),
        "result": "PASS",
        "schema": "gtt.authored-trailer-editor-acceptance.v1",
        "skeletal_mesh_object_path": mesh.get_path_name(),
        "skeleton_object_path": skeleton.get_path_name(),
        "source_gltf_sha256": source_hash,
        "verified_bones": sorted(verified_bones),
        "verified_sockets": sorted(verified_sockets),
    }
    temporary = output.with_suffix(output.suffix + ".tmp")
    temporary.write_text(
        json.dumps(payload, indent=2, sort_keys=True, separators=(",", ": ")) + "\n",
        encoding="utf-8",
    )
    temporary.replace(output)
    return output


def main() -> None:
    source_path = project_source()
    if not Path(source_path).is_file():
        fail("SOURCE_GLTF_MISSING")

    output = evidence_path()
    if output.exists():
        output.unlink()

    manager = unreal.InterchangeManager.get_interchange_manager_scripted()
    source_data = unreal.InterchangeManager.create_source_data(source_path)
    if source_data is None or not manager.can_translate_source_data(source_data, False):
        fail("INTERCHANGE_TRANSLATOR_UNAVAILABLE")

    pipelines = configure_pipelines(source_data)
    params = unreal.ImportAssetParameters(
        is_automated=True,
        override_pipelines=pipelines,
        destination_name=ASSET_NAME,
        replace_existing=True,
    )
    imported = manager.import_asset(DESTINATION, source_data, params)
    if not imported:
        fail("INTERCHANGE_IMPORT_FAILED")

    mesh = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if mesh is None or not isinstance(mesh, unreal.SkeletalMesh):
        fail("SKELETAL_MESH_NOT_CREATED")

    verified_bones = validate_bones(mesh)
    normalize_sockets(mesh)
    physics_asset = ensure_physics_asset(mesh)
    skeleton = mesh.get_editor_property("skeleton")
    if skeleton is None:
        fail("SKELETON_UNASSIGNED")

    final_sockets = socket_names(mesh)
    missing = sorted(set(REQUIRED_SOCKETS) - final_sockets)
    if missing:
        fail("FINAL_SOCKET_SET_" + "_".join(missing))
    if physics_asset is None:
        fail("PHYSICS_ASSET_UNASSIGNED")

    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, False):
        fail("SKELETAL_MESH_SAVE_FAILED")
    if not unreal.EditorAssetLibrary.save_loaded_asset(physics_asset, False):
        fail("PHYSICS_ASSET_SAVE_FAILED")

    evidence = write_editor_acceptance(
        source_path,
        mesh,
        skeleton,
        physics_asset,
        verified_bones,
        final_sockets,
    )

    unreal.log(
        "AUTHORED_TRAILER_IMPORT result=PASS "
        f"asset={ASSET_PATH} bones={','.join(REQUIRED_BONES)} "
        f"sockets={','.join(REQUIRED_SOCKETS)} physicsAsset=1 "
        f"evidence={evidence.as_posix()}"
    )


if __name__ == "__main__":
    main()
