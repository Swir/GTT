# GTT Farm Trailer Source Art

The original project-owned farm-trailer source is authored deterministically by `Scripts/generate_gtt_farm_trailer_gltf.py`. It generates a self-contained `GTT_FarmTrailer_Rig.gltf` import candidate with original geometry, `body` / `wheel_l` / `wheel_r` skeletal joints and the `socket_hitch`, `socket_cargo`, `socket_axle_l`, `socket_axle_r` anchor contract.

Generate and validate it with:

```bash
python Scripts/generate_gtt_farm_trailer_gltf.py --output /tmp/GTT_FarmTrailer_Rig.gltf
python Scripts/verify_v0_1_60_authored_trailer_source_rig.py --asset /tmp/GTT_FarmTrailer_Rig.gltf
```

The derived glTF is intentionally not committed as a substitute for a tested Unreal `.uasset`; the reviewable geometry/rig recipe stays in source control and CI generates the exact import candidate. UE 5.8 import, Physics Asset setup, final Unreal sockets, runtime towing evidence and rendered visual acceptance remain required before the authored-trailer roadmap gate can close.
