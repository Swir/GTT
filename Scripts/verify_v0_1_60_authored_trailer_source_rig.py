#!/usr/bin/env python3
"""Source-contract verifier for a generated GTT 0.1.60 authored trailer rig."""
from __future__ import annotations
import argparse
import base64
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "Scripts" / "generate_gtt_farm_trailer_gltf.py"
CONTRACT = "gtt.farm-trailer-source-rig.v2"
REQUIRED_JOINTS = ("body", "wheel_l", "wheel_r")
REQUIRED_SOCKETS = ("socket_hitch", "socket_cargo", "socket_axle_l", "socket_axle_r")
SOCKET_NODE_NAMES = {name: f"SOCKET_{name}" for name in REQUIRED_SOCKETS}

def require(cond: bool, message: str) -> None:
    if not cond:
        raise AssertionError(message)

def accessor_bounds(doc, index):
    a=doc["accessors"][index]
    return a.get("min"), a.get("max")

def verify(asset: Path) -> None:
    require(GENERATOR.is_file(), f"missing generator: {GENERATOR}")
    require(asset.is_file(), f"missing generated authored rig: {asset}")
    text=asset.read_text(encoding="utf-8")
    doc=json.loads(text)
    require(doc["asset"]["version"]=="2.0","asset must be glTF 2.0")
    require(doc["asset"]["extras"]["gtt_asset_contract"]==CONTRACT,"wrong asset contract")
    require(doc["extras"]["gtt_asset_contract"]==CONTRACT,"root contract marker missing")
    require("no third-party protected game assets" in doc["extras"]["source"],"originality marker missing")
    require(doc["extras"]["acceptance"].startswith("source-rig candidate only"),"runtime-acceptance disclaimer missing")
    require(doc["extras"]["interchange_socket_nodes"]==SOCKET_NODE_NAMES,"root Interchange socket map drift")

    nodes=doc["nodes"]
    names=[n.get("name") for n in nodes]
    for name in REQUIRED_JOINTS:
        require(name in names, f"required rig joint missing: {name}")
    for logical_name, source_name in SOCKET_NODE_NAMES.items():
        require(source_name in names, f"Interchange socket node missing: {source_name}")
        node=nodes[names.index(source_name)]
        require(node.get("extras",{}).get("gtt_socket_name")==logical_name,
                f"logical socket mapping drift: {source_name}")
    name_to_index={n.get("name"):i for i,n in enumerate(nodes)}
    body_children=tuple(names[i] for i in nodes[name_to_index["body"]].get("children", []))
    require(body_children == ("wheel_l","wheel_r","SOCKET_socket_hitch","SOCKET_socket_cargo","SOCKET_socket_axle_l","SOCKET_socket_axle_r"),
            f"skeletal/socket hierarchy drift: {body_children}")
    skin=doc["skins"][0]
    joint_names=tuple(names[i] for i in skin["joints"])
    require(joint_names==REQUIRED_JOINTS, f"unexpected joint order: {joint_names}")
    require(tuple(skin["extras"]["required_joints"])==REQUIRED_JOINTS,"skin joint contract drift")
    require(tuple(skin["extras"]["required_sockets"])==REQUIRED_SOCKETS,"skin socket contract drift")
    require(skin["extras"]["interchange_socket_nodes"]==SOCKET_NODE_NAMES,"skin Interchange socket map drift")

    expected_locations={
        "wheel_l": (0.72,-1.42,-0.42), "wheel_r": (0.72,1.42,-0.42),
        "SOCKET_socket_hitch": (-4.90,0.0,0.12), "SOCKET_socket_cargo": (0.0,0.0,1.05),
        "SOCKET_socket_axle_l": (0.72,-1.42,-0.42), "SOCKET_socket_axle_r": (0.72,1.42,-0.42),
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

    # A second process must reproduce the generated candidate byte-for-byte.
    with tempfile.TemporaryDirectory() as td:
        regenerated=Path(td)/"rig.gltf"
        subprocess.run([sys.executable,str(GENERATOR),"--output",str(regenerated)],check=True,cwd=ROOT)
        require(regenerated.read_bytes()==asset.read_bytes(),"generated rig is non-deterministic on this runner")
    digest=hashlib.sha256(asset.read_bytes()).hexdigest()
    print(f"GTT 0.1.60 authored trailer source rig: PASS sha256={digest} bytes={asset.stat().st_size}")
    print("Interchange SOCKET_ anchors validated; final UE mesh sockets remain a real editor/import gate.")
    print("Generated source asset is validated only; UE import + PhysicsAsset + packaged Win64 runtime/visual acceptance remain open.")

def main() -> int:
    ap=argparse.ArgumentParser()
    ap.add_argument("--asset", required=True, help="Generated glTF candidate to validate")
    args=ap.parse_args()
    verify(Path(args.asset))
    return 0

if __name__=="__main__":
    raise SystemExit(main())
