#!/usr/bin/env python3
"""Deterministically generate the original GTT farm-trailer source rig as self-contained glTF 2.0.

Project-owned source art only. Unreal import, PhysicsAsset/socket wiring and packaged Win64
runtime/visual acceptance are deliberately separate release gates.
"""
from __future__ import annotations
import argparse, base64, hashlib, json, math, struct
from pathlib import Path

ASSET_VERSION="gtt.farm-trailer-source-rig.v2"
REQUIRED_JOINTS=["body","wheel_l","wheel_r"]
REQUIRED_SOCKETS=["socket_hitch","socket_cargo","socket_axle_l","socket_axle_r"]
SOCKET_NODE_NAMES={name:f"SOCKET_{name}" for name in REQUIRED_SOCKETS}

class B:
    def __init__(self): self.buf=bytearray(); self.views=[]; self.acc=[]
    def add(self, raw, target=None):
        while len(self.buf)%4: self.buf.append(0)
        o=len(self.buf); self.buf.extend(raw)
        v={"buffer":0,"byteOffset":o,"byteLength":len(raw)}
        if target: v["target"]=target
        self.views.append(v); return len(self.views)-1
    def accessor(self,raw,ctype,typ,count,target=None,mi=None,ma=None,normalized=False):
        a={"bufferView":self.add(raw,target),"componentType":ctype,"count":count,"type":typ}
        if mi is not None:a["min"]=mi
        if ma is not None:a["max"]=ma
        if normalized:a["normalized"]=True
        self.acc.append(a); return len(self.acc)-1

def pf(v): return struct.pack("<%sf"%len(v),*v)
def pu8(v): return bytes(v)
def pu16(v): return struct.pack("<%sH"%len(v),*v)
def q6(x): return round(float(x), 6)

def box(v,j,idx,c,s,joint):
    cx,cy,cz=c; hx,hy,hz=(q/2 for q in s)
    corners=[(cx-hx,cy-hy,cz-hz),(cx+hx,cy-hy,cz-hz),(cx+hx,cy+hy,cz-hz),(cx-hx,cy+hy,cz-hz),
             (cx-hx,cy-hy,cz+hz),(cx+hx,cy-hy,cz+hz),(cx+hx,cy+hy,cz+hz),(cx-hx,cy+hy,cz+hz)]
    base=len(v); v.extend(corners); j.extend([joint]*8)
    faces=[0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,1,2,6,1,6,5,2,3,7,2,7,6,3,0,4,3,4,7]
    idx.extend(base+x for x in faces)

def wheel(v,j,idx,c,r,w,joint,segments=10):
    cx,cy,cz=c; y0=cy-w/2; y1=cy+w/2
    base=len(v)
    for y in (y0,y1):
        for i in range(segments):
            a=2*math.pi*i/segments
            v.append((q6(cx+r*math.cos(a)),q6(y),q6(cz+r*math.sin(a)))); j.append(joint)
    for i in range(segments):
        ni=(i+1)%segments
        a=base+i; b=base+ni; c1=base+segments+ni; d=base+segments+i
        idx += [a,b,c1,a,c1,d]
    # fan caps
    for y,off,rev in ((y0,0,True),(y1,segments,False)):
        center=len(v); v.append((q6(cx),q6(y),q6(cz))); j.append(joint)
        for i in range(segments):
            a=base+off+i; b=base+off+((i+1)%segments)
            idx += ([center,b,a] if rev else [center,a,b])

def primitive(builder, parts, material):
    v=[]; joints=[]; idx=[]
    for fn,args in parts: fn(v,joints,idx,*args)
    flatp=[x for q in v for x in q]
    pos=builder.accessor(pf(flatp),5126,"VEC3",len(v),34962,
                         [min(q[k] for q in v) for k in range(3)],
                         [max(q[k] for q in v) for k in range(3)])
    joint4=[]; weight4=[]
    for q in joints: joint4 += [q,0,0,0]; weight4 += [255,0,0,0]
    ja=builder.accessor(pu8(joint4),5121,"VEC4",len(v),34962)
    wa=builder.accessor(pu8(weight4),5121,"VEC4",len(v),34962,normalized=True)
    ia=builder.accessor(pu16(idx),5123,"SCALAR",len(idx),34963,[min(idx)],[max(idx)])
    return {"attributes":{"POSITION":pos,"JOINTS_0":ja,"WEIGHTS_0":wa},"indices":ia,"mode":4,"material":material}

def inv_t(x,y,z): return [1,0,0,0,0,1,0,0,0,0,1,0,-x,-y,-z,1]

def socket_node(logical_name, translation):
    return {
        "name": SOCKET_NODE_NAMES[logical_name],
        "translation": translation,
        "extras": {"gtt_socket": True, "gtt_socket_name": logical_name},
    }

