# GTT 0.0.61 Playtest — Native Per-Wheel Load & Cross-Axle Control

## Goal
Verify that the Native Rusty Fieldmaster 60 turns four-wheel contact/load evidence into meaningful low/medium-speed traction behavior on ruts, side slopes, mud and heavy-haul terrain without fighting the existing hill-start or high-speed stability systems.

## Required runtime
This playtest requires a real Unreal Engine 5.8 runtime with an accepted Native Fieldmaster rig. Repository sanity checks verify source contracts only and do not replace Win64 compile/package/runtime acceptance.

## Test A — Flat dry baseline
1. Use healthy tires, no trailer and a flat dry road.
2. Drive from 0 to roughly 25 km/h with moderate throttle.
3. Capture `NATIVE_WHEEL_LOAD_EVIDENCE`.

Expected: four contacts are normally present, normalized wheel loads stay near one another, cross-axle risk remains low and control stays inactive.

## Test B — One-side rut / ditch
1. Place the left wheels in a shallow rut while the right wheels remain on higher ground.
2. Apply moderate throttle between roughly 5 and 20 km/h.
3. Hold the asymmetry longer than the cross-axle grace period.

Expected: left/right load imbalance and at least one axle split increase. `cross_axle_risk` rises, throttle is progressively limited and a small brake intervention can appear. Severe unloading may temporarily zero steering until four-wheel support recovers.

## Test C — Diagonal articulation
1. Cross a diagonal mound so one front wheel and the opposite rear wheel unload relative to their axle mates.
2. Maintain low speed and steady throttle.

Expected: front/rear cross-axle imbalance reflects the diagonal articulation. The controller reduces drive before the tractor spins or tips aggressively, then releases after load balance returns.

## Test D — Mud + worn tires
1. Repeat the rut/diagonal tests inside an authored `GTTMudZone` with partially worn tires.
2. Repeat again after a tire upgrade.

Expected: terrain grip and tire health reduce front/rear axle grip, causing earlier/stronger intervention. Tire upgrades improve the margin but do not erase poor load distribution or mud consequences.

## Test E — Heavy Timber Haul
1. Attach and load the heavy timber trailer.
2. Drive over uneven ground and a side slope below the higher-speed sway regime.
3. Compare front/rear axle grip and cross-axle risk with the unloaded tractor.

Expected: tongue load shifts usable grip rearward and can reduce front-axle authority. Per-wheel load control works together with existing tow-load, hill-start and sway systems rather than creating a separate trailer state.

## Test F — Contact loss
1. Safely unload one wheel over a crest or deep rut, then briefly reach two-wheel support.
2. Keep speed below 26 km/h.

Expected: grounded-wheel count falls, contact risk contributes to `cross_axle_risk`, throttle authority drops, and a severe two-contact case can suppress steering. Recovery restores normal control without a persistent latch.

## Test G — Hill-start regression
1. Repeat the 0.0.60 loaded muddy hill-start and rollback scenarios.

Expected: `NATIVE_TERRAIN_LOAD_EVIDENCE` remains valid, rollback braking still wins when active, and cross-axle control does not override the hold brake.

## Test H — High-speed ownership boundary
1. Accelerate above 26 km/h on a stable road.

Expected: this low/medium-speed cross-axle controller stops adding intervention; the existing Native traction/stability subsystem remains responsible for higher-speed behavior.

## Demo gate
Do not publish a demo from source evidence alone. Demo still requires a verified UE 5.8 Win64 package, successful packaged-EXE smoke test, rendered visual acceptance, green relevant GitHub Actions and no demo-critical blockers.
