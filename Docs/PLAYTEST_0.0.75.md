# GTT 0.0.75 Playtest — Native Drivetrain Authority

## Scope

This milestone adds one final safety authority over the already-established Native Chaos vehicle-specific input/tuning layers for **Rusty Fieldmaster 60**, **Rattleback 82** and **Mulebox 1200**. It does not replace tire, cargo, damage or traction scaling. It prevents unsafe drivetrain commands from bypassing those systems.

## Test 1 — forward to reverse direction interlock

1. Enter each accepted Native vehicle.
2. Accelerate forward above roughly 10 km/h.
3. While still moving forward, hold full reverse input.
4. Confirm throttle is cut immediately and braking is applied instead of accepting an instant reverse drivetrain swap.
5. Confirm `NATIVE_DIRECTION_INTERLOCK` appears while the vehicle is still above the release speed.
6. Confirm steering authority is reduced during the interlock rather than allowing a full-lock snap while the drivetrain is braking through the direction change.
7. Once speed falls below roughly 3.5 km/h, confirm the reverse gear is committed and `NATIVE_DIRECTION_SHIFT_COMMIT` appears.
8. Repeat reverse-to-forward.

Expected result: no high-speed instant forward/reverse gear swap; the vehicle must decelerate before the requested direction becomes authoritative.

## Test 2 — neutral engine braking

1. Drive each Native vehicle on level road to moderate speed.
2. Release `VehicleThrottle` completely without pressing the opposite direction.
3. Confirm throttle becomes zero and the vehicle receives a modest speed-sensitive engine-braking force.
4. Confirm the effect is weaker than an emergency/interlock stop and does not resemble a full service-brake lock.
5. Confirm periodic `NATIVE_DRIVETRAIN_AUTHORITY_EVIDENCE` reports `engine_brake=YES`.

Expected result: coasting has controlled drivetrain drag instead of a completely free roll or an abrupt full stop.

## Test 3 — low-speed hill hold

1. Stop a Native vehicle on a mild farm or village incline.
2. Release throttle near zero speed.
3. Confirm the shared drivetrain authority applies the stationary hold brake below roughly 1.6 km/h.
4. Apply throttle in the current travel direction and confirm the hold releases naturally.
5. On a rollback, request the opposite motion direction and confirm the direction interlock arrests motion before committing the new gear.

Expected result: the Fieldmaster, Rattleback and Mulebox resist unwanted low-speed rolling without teleporting or adding a second physics model.

## Test 4 — existing gameplay systems remain authoritative

### Fieldmaster
- Verify the existing condition/tire governor still limits top speed and applies its own safety braking.
- Verify fuel exhaustion/critical condition still forces breakdown behavior.
- Verify heavy-haul, terrain and wheel-state systems still operate.

### Rattleback / Mulebox
- Verify road-vehicle tire slip, body-damage power loss and steering limits remain visible.
- Load the Mulebox cargo contract and confirm cargo power/steering penalties are not replaced by drivetrain authority.
- Cause a serious collision and confirm body damage/workshop/recovery loops remain intact.

Expected result: 0.0.75 acts as the final drivetrain safety authority only; it does not flatten vehicle-specific tuning or bypass the existing economy/damage loops.

## Test 5 — evidence capture

For each vehicle capture at least one `NATIVE_DRIVETRAIN_AUTHORITY_EVIDENCE` line while driving and one direction-change event. Evidence should include vehicle ID, signed speed, raw throttle/steer, current gear, stable direction, interlock state, engine-brake state and authority brake level.

## Regression / release honesty

- Run `python Scripts/verify_native_drivetrain_authority.py`.
- Run the complete Project sanity workflow.
- Roadmap remains **125/130 (96.2%)**. This source/runtime authority milestone does **not** close either Native Chaos roadmap checkbox without real UE 5.8 runtime acceptance on authored vehicle assets.
- A Windows demo release still requires a verified UE 5.8 Win64 package, successful packaged-EXE runtime smoke test, green relevant CI and rendered visual approval. Source-level sanity success alone is not demo verification.
