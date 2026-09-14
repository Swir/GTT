# Native Trailer Authoring & Activation Contract

This document is the canonical source/runtime handoff for the final original GTT farm trailer. It does not provide or claim a finished binary Unreal asset.

## Required authored skeletal contract

The trailer skeletal mesh component must be tagged `GTT.AuthoredTrailerRig` or named `AuthoredTrailerMesh` and must provide:

- Physics Asset assigned to the skeletal mesh component.
- `root` bone for the trailer chassis hierarchy.
- `wheel_l` and `wheel_r` wheel bones at the real left/right rolling centres.
- `tow_eye` socket at the physical coupling point.
- `axle_center` socket at the real axle centre used by runtime stabilization.

The final model, rig, materials and textures must be original GTT content or otherwise properly licensed. Do not import protected assets from GTA or any other commercial game.

## Runtime stages

1. `UGTTTrailerNativeAcceptanceSubsystem` performs strict authored-rig and Native Fieldmaster hitch acceptance, plus jackknife/fail-safe monitoring.
2. `UGTTTrailerAuthoredRuntimeSubsystem` activates only after the authored rig is valid.
3. The authored skeletal component becomes the visible trailer presentation while the existing greybox physics/presentation remains available as a safe fallback.
4. `wheel_l` / `wheel_r` feed independent ground-contact traces and axle-tilt evidence.
5. `axle_center` is the application point for load/contact-aware lateral and yaw stabilization during Native heavy haul.
6. `tow_eye` is compared against the Native Fieldmaster `rear_hitch` so authoring errors are measurable in runtime evidence.

## Authoring tolerances

- Put wheel bones at the actual rolling centres, not at suspension pivots or decorative hub offsets.
- Place `axle_center` midway between the intended wheel centres on the physical axle.
- Place `tow_eye` on the real coupler pivot so it can coincide with `rear_hitch` without mesh offsets.
- Do not compensate for a bad socket with hard-coded world offsets. Fix the authored asset.
- Keep the chassis root/origin stable enough that greybox fallback and authored presentation occupy the same world footprint.

## Activation evidence

A valid runtime should emit `AUTHORED_TRAILER_RUNTIME_EVIDENCE` with:

- `active=1`
- `contacts` showing the current 0.0/0.5/1.0 wheel contact ratio
- independent left/right clearances
- authored axle tilt
- `hitchError` while attached to Native Fieldmaster
- articulation angle
- stabilization load

The existing 0.0.72 evidence `NATIVE_TRAILER_ACCEPTANCE_EVIDENCE` must also remain healthy. Neither token alone is sufficient to close the roadmap checkbox: the final authored `.uasset` must be committed and observed in real Unreal runtime.

## Fallback rule

If the authored component is absent or invalid, the subsystem must not partially activate. Greybox visuals stay/restored, existing axle constraints/cargo/repair systems remain authoritative, and gameplay must remain recoverable.

## Roadmap close rule

Only mark `Authored skeletal trailer wheel assets and final hitch sockets` complete after all of the following are true:

1. The final original skeletal trailer asset is committed.
2. Required bones/sockets/Physics Asset pass the source/runtime contract.
3. Unreal runtime shows the authored trailer, both wheel contacts, correct hitch alignment and heavy-haul behavior.
4. A real playtest confirms axle damage, cargo integrity and trailer recovery still work.

Until then the roadmap must remain unchanged.
