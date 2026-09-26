#!/usr/bin/env python3
"""Generate deterministic, project-owned skeletal glTF rigs for the Native Chaos fleet."""
from __future__ import annotations

import argparse
import base64
import json
import math
import struct
from pathlib import Path

VEHICLES = {
    "Fieldmaster60": {"size": (4.8, 2.25, 1.75), "wheelbase": 2.75, "track": 1.72, "wheel": 0.62, "hitch": True, "color": (.22, .34, .10, 1)},
    "Rattleback82": {"size": (4.45, 1.86, 1.35), "wheelbase": 2.62, "track": 1.55, "wheel": 0.36, "hitch": False, "color": (.38, .08, .05, 1)},
    "Mulebox1200": {"size": (5.15, 2.05, 2.15), "wheelbase": 3.15, "track": 1.68, "wheel": 0.43, "hitch": True, "color": (.12, .20, .32, 1)},
}
JOINTS = ("root", "wheel_fl", "wheel_fr", "wheel_rl", "wheel_rr")


class Buffer:
    def __init__(self): self.data=bytearray(); self.views=[]; self.accessors=[]
    def add(self, raw, target=None):
        while len(self.data)%4: self.data.append(0)
        offset=len(self.data); self.data.extend(raw)
        view={"buffer":0,"byteOffset":offset,"byteLength":len(raw)}
        if target: view["target"]=target
        self.views.append(view); return len(self.views)-1
    def accessor(self, raw, component, kind, count, target=None, minimum=None, maximum=None, normalized=False):
        value={"bufferView":self.add(raw,target),"componentType":component,"count":count,"type":kind}
        if minimum is not None: value["min"]=minimum
        if maximum is not None: value["max"]=maximum
        if normalized: value["normalized"]=True
        self.accessors.append(value); return len(self.accessors)-1


def floats(values): return struct.pack(f"<{len(values)}f", *values)
def ushorts(values): return struct.pack(f"<{len(values)}H", *values)


def add_box(vertices, joints, indices, center, size, joint):
    cx,cy,cz=center; hx,hy,hz=(v/2 for v in size)
    corners=[(cx-hx,cy-hy,cz-hz),(cx+hx,cy-hy,cz-hz),(cx+hx,cy+hy,cz-hz),(cx-hx,cy+hy,cz-hz),
             (cx-hx,cy-hy,cz+hz),(cx+hx,cy-hy,cz+hz),(cx+hx,cy+hy,cz+hz),(cx-hx,cy+hy,cz+hz)]
    base=len(vertices); vertices.extend(corners); joints.extend([joint]*8)
    faces=(0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,1,2,6,1,6,5,2,3,7,2,7,6,3,0,4,3,4,7)
    indices.extend(base+i for i in faces)


def add_wheel(vertices, joints, indices, center, radius, width, joint, segments=12):
    cx,cy,cz=center; base=len(vertices)
    for y in (cy-width/2, cy+width/2):
        for i in range(segments):
            angle=2*math.pi*i/segments
            vertices.append((cx+radius*math.cos(angle),y,cz+radius*math.sin(angle))); joints.append(joint)
    for i in range(segments):
        nxt=(i+1)%segments; a=base+i; b=base+nxt; c=base+segments+nxt; d=base+segments+i
        indices.extend((a,b,c,a,c,d))


def primitive(buffer, vertices, joints, indices, material):
    pos=[value for vertex in vertices for value in vertex]
    pos_accessor=buffer.accessor(floats(pos),5126,"VEC3",len(vertices),34962,
        [min(v[i] for v in vertices) for i in range(3)],[max(v[i] for v in vertices) for i in range(3)])
    joint_data=[]; weights=[]
    for joint in joints: joint_data.extend((joint,0,0,0)); weights.extend((255,0,0,0))
    joint_accessor=buffer.accessor(bytes(joint_data),5121,"VEC4",len(vertices),34962)
    weight_accessor=buffer.accessor(bytes(weights),5121,"VEC4",len(vertices),34962,normalized=True)
    index_accessor=buffer.accessor(ushorts(indices),5123,"SCALAR",len(indices),34963,[min(indices)],[max(indices)])
    return {"attributes":{"POSITION":pos_accessor,"JOINTS_0":joint_accessor,"WEIGHTS_0":weight_accessor},"indices":index_accessor,"material":material,"mode":4}


