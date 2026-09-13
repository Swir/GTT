# Native Chaos Vehicle Migration Plan

GTT 0.0.30 establishes a canonical source contract for migrating the current source-driven vehicle simulation to Unreal Engine 5.8 Chaos Vehicles without throwing away the existing fuel, damage, tuning, tire, mud, ownership, wanted, save/load or workshop gameplay.

## Current boundary

The playable vehicles currently inherit from the GTT `APawn` vehicle base and use runtime `UStaticMeshComponent` greybox bodies plus `UGTTVehicleDynamicsComponent`. This is deliberately retained until proper Chaos vehicle assets can be authored and compiled. A true Chaos vehicle uses `AWheeledVehiclePawn` / `UChaosWheeledVehicleMovementComponent` together with a skeletal mesh, physics asset and wheel setup. Therefore Native Chaos is **not completed** by this milestone and the roadmap checkboxes remain open.

## Canonical target specifications

`GTTChaosVehicleSpec` is now the single migration contract for all three owned/drivable vehicle families:

- `RustyFieldmaster60` — 4WD agricultural gearing, tractor-specific front/rear wheel geometry, high final drive and long-travel suspension target.
- `Rattleback82` — rear-wheel-drive road car target with shorter suspension travel, road-biased friction and five-speed gearing.
- `Mulebox1200` — rear-wheel-drive utility van target with higher loaded mass, commercial suspension and torque-oriented gearing.

The profile includes target mass, engine torque/RPM, idle RPM, final-drive ratio, steering angle, drive layout, front/rear wheel dimensions, suspension raise/drop, spring/damping values, friction multiplier and forward/reverse ratios.

## Migration sequence

1. Author original GTT skeletal chassis meshes for Fieldmaster, Rattleback and Mulebox. Bone/socket naming must be project-owned and documented.
2. Create matching Chaos physics assets and verify chassis center-of-mass and collision primitives.
3. Create front/rear `UChaosVehicleWheel` classes per family using the canonical `FGTTChaosVehicleSpec` values.
4. Introduce an `AWheeledVehiclePawn`-based GTT native vehicle class with `UChaosWheeledVehicleMovementComponent`.
5. Route throttle, steering, brake/reverse and automatic/manual gear commands to Chaos movement input.
6. Reconnect shared gameplay state: condition power loss, engine upgrades, tire integrity, mud grip, overheating, fuel burn, detached panels, police spike strips, garage ownership and save/load.
7. Migrate Fieldmaster first and compare its speed envelope, hill climbing, mud behavior and heavy trailer handling against the current source-driven implementation.
8. Migrate Rattleback and Mulebox after the tractor passes runtime acceptance.
9. Run full UE 5.8 Development and Shipping Win64 compile/package tests and the dedicated driving matrix before changing the two Native Chaos roadmap tasks to `[x]`.

## Acceptance rules

Native Chaos cannot be declared complete from static source inspection alone. Completion requires real skeletal/physics assets, a working Chaos movement pawn, all three vehicle profiles connected to gameplay, successful Unreal compile/package, and runtime driving checks covering fuel, damage, tires, mud, tuning, towing, police spikes, garage save/load and controller input.

Until that happens the existing vehicle simulation remains the stable gameplay path and no packaged EXE verification is claimed.
