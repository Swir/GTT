# GTT 0.0.35 — Native Rig Architecture Playtest

This milestone turns the 0.0.34 fleet bridge into an asset-ready Chaos rig contract. It adds concrete native `UChaosVehicleWheel` classes for every owned vehicle axle plus one canonical bone/socket naming contract for future skeletal meshes and physics assets.

## What is implemented

- Six native Chaos wheel classes: front/rear for Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200.
- Wheel radius, width, suspension travel, spring rate, damping, friction and steering angle are sourced from the existing canonical `FGTTChaosVehicleSpec` values rather than duplicated tuning numbers.
- Driven/steered/braked/handbraked axle behavior is explicit per wheel class.
- Canonical skeletal contract: `root`, `wheel_fl`, `wheel_fr`, `wheel_rl`, `wheel_rr`, `driver_seat`, `driver_exit`.
- Fieldmaster and Mulebox additionally require `rear_hitch`; Rattleback intentionally does not.
- Rig lookup uses the same persistent vehicle IDs as save/load and the 0.0.34 Chaos bridge.

## Source-level acceptance

1. Confirm all six wheel classes compile against UE 5.8 `UChaosVehicleWheel` API.
2. Confirm the constructors read the canonical vehicle specs.
3. Confirm front wheels steer and rear wheels provide driven/handbrake behavior for the current rear-drive fleet contract.
4. Confirm each rig contract resolves from its persistent ID.
5. Confirm Fieldmaster/Mulebox require `rear_hitch` while Rattleback does not.
6. Run `python Scripts/verify_native_chaos_rig.py` and the full Project sanity workflow.

## UE 5.8 native asset acceptance — still required

For each vehicle, create/import an original GTT skeletal mesh and physics asset that obeys the contract, then configure a real `AWheeledVehiclePawn` / `UChaosWheeledVehicleMovementComponent` path with these wheel classes.

Required validation:

1. Physics body remains stable at rest and under collision.
2. All four wheels contact terrain at the intended positions and rotate around the correct axes.
3. Steering, suspension compression/rebound and braking are visually and physically coherent.
4. The 0.0.34 bridge transitions into native driving without simultaneous legacy forces.
5. Fuel, condition, engine tune, tire integrity/tune and mud response remain authoritative.
6. Fieldmaster `rear_hitch` works with the articulated trailer under load and survives save/garage regression.
7. Package Win64 and perform a real EXE launch/drive/exit/relaunch smoke test.

## Demo gate

This milestone improves the path to production vehicle handling, but it is not by itself a demo-release gate pass. Do not publish the first demo until the native or fallback vehicle presentation is visually coherent, the playable slice looks presentable, and an actual packaged Win64 EXE passes runtime smoke testing.

There is no packaged EXE verification for 0.0.35 on the current sanity runner. Native Chaos roadmap checkboxes therefore remain open.