def inverse_translation(x,y,z): return [1,0,0,0,0,1,0,0,0,0,1,0,-x,-y,-z,1]


def generate(name, spec):
    length,width,height=spec["size"]; wheelbase=spec["wheelbase"]; track=spec["track"]; radius=spec["wheel"]
    front_x=wheelbase/2; rear_x=-wheelbase/2; side=track/2; wheel_z=radius
    positions=((front_x,-side,wheel_z),(front_x,side,wheel_z),(rear_x,-side,wheel_z),(rear_x,side,wheel_z))
    buffer=Buffer(); body_v=[]; body_j=[]; body_i=[]
    add_box(body_v,body_j,body_i,(0,0,radius+height/2),(length,width,height),0)
    add_box(body_v,body_j,body_i,(length*.12,0,radius+height*.90),(length*.38,width*.86,height*.42),0)
    primitives=[primitive(buffer,body_v,body_j,body_i,0)]
    for index,position in enumerate(positions,1):
        vertices=[]; joints=[]; indices=[]; add_wheel(vertices,joints,indices,position,radius,radius*.46,index)
        primitives.append(primitive(buffer,vertices,joints,indices,1))
    nodes=[{"name":f"GTT_{name}_Rig","children":[1],"mesh":0,"skin":0},
           {"name":"root","children":[2,3,4,5,6,7] + ([8] if spec["hitch"] else [])}]
    for bone,position in zip(JOINTS[1:],positions): nodes.append({"name":bone,"translation":list(position)})
    nodes.append({"name":"SOCKET_driver_seat","translation":[0,0,radius+height*.72]})
    nodes.append({"name":"SOCKET_driver_exit","translation":[0,width*.72,radius+height*.45]})
    if spec["hitch"]: nodes.append({"name":"SOCKET_rear_hitch","translation":[-length*.56,0,radius*.75]})
    matrices=[]
    for position in ((0,0,0),)+positions: matrices.extend(inverse_translation(*position))
    ibm=buffer.accessor(floats(matrices),5126,"MAT4",5)
    encoded=base64.b64encode(bytes(buffer.data)).decode("ascii")
    return {"asset":{"version":"2.0","generator":"GTT Native Chaos fleet rig generator"},"scene":0,"scenes":[{"nodes":[0]}],
      "nodes":nodes,"skins":[{"name":f"GTT_{name}_Skeleton","joints":[1,2,3,4,5],"skeleton":1,"inverseBindMatrices":ibm}],
      "meshes":[{"name":f"SK_GTT_{name}","primitives":primitives}],
      "materials":[{"name":"GTT_Body","pbrMetallicRoughness":{"baseColorFactor":spec["color"],"metallicFactor":.32,"roughnessFactor":.58}},
                   {"name":"GTT_Tire","pbrMetallicRoughness":{"baseColorFactor":[.02,.02,.025,1],"metallicFactor":0,"roughnessFactor":.92}}],
      "buffers":[{"byteLength":len(buffer.data),"uri":"data:application/octet-stream;base64,"+encoded}],"bufferViews":buffer.views,"accessors":buffer.accessors}


def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--output-dir",required=True); args=parser.parse_args()
    destination=Path(args.output_dir); destination.mkdir(parents=True,exist_ok=True)
    for name,spec in VEHICLES.items():
        path=destination/f"GTT_{name}_Rig.gltf"
        path.write_text(json.dumps(generate(name,spec),sort_keys=True,separators=(",",":"))+"\n",encoding="utf-8")
        print(path)


if __name__ == "__main__": main()
