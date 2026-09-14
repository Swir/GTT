# GTT 0.0.58 Playtest — Native Traction & Slip Control

## Goal
Verify that the Native Fieldmaster converts live chassis slip, wheel contact, axle unloading, tire condition and heavy-haul tongue load into progressive traction control before the stronger rollover/fishtail intervention is required.

## Required runtime
This playtest requires a real Unreal Engine 5.8 runtime. Source sanity checks do not replace Win64 packaging/runtime acceptance.

## Test A — Healthy level-road baseline
1. Use a healthy Native Fieldmaster with good tires and no trailer.
2. Drive normally at 10–35 km/h on level road with smooth steering.
3. Observe `NATIVE_STABILITY_EVIDENCE`.

Expected: `traction_risk` stays low, `slip_deg` remains modest, `front_traction` and `rear_traction` stay high, `traction_control=NO`, and `throttle_limit` remains 1.00.

## Test B — Tire degradation
1. Reduce tire integrity through existing damage/mud gameplay.
2. Accelerate and corner above 9 km/h without creating a rollover condition.
3. Hold a sustained moderate slide for longer than the traction-loss grace period.

Expected: lower tire integrity reduces axle traction. `traction_risk` rises, `traction_control=YES`, and `throttle_limit` drops progressively instead of instantly applying the full stability shutdown.

## Test C — Heavy-haul front unloading
1. Attach and load the Heavy Timber Haul trailer.
2. Climb an uneven grade or crest while applying throttle.
3. Watch `front_rear_bias`, `front_traction`, `rear_traction`, `tow_load`, and `traction_risk`.

Expected: tongue load plus front-axle unloading reduces steering-axle traction. The controller limits requested throttle when the loss persists; severe combined slip/load-transfer can escalate into the existing full intervention.

## Test D — Lateral slip recovery
1. Produce a controlled lateral slide on uneven terrain at moderate speed.
2. Keep the vehicle below rollover attitude while `slip_deg` rises.
3. Reduce steering/throttle and regain alignment.

Expected: progressive throttle limiting/light braking appears during sustained slip and clears when `traction_risk` falls. Inputs must not remain latched after recovery.

## Test E — Tire upgrade comparison
1. Repeat the same route with tire upgrade level 0 and then level 3.
2. Keep tire integrity, cargo and route as similar as possible.

Expected: upgraded tires provide a modest traction reserve, but do not defeat protection during severe slip, lost wheel contact or heavy-haul instability.

## Evidence to keep
Capture `NATIVE_STABILITY_EVIDENCE` lines containing `traction_risk`, `slip_deg`, `front_traction`, `rear_traction`, `traction_loss_s`, `traction_control`, `throttle_limit`, `tire_integrity`, `tow_load`, `load_transfer`, `intervention`, and `brake`.

## Demo gate
Do not mark demo-ready from this test alone. Demo still requires a verified Win64 Unreal package, packaged-EXE runtime smoke, visual acceptance, green relevant CI, and no demo-critical blockers.
