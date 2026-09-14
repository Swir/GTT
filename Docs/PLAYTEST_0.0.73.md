# GTT 0.0.73 Playtest — Authored Trailer Runtime Takeover & Wheel-Contact Dynamics

## Purpose
Verify that a final original skeletal trailer can actually take over presentation and contribute authored wheel/axle geometry to the Native Fieldmaster heavy-haul loop.

## Setup
1. Use Unreal Engine 5.8 with the accepted Native Fieldmaster takeover available.
2. Add the intended original trailer skeletal mesh component to `AGTTFarmTrailer` and tag it `GTT.AuthoredTrailerRig` (or name it `AuthoredTrailerMesh`).
3. Assign a Physics Asset.
4. Confirm bones `root`, `wheel_l`, `wheel_r` and sockets `tow_eye`, `axle_center` exist.
5. Open Output Log and filter for `AUTHORED_TRAILER_RUNTIME_EVIDENCE` and `NATIVE_TRAILER`.

## Scenario A — Presentation takeover
1. Spawn a trailer with a valid authored rig.
2. Confirm the skeletal trailer becomes visible and the legacy primitive presentation is hidden.
3. Remove the Physics Asset or required socket from a test copy.
4. Confirm the authored presentation is rejected and greybox visuals return instead of leaving the trailer invisible.

## Scenario B — Authored wheel contacts
1. Park on flat ground and confirm both wheel contacts report active with reasonable clearances.
2. Drive one wheel onto a bank/ditch edge.
3. Confirm left/right contact and clearance telemetry diverge and axle tilt changes with the authored wheel positions.
4. Confirm no contact is fabricated when a wheel is genuinely unsupported.

## Scenario C — Native heavy-haul dynamics
1. Attach to an accepted Native Fieldmaster.
2. Load the heavy-haul cargo and drive straight, through a medium-speed bend and over uneven ground.
3. Confirm `stabilization` rises with lateral motion, articulation and the existing tow-load factor.
4. Confirm the trailer receives anti-sway force/torque at the authored `axle_center` rather than using a separate mission-only fake state.
5. Confirm normal cargo integrity, trailer integrity and axle-damage logic still operate.

## Scenario D — Authored hitch geometry
1. Confirm `tow_eye` aligns closely with the Fieldmaster `rear_hitch` during normal towing.
2. Deliberately offset a test rig and verify hitch-error telemetry rises.
3. Confirm the existing 0.0.72 Native trailer acceptance/fail-safe remains authoritative for invalid hitch geometry and jackknife detach.

## Scenario E — Recovery and fallback
1. Damage the trailer and use the existing roadside trailer repair/recovery flow.
2. Confirm the authored runtime presentation remains active after a valid repair.
3. Make the authored rig invalid at runtime in a development test and confirm greybox presentation is restored.
4. Confirm heavy-haul can continue through the legacy fallback instead of becoming soft-locked.

## Demo gate
Do not publish a demo based on source-level validation alone. The same release candidate still requires a successful Unreal Engine 5.8 Win64 compile/package, packaged-EXE runtime smoke test, rendered visual acceptance, green relevant Actions and no demo-critical blocker.
