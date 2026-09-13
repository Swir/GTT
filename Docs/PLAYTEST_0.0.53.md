# GTT 0.0.53 — Native Fieldmaster Drive Dynamics

## Goal
Make the active Native Chaos Fieldmaster respond to the same persistent condition, tire and tuning state that already drives the legacy vehicle economy instead of behaving like a pristine vehicle regardless of damage.

## What changed
- Added a game-world `UGTTNativeDriveDynamicsSubsystem` that operates only while the Native Fieldmaster takeover is active and accepted.
- Engine tuning now raises the native tractor's effective speed ceiling.
- Poor vehicle condition lowers the effective speed ceiling.
- Damaged tires lower the effective speed ceiling and, at severe tire damage, add progressive rolling drag at road speed.
- Critical vehicle condition or an empty fuel tank commands zero throttle/steering and full braking instead of allowing the native pawn to keep driving.
- Periodic `NATIVE_DRIVE_DYNAMICS` evidence records speed, calculated cap, condition, tire state, upgrade levels, governor activity, brake drag and critical state.

## Runtime playtest
1. Launch in Unreal Engine 5.8 with an accepted authored Native Fieldmaster rig and activate takeover.
2. With healthy condition, healthy tires and engine tune level 0, accelerate on a flat road and confirm the tractor approaches the base native cap without repeated governor oscillation.
3. Apply engine tune levels 1–3 through the existing tuning service and repeat the road run. Confirm the allowed native speed envelope rises by the expected amount.
4. Damage body condition to roughly 50% and repeat. Confirm the effective speed cap drops and `NATIVE_DRIVE_DYNAMICS` reports a lower `cap_kmh`.
5. Damage tire integrity below 35%. Confirm road-speed rolling drag appears and damaged tires cannot sustain the healthy top-speed envelope.
6. Reduce condition below the critical threshold. Confirm throttle and steering are cancelled and full braking is commanded.
7. Refuel/repair through the existing workshop and confirm the native vehicle recovers because the subsystem consumes the shared migration snapshot rather than a duplicate state.
8. Confirm legacy Fieldmaster, old car and van behavior is unchanged when Native takeover is not active.

## Acceptance limits
- Repository sanity verifies source wiring and roadmap honesty only.
- This milestone does not close Native Chaos movement/drivetrain roadmap items without real UE 5.8 runtime handling evidence.
- Demo still requires verified Win64 package, packaged EXE smoke test, rendered visual acceptance and green relevant Actions.
