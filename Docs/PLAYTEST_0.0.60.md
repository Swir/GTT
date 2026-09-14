# GTT 0.0.60 Playtest — Native Terrain-Load & Hill Control

## Goal
Verify that the Native Rusty Fieldmaster 60 connects authored mud, grade, axle load evidence and heavy-haul tongue load into low-speed hill-start behavior, including rollback control.

## Required runtime
This playtest requires a real Unreal Engine 5.8 runtime with an accepted Native Fieldmaster rig. Repository sanity checks confirm source contracts only and do not replace a Win64 compile/package/runtime acceptance run.

## Test A — Dry unloaded hill start
1. Activate Native Fieldmaster takeover with healthy tires and no trailer.
2. Stop on a moderate dry slope above roughly 5 degrees.
3. Apply smooth throttle uphill.
4. Capture `NATIVE_TERRAIN_LOAD_EVIDENCE`.

Expected: terrain grip remains near the dry baseline, rear-load bias stays modest, throttle limiting is light or inactive, and the tractor pulls away without rollback control.

## Test B — Muddy hill start
1. Stop the same tractor in an authored `GTTMudZone` on a comparable slope.
2. Apply the same throttle input.
3. Capture `NATIVE_TERRAIN_LOAD_EVIDENCE` while speed remains below 12 km/h.

Expected: `mud` increases, `terrain_grip` and `launch_grip` decrease, and `throttle_limit` becomes lower than the dry run. The control should reduce wheelspin tendency without permanently stealing throttle after leaving mud/grade conditions.

## Test C — Heavy trailer axle unloading
1. Attach and load the Heavy Timber Haul trailer.
2. Stop on the same uphill section with the trailer aligned safely.
3. Compare front/rear clearance and `rear_load_bias` against the unloaded run.
4. Pull away under moderate throttle.

Expected: trailer `tow_load` increases rear-load bias and reduces allowed launch throttle when the front axle is unloaded. Existing sway/load-transfer systems remain active once speed rises.

## Test D — Rollback control
1. With the loaded trailer on a meaningful uphill grade, allow the tractor to roll backwards while throttle is requested forward.
2. Hold the condition for longer than the short rollback grace period.
3. Observe speed and telemetry.

Expected: after sustained reverse speed beyond the rollback threshold, `rollback_control=YES`, drive throttle is cut and proportional brake is applied. The system should prevent a rapidly accelerating backwards roll but release naturally after the condition clears.

## Test E — Flat-ground regression
1. Drive on flat dry road with and without the trailer.
2. Accelerate normally through low speed and beyond 12 km/h.

Expected: the terrain-load controller does not interfere with normal road acceleration. Higher-speed traction/stability behavior remains owned by the existing Native stability subsystem.

## Test F — Damage/tire regression
1. Repeat the muddy hill start with partially worn tires and then with a tire upgrade.
2. Compare `terrain_grip`, launch grip and throttle limit.

Expected: shared tire integrity continues to affect authored terrain grip; upgrades help but do not remove mud and load consequences.

## Demo gate
Do not publish a demo from this source evidence alone. Demo still requires a verified UE 5.8 Win64 package, successful packaged-EXE smoke test, rendered visual acceptance, green relevant GitHub Actions and no demo-critical blockers.
