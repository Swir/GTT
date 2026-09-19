# GTT 0.1.57 — Physical Trailer Suspension & Hitch Dynamics Playtest

This checklist validates the new physical trailer-dynamics layer without treating source-contract CI as packaged Win64 acceptance.

## Automated source acceptance

- `LeftWheelConstraint` and `RightWheelConstraint` gain bounded vertical travel instead of a fully locked Z axis.
- Suspension uses position/velocity drive with explicit spring, damping and force-limit values.
- Loaded cargo increases spring and damping authority while preserving the existing 980 kg empty / 1680 kg loaded body-mass loop.
- Hitch break force and torque react to cargo state, trailer integrity and live hitch stress.
- A physically broken hitch routes through the existing authoritative `AGTTFarmTrailer::DetachTrailer()` path.
- The trailer-dynamics subsystem does not write throttle, brake, steering or target gear; final native drivetrain input remains owned by the existing drivetrain subsystem.
- Roadmap truth remains 125/130 (96.2%); authored skeletal trailer wheels/final hitch sockets and Win64 runtime gates stay open.

## Unreal runtime acceptance — required before closing roadmap gates

1. **Empty trailer / uneven shoulder** — tow an empty trailer over a rough shoulder and verify both wheel constraints show controlled vertical travel without persistent body jitter.
2. **Loaded trailer / same route** — load cargo and repeat the route; verify the heavier 1680 kg state remains authoritative and suspension settles with the stronger loaded tuning.
3. **Normal road towing** — drive and corner normally for at least five minutes; the hitch must not break under ordinary road loads.
4. **Severe overstress** — reproduce a hard jackknife/high-load event with reduced trailer integrity; a physical hitch break must detach through normal trailer state without duplicate cargo payout or duplicate damage authority.
5. **Wheel damage and recovery** — break a trailer wheel, use the existing roadside repair path, and confirm the restored constraint receives the bounded suspension configuration again.
6. **Farm Cargo continuity** — load, tow, save/load if available in the candidate, deliver and verify the mission/economy authority remains unchanged.
7. **Native Fieldmaster authority** — with the native Fieldmaster path active, verify trailer dynamics never overrides final throttle/brake/steering/gear composition.
8. **Packaged candidate** — repeat the key empty/loaded/hitch-break scenarios in the exact UE 5.8 Win64 packaged candidate before any demo or roadmap gate is marked complete.

## Evidence expected

Capture the same-candidate runtime log lines `TRAILER_NATIVE_DYNAMICS` and, when intentionally reproduced, `TRAILER_HITCH_PHYSICS_BREAK`, together with the candidate commit SHA. Screenshots/video should show the actual trailer and wheel behavior; source-contract success alone is not visual or packaged-runtime proof.
