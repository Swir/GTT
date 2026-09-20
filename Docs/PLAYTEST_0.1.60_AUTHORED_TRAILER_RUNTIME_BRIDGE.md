# GTT 0.1.60 — Authored Trailer Runtime Bridge Acceptance

This milestone connects the already-generated project-owned trailer source rig to the live `AGTTFarmTrailer` runtime path without transferring gameplay authority away from the existing physical trailer actor. It is deliberately fail-closed: if the final Unreal skeletal asset, required bones, required sockets or PhysicsAsset are missing, the subsystem leaves the proven placeholder presentation active.

## Runtime bridge contract

- Canonical authored Unreal asset path: `/Game/GTT/Vehicles/Trailer/SK_GTT_FarmTrailer.SK_GTT_FarmTrailer`.
- Required bones: `body`, `wheel_l`, `wheel_r`.
- Required sockets: `socket_hitch`, `socket_cargo`, `socket_axle_l`, `socket_axle_r`.
- A valid PhysicsAsset must be assigned before the authored presentation is accepted.
- The runtime bridge never becomes collision, suspension, hitch, damage, cargo-integrity or roadside-repair authority.
- The authored presentation is implemented as a `UPoseableMeshComponent` attached to the existing trailer root.
- Physical `LeftWheel` / `RightWheel` motion drives the authored `wheel_l` / `wheel_r` bone transforms so suspension travel and wheel-loss motion remain visible.
- Placeholder chassis/wheel presentation is hidden only after all authored-asset validation passes.
- Cargo logs remain driven by the existing authoritative trailer presentation path.

## Source verification

```bash
python Scripts/generate_gtt_farm_trailer_gltf.py --output /tmp/GTT_FarmTrailer_Rig.gltf
python Scripts/verify_v0_1_60_authored_trailer_source_rig.py --asset /tmp/GTT_FarmTrailer_Rig.gltf
python Scripts/verify_v0_1_60_authored_trailer_runtime_bridge.py
```

## Unreal Editor / PIE acceptance — still required

1. Generate and import the deterministic trailer rig into the canonical asset path.
2. Confirm `body`, `wheel_l`, `wheel_r` exist on the imported skeletal mesh.
3. Create/confirm the four final sockets and assign the final PhysicsAsset.
4. Launch PIE and verify `AUTHORED_TRAILER_PRESENTATION event=ACTIVATED` appears exactly for eligible farm trailers.
5. Verify no placeholder chassis, fender, drawbar or cylinder wheel presentation remains visible after authored activation.
6. Verify the authored wheel geometry follows real suspension travel while the physical wheel bodies remain collision authority.
7. Break the left wheel, then the right wheel, and verify the corresponding authored wheel geometry follows the detached physical body instead of remaining glued to the chassis.
8. Load and unload cargo; cargo visuals and cargo integrity must remain authoritative and must not be duplicated by the authored rig.
9. Attach to the Native Fieldmaster, reach road speed and verify 0.1.59 tow-load coupling plus 0.1.60 anti-sway/jackknife logic remain active.
10. Trigger roadside trailer repair and verify repaired wheel state/presentation returns without transferring payment or repair authority to the presentation bridge.
11. Package the exact candidate on UE 5.8 Win64 and run the existing authored-trailer runtime evaluator.
12. Reject the candidate if visual review shows scale mismatch, wheel clipping/floating, broken hitch alignment, socket mismatch or placeholder overlap.

## Roadmap truth

This runtime bridge materially removes source-side integration work from the authored-trailer blocker, but it does **not** close `Authored skeletal trailer wheel assets and final hitch sockets`. That checkbox still requires the real imported Unreal asset, final sockets/PhysicsAsset, same-SHA packaged Win64 runtime evidence and rendered visual acceptance.
