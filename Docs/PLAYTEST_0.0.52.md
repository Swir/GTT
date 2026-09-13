# GTT 0.0.52 — Native Chaos Runtime Guard & Evidence

## Goal
Close the gap between source-level Native Chaos acceptance and real Unreal runtime behavior without pretending that a packaged Win64 build has been verified.

## What changed
- Added an automatic `UTickableWorldSubsystem` that watches the active Native Fieldmaster takeover in real game worlds.
- A takeover is considered runtime-healthy only while the native pawn is still accepted, the Chaos movement component is active, a Physics Asset exists, and collision remains enabled.
- If that contract stays unhealthy for more than 1.5 seconds, the subsystem calls the existing `DeactivateLegacyTakeover()` path and restores the proven legacy vehicle instead of leaving the player trapped in a broken native pawn.
- While takeover is active the subsystem emits periodic `NATIVE_CHAOS_EVIDENCE` log lines with takeover/readiness state, movement activity, physics-asset/collision state, occupancy, speed, condition and fuel.
- The evidence log is deliberately diagnostic. It does **not** by itself close the Native Chaos roadmap items or prove a packaged Win64 demo.

## Runtime playtest
1. Launch the project in Unreal Engine 5.8 with a valid authored Fieldmaster skeletal mesh and Physics Asset.
2. Acquire/own the Rusty Fieldmaster 60 so native takeover can activate.
3. Enter the native Fieldmaster and drive for at least 30 seconds over road and uneven terrain.
4. Inspect the Unreal log for repeated `NATIVE_CHAOS_EVIDENCE` entries. Confirm `takeover=YES`, `ready=YES`, `movement=ACTIVE`, `physics_asset=YES`, `collision=YES` while driving.
5. Verify speed values rise and fall with real movement and that condition/fuel reflect live gameplay state.
6. Negative test: disable/deactivate the Chaos movement component during a development session. After the grace period, confirm the runtime guard logs an error and the legacy Fieldmaster becomes available again instead of leaving a dead native takeover.
7. Re-enable the valid native setup and verify takeover can be attempted again normally.

## Acceptance limits
- Repository sanity verifies the presence and wiring of the guard/evidence contract only.
- Native Chaos roadmap checkboxes stay open until an authored vehicle rig is run in UE 5.8 and its handling/suspension/wheels are actually accepted.
- Demo release still additionally requires a verified Win64 package, packaged EXE runtime smoke, rendered visual acceptance and green relevant Actions.
