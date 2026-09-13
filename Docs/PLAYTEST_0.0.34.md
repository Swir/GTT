# GTT 0.0.34 — Chaos Fleet Runtime Bridge Playtest

This milestone expands the gameplay-aware Chaos migration bridge from the Rusty Fieldmaster 60 to the full owned vehicle fleet: Rattleback 82 and Mulebox 1200. It deliberately preserves the current source-driven dynamics until each vehicle has a real UE 5.8 skeletal/physics rig and native Chaos movement component.

## Fleet wiring now

- Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200 each own `UGTTChaosVehicleBridgeComponent`.
- All three mirror the existing `VehicleThrottle` and `VehicleSteer` axes into the bridge while still calling the proven base vehicle input path.
- Each bridge resolves its canonical profile using the existing persistent vehicle ID (`RustyFieldmaster60`, `Rattleback82`, `Mulebox1200`).
- Fuel, engine-running state, vehicle condition, engine tuning, tire integrity and tire tuning remain the authoritative gameplay state used by the future native movement path.
- A detected native skeletal + Chaos movement rig disables legacy `UGTTVehicleDynamicsComponent` ticking to prevent double-driving.
- Without those native prerequisites, the bridge remains in `WAITING` and legacy gameplay stays authoritative.

## Current fallback acceptance

For each of the three vehicles:

1. Enter the vehicle in the current source-only greybox build.
2. Accelerate, reverse and steer with keyboard and controller mappings.
3. Verify fuel burn, engine-off behavior, damage power loss, tire damage, tuning, mud/off-road behavior and garage save/load remain functional.
4. Verify no vehicle becomes immobile merely because the Chaos bridge exists.
5. Exit/re-enter and recall the vehicle from a garage slot.
6. Confirm the bridge remains `WAITING` while no native skeletal/physics rig exists.

## Native-rig acceptance when assets exist

Repeat independently for Fieldmaster, Rattleback and Mulebox:

1. Attach an authored skeletal vehicle body, physics asset and `UChaosWheeledVehicleMovementComponent` configured from the canonical vehicle spec.
2. Confirm bridge transition `WAITING -> READY -> DRIVING` when the vehicle is occupied and its engine is running.
3. Confirm legacy dynamics tick disables once, preventing simultaneous force application.
4. Validate forward/reverse gears, braking and steering.
5. Damage the vehicle and verify lower condition reduces effective drive power.
6. Damage tires and verify lower tire integrity reduces effective steering/grip authority.
7. Install engine/tire upgrades and verify the same persisted tuning state affects the native path.
8. Exhaust fuel and verify drive input is gated and braking is applied.
9. Save/reload and verify the same persistent vehicle identity and gameplay state return.

## Cross-system regression

- Fieldmaster heavy trailer/tow behavior must remain usable on fallback until native hitch/socket acceptance exists.
- Rattleback workshop performance/visual packages must remain tied to the same persisted upgrade levels.
- Mulebox cargo/farm-job use must retain its durability, fuel and ownership behavior.
- Theft heat, wanted response, garage recall, repair/refuel, body damage and radio must remain unchanged.
- No GTA or third-party vehicle assets/code are introduced.

## Runtime boundary

Repository sanity can prove fleet wiring, persistent-ID/spec consistency and fallback protection. It cannot prove wheel contact, suspension tuning, skeletal physics or packaged Win64 behavior. Therefore the Native Chaos roadmap tasks, authored trailer wheel/hitch task, full Unreal compile/package smoke test and Win64 runner remain open. No packaged EXE verification is claimed for 0.0.34.