def generate():
    b=B()
    body=[]
    for c,s in [
      ((0,0,.32),(5.35,2.35,.34)), ((0,0,-.02),(4.65,.24,.26)),
      ((-3.55,-.62,.12),(2.45,.18,.16)),((-3.55,.62,.12),(2.45,.18,.16)),
      ((-4.72,0,.12),(.36,.50,.22)),((-2.55,0,1.02),(.16,2.25,1.20)),
      ((0,-1.12,.88),(5.0,.12,.88)),((0,1.12,.88),(5.0,.12,.88)),
      ((2.55,0,.88),(.14,2.22,.88)),((.70,-1.28,.16),(1.55,.10,.20)),
      ((.70,1.28,.16),(1.55,.10,.20)),((2.68,0,.35),(.10,1.95,.13))
    ]: body.append((box,(c,s,0)))
    left=[(wheel,((.72,-1.42,-.42),.64,.34,1,10))]
    right=[(wheel,((.72,1.42,-.42),.64,.34,2,10))]
    prims=[primitive(b,body,0),primitive(b,left,1),primitive(b,right,1)]
    ibm=inv_t(0,0,0)+inv_t(.72,-1.42,-.42)+inv_t(.72,1.42,-.42)
    ibma=b.accessor(pf(ibm),5126,"MAT4",3)
    uri="data:application/octet-stream;base64,"+base64.b64encode(bytes(b.buf)).decode()
    return {
      "asset":{"version":"2.0","generator":"GTT deterministic trailer rig generator",
               "extras":{"license":"Project-owned original source art","gtt_asset_contract":ASSET_VERSION}},
      "scene":0,"scenes":[{"nodes":[0]}],
      "nodes":[
        {"name":"GTT_FarmTrailer_Rig","children":[1],"mesh":0,"skin":0},
        {"name":"body","children":[2,3,4,5,6,7]},{"name":"wheel_l","translation":[.72,-1.42,-.42]},
        {"name":"wheel_r","translation":[.72,1.42,-.42]},
        socket_node("socket_hitch",[-4.90,0,.12]),
        socket_node("socket_cargo",[0,0,1.05]),
        socket_node("socket_axle_l",[.72,-1.42,-.42]),
        socket_node("socket_axle_r",[.72,1.42,-.42])],
      "skins":[{"name":"GTT_FarmTrailer_Skin","joints":[1,2,3],"skeleton":1,"inverseBindMatrices":ibma,
                "extras":{"required_joints":REQUIRED_JOINTS,"required_sockets":REQUIRED_SOCKETS,
                          "interchange_socket_nodes":SOCKET_NODE_NAMES}}],
      "meshes":[{"name":"GTT_FarmTrailer","primitives":prims}],
      "materials":[
        {"name":"GTT_TrailerPaint","pbrMetallicRoughness":{"baseColorFactor":[.10,.22,.11,1],"metallicFactor":.55,"roughnessFactor":.62}},
        {"name":"GTT_Tire","pbrMetallicRoughness":{"baseColorFactor":[.025,.025,.03,1],"metallicFactor":0,"roughnessFactor":.92}}],
      "buffers":[{"byteLength":len(b.buf),"uri":uri}],"bufferViews":b.views,"accessors":b.acc,
      "extras":{"gtt_asset_contract":ASSET_VERSION,"required_joints":REQUIRED_JOINTS,"required_sockets":REQUIRED_SOCKETS,
                "interchange_socket_nodes":SOCKET_NODE_NAMES,
                "units":"meters","source":"procedurally authored for Swir/GTT; no third-party protected game assets",
                "acceptance":"source-rig candidate only; Unreal import + PhysicsAsset + packaged Win64 runtime/visual evidence required"}}

def main():
    ap=argparse.ArgumentParser(); ap.add_argument("--output",default="SourceArt/Trailer/GTT_FarmTrailer_Rig.gltf"); ap.add_argument("--check",action="store_true")
    a=ap.parse_args(); text=json.dumps(generate(),sort_keys=True,separators=(",",":"))+"\n"; p=Path(a.output)
    if a.check:
        if not p.is_file(): raise SystemExit(f"missing generated asset: {p}")
        cur=p.read_text(encoding="utf-8")
        if cur!=text: raise SystemExit(f"generated trailer rig is stale: {p}")
        print(f"OK {p} sha256={hashlib.sha256(cur.encode()).hexdigest()}"); return
    p.parent.mkdir(parents=True,exist_ok=True); p.write_text(text,encoding="utf-8")
    print(f"Wrote {p} sha256={hashlib.sha256(text.encode()).hexdigest()}")

if __name__=="__main__": main()
