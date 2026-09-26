#!/usr/bin/env python3
"""Import and validate the three project-owned Native Chaos vehicle rigs in UE 5.8."""
from __future__ import annotations

import json
import os
import re
from pathlib import Path
import unreal

VEHICLES = {
    "Fieldmaster60": {"destination":"/Game/GTT/Vehicles/Fieldmaster","hitch":True},
    "Rattleback82": {"destination":"/Game/GTT/Vehicles/Rattleback","hitch":False},
    "Mulebox1200": {"destination":"/Game/GTT/Vehicles/Mulebox","hitch":True},
}
REQUIRED_BONES=("root","wheel_fl","wheel_fr","wheel_rl","wheel_rr")


def fail(reason):
    unreal.log_error(f"NATIVE_VEHICLE_RIG_IMPORT result=FAIL reason={reason}")
    raise RuntimeError(reason)


def root(): return Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()


def pipelines(source_data, asset_name):
    values=list(unreal.InterchangeProjectSettingsScript.get_pipeline_stack_from_source_data(False,source_data))
    generic=next((p for p in values if isinstance(p,unreal.InterchangeGenericAssetsPipeline)),None)
    if generic is None: fail("GENERIC_ASSET_PIPELINE_MISSING")
    generic.set_editor_property("asset_name",asset_name); generic.set_editor_property("use_source_name_for_asset",False)
    generic.set_editor_property("asset_type_sub_folders",False)
    mesh=generic.get_editor_property("mesh_pipeline"); mesh.set_editor_property("import_skeletal_meshes",True)
    mesh.set_editor_property("import_static_meshes",False); mesh.set_editor_property("create_physics_asset",True)
    mesh.set_editor_property("combine_skeletal_meshes_behavior",unreal.InterchangeCombineSkeletalMeshesBehavior.BY_SKELETON)
    common=generic.get_editor_property("common_meshes_properties"); common.set_editor_property("import_sockets",True)
    common.set_editor_property("convert_statics_in_bone_hierarchy_to_skeletals",False)
    generic.get_editor_property("common_skeletal_meshes_and_animations_properties").set_editor_property("import_only_animations",False)
    return values


def socket_names(mesh):
    return {str(mesh.get_socket_by_index(i).get_editor_property("socket_name")) for i in range(mesh.num_sockets()) if mesh.get_socket_by_index(i)}


def import_vehicle(name, spec):
    source=root()/"Intermediate"/"GTT"/"NativeVehicles"/f"GTT_{name}_Rig.gltf"
    if not source.is_file(): fail(f"SOURCE_MISSING_{name}")
    destination=spec["destination"]; asset_name=f"SK_GTT_{name}"; asset_path=f"{destination}/{asset_name}"
    manager=unreal.InterchangeManager.get_interchange_manager_scripted()
    source_data=unreal.InterchangeManager.create_source_data(os.fspath(source))
    configured=pipelines(source_data,asset_name)
    params=unreal.ImportAssetParameters(is_automated=True,
        override_pipelines=[unreal.SoftObjectPath(p.get_path_name()) for p in configured],destination_name=asset_name,replace_existing=True)
    if not manager.import_asset(destination,source_data,params): fail(f"IMPORT_FAILED_{name}")
    mesh=unreal.EditorAssetLibrary.load_asset(asset_path) if unreal.EditorAssetLibrary.does_asset_exist(asset_path) else None
    if not isinstance(mesh,unreal.SkeletalMesh):
        candidates=[]
        for candidate_path in unreal.EditorAssetLibrary.list_assets(destination,recursive=True,include_folder=False):
            candidate=unreal.EditorAssetLibrary.load_asset(candidate_path)
            if isinstance(candidate,unreal.SkeletalMesh): candidates.append(candidate)
        if len(candidates)!=1: fail(f"SKELETAL_MESH_COUNT_{name}_{len(candidates)}")
        mesh=candidates[0]
        if mesh.get_path_name()!=f"{asset_path}.{asset_name}":
            if not unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name(),asset_path): fail(f"SKELETAL_MESH_RENAME_FAILED_{name}")
    for bone in REQUIRED_BONES:
        if str(mesh.get_bone_parent(unreal.Name(bone))) == "None" and bone != "root": fail(f"BONE_MISSING_{name}_{bone}")
    desired=["driver_seat","driver_exit"] + (["rear_hitch"] if spec["hitch"] else [])
    existing=socket_names(mesh)
    for socket in desired:
        if socket in existing: continue
        prefixed=f"SOCKET_{socket}"
        if prefixed not in existing or not mesh.rename_socket(unreal.Name(prefixed),unreal.Name(socket)):
            fail(f"SOCKET_MISSING_{name}_{socket}")
        existing.remove(prefixed); existing.add(socket)
    physics=mesh.get_editor_property("physics_asset")
    if physics is None:
        unreal.SkeletalMeshEditorSubsystem.create_physics_asset(mesh,True,0)
        physics=mesh.get_editor_property("physics_asset")
    if physics is None: fail(f"PHYSICS_ASSET_MISSING_{name}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh,False): fail(f"SAVE_FAILED_{name}")
    unreal.EditorAssetLibrary.save_loaded_asset(physics,False)
    return {"vehicle":name,"skeletal_mesh":mesh.get_path_name(),"physics_asset":physics.get_path_name(),"bones":list(REQUIRED_BONES),"sockets":desired}


def main():
    sha=os.environ.get("GITHUB_SHA","").strip().lower()
    if sha and re.fullmatch(r"[0-9a-f]{40}",sha) is None: fail("GITHUB_SHA_INVALID")
    assets=[import_vehicle(name,spec) for name,spec in VEHICLES.items()]
    evidence=root()/"Intermediate"/"GTT"/"NativeVehicles"/"NATIVE_VEHICLE_RIG_EDITOR_ACCEPTANCE.json"
    evidence.parent.mkdir(parents=True,exist_ok=True)
    evidence.write_text(json.dumps({"schema":"gtt.native-vehicle-rig-editor-acceptance.v1","result":"PASS","git_sha":sha or None,"assets":assets},indent=2,sort_keys=True)+"\n",encoding="utf-8")
    unreal.log(f"NATIVE_VEHICLE_RIG_IMPORT result=PASS assets={len(assets)} evidence={evidence.as_posix()}")


if __name__ == "__main__": main()
