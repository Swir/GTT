# GTT 0.0.72 Playtest — Native Trailer Authored-Rig Acceptance & Hitch Safety

## Purpose
Verify that the articulated farm trailer has a strict authored-rig and Native hitch runtime acceptance path before the remaining trailer roadmap checkbox can ever be closed.

## Setup
1. Use a real Unreal Engine 5.8 build with the accepted Native Fieldmaster takeover path available.
2. Add/assign the intended authored trailer skeletal component and tag it `GTT.AuthoredTrailerRig` (or name it `AuthoredTrailerMesh`).
3. The authored trailer asset must expose `root`, `wheel_l`, `wheel_r` bones, a Physics Asset and `tow_eye` plus `axle_center` sockets.
4. Open Output Log and filter for `NATIVE_TRAILER`.

## Scenario A — Valid authored rig
1. Spawn the trailer with the authored skeletal component configured.
2. Attach it to the active Native Fieldmaster.
3. Confirm `NATIVE_TRAILER_ACCEPTANCE_EVIDENCE` reports authored=1, physics=1, bones=1, sockets=1, nativeTow=1, aligned=1 and accepted=1.
4. Drive straight, reverse and turn through ordinary farm-yard angles.
5. Confirm the trailer remains attached and the existing heavy-haul cargo/axle systems continue to work.

## Scenario B — Missing authored requirement
1. Repeat with one required bone/socket or the Physics Asset deliberately removed from a test copy.
2. Confirm evidence reports accepted=0 and names the missing contract reason.
3. Confirm an attached Native trailer does not remain silently accepted; after the short grace period `NATIVE_TRAILER_FAILSAFE` is emitted and the trailer detaches.

## Scenario C — Hitch alignment
1. With a valid authored rig, deliberately offset the trailer so its coupler exceeds the runtime hitch alignment tolerance.
2. Confirm evidence shows aligned=0 / `HITCH_ALIGNMENT_ERROR`.
3. Confirm sustained invalid alignment triggers the fail-safe detach rather than preserving a corrupt constraint state.

## Scenario D — Jackknife safety
1. Attach a valid authored trailer and create a high-articulation reversing manoeuvre.
2. Above the warning angle confirm `NATIVE_TRAILER_JACKKNIFE_WARNING` appears.
3. Above the hard limit confirm runtime acceptance fails and the fail-safe detaches after the grace period.
4. Confirm the player can recover/reset the trailer using the existing trailer recovery path.

## Scenario E — Regression
- Legacy/greybox trailer behavior remains available when no authored skeletal trailer rig is present.
- Existing breakable wheel constraints, cargo integrity, trailer integrity, roadside repair and heavy-haul payout logic continue to function.
- Native Fieldmaster hitch handoff still uses the accepted `rear_hitch` transform.
- Roadmap must remain 125/130 until the final authored trailer asset is actually committed and runtime-validated.

## Demo gate
This milestone is source/runtime acceptance architecture only. Do not publish a demo until the same release candidate has a verified UE 5.8 Win64 package, packaged-EXE smoke test, rendered visual acceptance, green relevant Actions and no demo-critical blocker.
