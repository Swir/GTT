# GTT 0.0.59 Playtest — Native Terrain Grip & Collision Consequences

## Goal
Verify that the Native Rusty Fieldmaster 60 keeps the same terrain and collision consequences as the proven legacy vehicle path: authored mud must reduce usable drive force and wear the same persistent tires, while meaningful collision hits must damage the same persistent condition/tire state.

## Required runtime
This playtest requires a real Unreal Engine 5.8 runtime with an accepted Native Fieldmaster rig. Repository sanity checks confirm source contracts only; they do not replace Win64 compile/package/runtime acceptance.

## Test A — Mud entry and exit
1. Activate Native Fieldmaster takeover and enter the tractor with healthy tires.
2. Approach an existing authored `GTTMudZone` at steady throttle.
3. Drive through the volume and then leave it.
4. Capture `NATIVE_TERRAIN_RESPONSE` evidence while inside the mud.

Expected: `mud` rises above zero, `grip` falls below the dry baseline, `throttle_limit` is reduced, horizontal speed is resisted and normal throttle authority returns after leaving the short mud-response hold window.

## Test B — Shared tire wear
1. Record the current migration `TireIntegrity`.
2. Drive repeatedly through mud above the 9 km/h wear threshold.
3. Compare tire integrity before/after and then observe `NATIVE_STABILITY_EVIDENCE` from the existing traction controller.

Expected: `TireIntegrity` decreases through the Native mud path and the same reduced tire value lowers front/rear traction estimates. Tire upgrades reduce wear/drag modestly but never eliminate mud consequences.

## Test C — Heavy-haul mud crossing
1. Attach and load the Heavy Timber Haul trailer.
2. Cross the same mud zone at controlled speed.
3. Observe trailer sway/load-transfer evidence together with terrain response.

Expected: mud does not bypass the existing heavy-haul systems. Reduced tire integrity and lower usable throttle make the loaded crossing materially harder while tow-load/sway logic remains active.

## Test D — Moderate collision
1. With condition near 100%, make a controlled collision above roughly 12 km/h but below the severe-impact range.
2. Capture `NATIVE_IMPACT_DAMAGE`.

Expected: condition decreases once for the impact; a single physics contact must not apply many duplicate damage events due to the 0.30 s impact cooldown.

## Test E — Severe collision / tires
1. Repeat with a controlled impact above roughly 34 km/h.
2. Compare condition and `TireIntegrity` before/after.

Expected: both vehicle condition and tire integrity decrease. Repeated legitimate later impacts can add damage, but one contact burst is cooldown-limited.

## Test F — Broken-down safety
1. Reduce condition to zero through controlled test damage.
2. Attempt to accelerate.

Expected: Native throttle and steering are forced to zero and full braking is applied; the shared mirror/save state reflects the damaged condition.

## Regression checks
- Leave all mud volumes and drive on normal road: `GetNativeMudSeverity()` must return zero after the hold window and `GetNativeTerrainGripFactor()` must return to the dry/tire-health baseline.
- Quick-save / quick-load and workshop/service flows must retain authoritative condition, tire integrity and upgrades through the existing migration mirror.
- Legacy Fieldmaster behavior in `GTTMudZone` must remain unchanged.

## Demo gate
Do not publish a demo from source evidence alone. Demo still requires a verified UE 5.8 Win64 package, successful packaged-EXE smoke test, rendered visual acceptance, green relevant GitHub Actions and no demo-critical blockers.
