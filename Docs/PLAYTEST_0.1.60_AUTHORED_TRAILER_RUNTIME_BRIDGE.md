# GTT 0.1.60 — Authored Trailer Runtime Bridge Acceptance

This milestone connects the project-owned trailer source rig to the live `AGTTFarmTrailer` runtime path without transferring gameplay authority away from the existing physical trailer actor. It is deliberately fail-closed: if the final Unreal skeletal asset, required bones, required sockets or PhysicsAsset are missing, the subsystem leaves the proven placeholder presentation active.

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

## Passive packaged-runtime evidence

The same presentation subsystem now emits the machine-readable observations already consumed by `Scripts/evaluate_authored_trailer_runtime.ps1`; this closes the source-side telemetry gap without claiming that the future Win64 run has happened.

- `AUTHORED_TRAILER_RUNTIME_EVIDENCE` samples report authored activation, Native Fieldmaster attachment, dual-wheel ground contact/clearance, axle tilt, authored-vs-physical hitch alignment, articulation, mirrored stability authority and jackknife warning state.
- Ground contact is measured with downward world traces from the existing physical wheel bodies; the evidence path does not move or repair the trailer.
- Authored `socket_hitch` is compared with the existing physical `HitchCoupler`, so the final imported rig must remain aligned with the proven physical hitch geometry.
- `NATIVE_TRAILER_SCENARIO_SAMPLE` accumulates only while the final authored presentation is active on a loaded trailer attached to the Native Fieldmaster with both wheels intact.
- A PASS marker requires at least 900 cm of observed travel, at least 4 km/h peak speed, eight dual-contact samples, eight safe-hitch samples and a controlled stop at or below 1.5 km/h.
- Hitch error above 110 cm emits an explicit diagnostic failure. Safe samples require at most 80 cm hitch error and no jackknife warning.
- The evidence-only stability/jackknife calculation mirrors the authoritative `UGTTTrailerRoadFeedbackSubsystem` constants and is deterministically checked for drift. It applies no duplicate forces or gameplay mutation.
- The packaged evaluator still requires `BUILD_INFO.json` for Win64, packaged runtime smoke PASS, Native Chaos runtime PASS, deterministic drivetrain PASS and same-SHA evidence before it can produce `NATIVE_TRAILER_RUNTIME.json` PASS.

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
5. Verify `AUTHORED_TRAILER_RUNTIME_EVIDENCE` begins only after the authored asset passes validation; confirm `hitchError` is measured against the physical hitch coupler.
6. Verify no placeholder chassis, fender, drawbar or cylinder wheel presentation remains visible after authored activation.
7. Verify the authored wheel geometry follows real suspension travel while the physical wheel bodies remain collision authority.
8. Break the left wheel, then the right wheel, and verify the corresponding authored wheel geometry follows the detached physical body instead of remaining glued to the chassis.
9. Load and unload cargo; cargo visuals and cargo integrity must remain authoritative and must not be duplicated by the authored rig.
10. Attach to the Native Fieldmaster, reach road speed and verify 0.1.59 tow-load coupling plus 0.1.60 anti-sway/jackknife logic remain active.
11. With a loaded, intact trailer, drive at least 900 cm above 4 km/h, keep both wheels grounded/hitch aligned, then stop below 1.5 km/h; verify a single `NATIVE_TRAILER_SCENARIO_COMPLETE result=PASS` is emitted.
12. Trigger roadside trailer repair and verify repaired wheel state/presentation returns without transferring payment or repair authority to the presentation/evidence bridge.
13. Package the exact candidate on UE 5.8 Win64 and run `Scripts/evaluate_authored_trailer_runtime.ps1` against the packaged runtime log.
14. Reject the candidate if the evaluator is not PASS or visual review shows scale mismatch, wheel clipping/floating, broken hitch alignment, socket mismatch or placeholder overlap.

## Roadmap truth

This runtime/evidence bridge materially removes source-side integration and observability work from the authored-trailer blocker, but it does **not** close `Authored skeletal trailer wheel assets and final hitch sockets`. That checkbox still requires the real imported Unreal asset, final sockets/PhysicsAsset, same-SHA packaged Win64 runtime evidence and rendered visual acceptance. Roadmap truth remains **125/130 (96.2%)**.
