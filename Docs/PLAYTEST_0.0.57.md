# GTT 0.0.57 Playtest — Native Load Transfer & Grade Response

## Goal
Verify that Native Fieldmaster stability reacts to chassis weight transfer, grade and heavy-haul tongue load without false positives during ordinary level driving.

## Required runtime
This playtest requires a real Unreal Engine 5.8 runtime. Source sanity checks do not replace Win64 packaging/runtime acceptance.

## Test A — Level unloaded baseline
1. Start Native Fieldmaster takeover on level ground with no trailer.
2. Drive 15–30 km/h across ordinary terrain.
3. Confirm `NATIVE_STABILITY_EVIDENCE` reports low `load_transfer`, near-neutral `front_rear_bias`/`side_bias`, and no sustained intervention.

Expected: no unnecessary throttle cut or brake intervention.

## Test B — Loaded grade
1. Attach the Heavy Timber Haul trailer and load timber.
2. Climb and descend a meaningful grade above 12 km/h.
3. Observe `tow_load`, `load_transfer`, `front_rear_bias`, pitch and brake evidence.

Expected: heavy tow load raises chassis risk under strong longitudinal transfer; sustained unsafe transfer cuts throttle and adds proportional braking.

## Test C — Off-camber terrain
1. With loaded trailer attached, cross an uneven shoulder or hillside at moderate speed.
2. Produce a persistent left/right wheel-clearance difference without rolling the vehicle.

Expected: `side_bias` rises. If the imbalance is sustained and severe, the system suppresses steering and applies braking rather than allowing the oscillation to grow.

## Test D — Recovery
1. Return to level terrain and reduce speed.
2. Confirm load-transfer timers clear and intervention stops.

Expected: protection is temporary and does not leave brakes/steering latched.

## Evidence to keep
Capture log lines containing `NATIVE_STABILITY_EVIDENCE` with `load_transfer`, `front_rear_bias`, `side_bias`, `load_transfer_s`, `tow_load`, `intervention` and `brake`.

## Demo gate
Do not mark demo-ready from this test alone. Demo still requires verified Win64 package, packaged-EXE runtime smoke, visual acceptance, green relevant CI, and no demo-critical blockers.