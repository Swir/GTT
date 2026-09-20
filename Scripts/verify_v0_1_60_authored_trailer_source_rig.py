#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.60 authored trailer source rig."""
from __future__ import annotations
import base64
import hashlib
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "Scripts" / "generate_gtt_farm_trailer_gltf.py"
ASSET = ROOT / "SourceArt" / "Trailer" / "GTT_FarmTrailer_Rig.gltf"
CONTRACT = "gtt.farm-trailer-source-rig.v1"
REQUIRED_JOINTS = ("body", "wheel_l", "wheel_r")
REQUIRED_SOCKETS = ("socket_hitch", "socket_cargo", "socket_axle_l", "socket_axle_r")

def require(cond: bool, message: str) -> None:
    if not cond:
        raise AssertionError(message)

def accessor_bounds(doc, index):
    a=doc["accessors"][index]
    return a.get("min"), a.get("max")

def main() -> int:
    require(GENERATOR.is_file(), f"missing generator: {GENERATOR}")
    require(ASSET.is_file(), f"missing authored source rig: {ASSET}")
    text=ASSET.read_text(encoding="utf-8")
    doc=json.loads(text)
    require(doc["asset"]["version"]=="2.0","asset must be glTF 2.0")
    require(doc["asset"]["extras"]["gtt_asset_contract"]==CONTRACT,"wrong asset contract")
    require(doc["extras"]["gtt_asset_contract"]==CONTRACT,"root contract marker missing")
    require("no third-party protected game assets" in doc["extras"]["source"],"originality marker missing")
    require(doc["extras"]["acceptance"].startswith("source-rig candidate only"),"runtime-acceptance disclaimer missing")

    nodes=doc["nodes"]
    names=[n.get("name") for n in nodes]
    for name in REQUIRED_JOINTS + REQUIRED_SOCKETS:
        require(name in names, f"required rig node missing: {name}")
    name_to_index={n.get("name"):i for i,n in enumerate(nodes)}
    skin=doc["skins"][0]
    joint_names=tuple(names[i] for i in skin["joints"])
    require(joint_names==REQUIRED_JOINTS, f"unexpected joint order: {joint_names}")
    require(tuple(skin["extras"]["required_joints"])==REQUIRED_JOINTS,"skin joint contract drift")
    require(tuple(skin["extras"]["required_sockets"])==REQUIRED_SOCKETS,"skin socket contract drift")

    expected_locations={
        "wheel_l": (0.72,-1.42,-0.42),
        "wheel_r": (0.72,1.42,-0.42),
        "socket_hitch": (-4.90,0.0,0.12),
        "socket_cargo": (0.0,0.0,1.05),
        "socket_axle_l": (0.72,-1.42,-0.42),
        "socket_axle_r": (0.72,1.42,-0.42),
    }
    for name, expected in expected_locations.items():
        actual=tuple(nodes[name_to_index[name]].get("translation",(0,0,0)))
        require(all(abs(a-b)<1e-6 for a,b in zip(actual,expected)), f"{name} location drift: {actual}")

    require(len(doc["meshes"])==1 and len(doc["meshes"][0]["primitives"])==3,"expected body + two wheel primitives")
    require([m["name"] for m in doc["materials"]]==["GTT_TrailerPaint","GTT_Tire"],"material contract drift")
    raw_uri=doc["buffers"][0]["uri"]
    require(raw_uri.startswith("data:application/octet-stream;base64,"),"asset must be self-contained")
    raw=base64.b64decode(raw_uri.split(",",1)[1], validate=True)
    require(len(raw)==doc["buffers"][0]["byteLength"],"embedded buffer length mismatch")
    for view in doc["bufferViews"]:
        require(view.get("buffer")==0,"unexpected extra buffer")
        require(view.get("byteOffset",0)+view["byteLength"]<=len(raw),"buffer view escapes embedded buffer")
    for prim in doc["meshes"][0]["primitives"]:
        attrs=prim["attributes"]
        for semantic in ("POSITION","JOINTS_0","WEIGHTS_0"):
            require(semantic in attrs, f"missing skinned attribute: {semantic}")
        require(prim["mode"]==4,"primitive must use triangles")
    body_min,body_max=accessor_bounds(doc,doc["meshes"][0]["primitives"][0]["attributes"]["POSITION"])
    require(body_min[0] <= -4.8 and body_max[0] >= 2.6,"body/drawbar length envelope regressed")
    require(body_min[1] <= -1.25 and body_max[1] >= 1.25,"body width envelope regressed")
    require(body_max[2] >= 1.5,"rail height envelope regressed")

    # Source generation must be byte-for-byte deterministic.
    with tempfile.TemporaryDirectory() as td:
        generated=Path(td)/"rig.gltf"
        subprocess.run([sys.executable,str(GENERATOR),"--output",str(generated)],check=True,cwd=ROOT)
        require(generated.read_bytes()==ASSET.read_bytes(),"generated source rig is stale/non-deterministic")
    digest=hashlib.sha256(ASSET.read_bytes()).hexdigest()
    print(f"GTT 0.1.60 authored trailer source rig: PASS sha256={digest} bytes={ASSET.stat().st_size}")
    print("Source asset is validated only; Unreal import + PhysicsAsset + packaged Win64 runtime/visual acceptance remain open.")
    return 0

if __name__=="__main__":
    raise SystemExit(main())
