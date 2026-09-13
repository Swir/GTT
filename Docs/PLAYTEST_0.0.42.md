# GTT 0.0.42 — Fieldmaster Native Pawn Acceptance

This milestone adds the first concrete `AWheeledVehiclePawn` shell for the Rusty Fieldmaster 60. It is a source-level activation path, not a claim that a final authored skeletal/physics asset has already passed Unreal runtime acceptance.

## Source acceptance

1. Open `AGTTFieldmasterNativePawn` in an Unreal 5.8 project build.
2. Confirm its movement component resolves as `UChaosWheeledVehicleMovementComponent`.
3. Assign a Fieldmaster skeletal mesh containing `root`, `wheel_fl`, `wheel_fr`, `wheel_rl`, `wheel_rr` plus the required `driver_seat`, `driver_exit` and `rear_hitch` sockets.
4. Assign a valid Physics Asset covering the chassis and wheel bodies.
5. Start PIE and confirm `ConfigureAndValidateNativeFieldmaster` reports valid RIG, WHEELS, POWERTRAIN and `PHYSICS ASSET: YES`.
6. Confirm a deliberately missing bone/socket or Physics Asset keeps `IsNativeFieldmasterReady()` false.

## Runtime driving acceptance

The following must be tested before either Native Chaos roadmap checkbox can be closed:

- forward and reverse launch without wheel explosion or chassis tunnelling,
- steering at low and high speed,
- suspension compression/rebound on road edges and off-road terrain,
- braking and neutral/idle behavior,
- wheel contact and traction on mud/off-road zones,
- fuel, condition, tire wear and tuning still influence the shared gameplay state,
- enter/exit, theft heat, police/ranger systems, save/load and garage persistence remain intact,
- `rear_hitch` remains usable by the farm trailer integration,
- no double-simulation from the legacy drivetrain after native acceptance.

## Packaged acceptance

A Windows demo/release still requires a real UE 5.8 Win64 compile, package and packaged EXE smoke test. Verify the Fieldmaster in the packaged build at day and night, with trailer attached and detached, and visually inspect the authored vehicle/rig before considering a public demo.

## Honesty gate

0.0.42 does **not** mark `Dedicated native Chaos wheeled tractor movement` or `Dedicated native Chaos drivetrain/suspension/wheel setup` complete. Those remain open until the authored rig/Physics Asset exists and the runtime checks above are actually performed.
