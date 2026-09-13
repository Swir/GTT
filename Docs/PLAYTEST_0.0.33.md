# GTT 0.0.33 — Fieldmaster Chaos Runtime Bridge Playtest

This milestone moves the Rusty Fieldmaster 60 from a spec-only migration plan to a gameplay-state-aware Chaos runtime bridge. It deliberately keeps the current source-driven drivetrain as the fallback until a real UE 5.8 skeletal vehicle rig is present and validated.

## What is wired now

- The Fieldmaster owns `UGTTChaosVehicleBridgeComponent`.
- Existing `VehicleThrottle` and `VehicleSteer` inputs are mirrored into the bridge without removing the proven fallback controls.
- The bridge resolves the canonical `RustyFieldmaster60` Chaos spec from the existing persistent vehicle ID.
- When a real `UChaosWheeledVehicleMovementComponent` plus skeletal mesh are present, the bridge disables the legacy dynamics tick to prevent double-driving.
- Fuel, engine-running state, condition, engine tuning, tire integrity and tire tuning feed the effective Chaos throttle/steering gates.
- Forward/reverse intent, steering, idle braking and hard stop when the engine/fuel/condition gate fails are routed to the native movement component.
- If the native rig is missing, the current source-driven Fieldmaster remains authoritative and fully playable.

## Fallback acceptance

1. Launch the current greybox build without adding a skeletal Chaos rig.
2. Enter the Rusty Fieldmaster 60 and verify throttle, reverse, steering, fuel burn, damage, tuning, mud, trailer towing, theft heat and save/load continue using the existing vehicle dynamics path.
3. Verify the bridge reports `WAITING` rather than disabling the fallback.
4. Exit/re-enter, recall from garage and quick-save/load; confirm the vehicle remains usable.

## Native-rig acceptance once UE assets exist

1. Add the authored Fieldmaster skeletal mesh + physics asset and a native `UChaosWheeledVehicleMovementComponent` using the canonical 0.0.30 spec.
2. Confirm the bridge changes to `READY`, then `DRIVING` while occupied with a running engine.
3. Confirm the legacy `UGTTVehicleDynamicsComponent` tick disables exactly once so both simulations do not apply force simultaneously.
4. Test forward and reverse, steering, engine-off braking and out-of-fuel stopping.
5. Damage the tractor below 50% and verify available drive power is reduced.
6. Install engine upgrades and verify effective throttle power rises without bypassing fuel/condition gates.
7. Damage tires and install tire upgrades; verify steering authority follows the shared tire state.
8. Save, reload and repeat so the same `RustyFieldmaster60` identity still restores the authoritative sandbox state.

## Regression

- Existing Fieldmaster source-driven drivetrain remains the fallback until the native movement + skeletal body prerequisites are detected.
- Rattleback 82 and Mulebox 1200 remain on their current dynamics path in this milestone.
- Heavy trailer, mud/off-road modifiers, detachable parts and workshop customization must remain functional on the fallback path.
- No GTA or other third-party vehicle assets are introduced.

## Runtime boundary

Repository sanity can verify source wiring and fallback safety, but it cannot prove wheel contact, suspension tuning, skeletal physics, packaged Win64 behavior or real Chaos handling. Therefore both Native Chaos roadmap checkboxes, the full Unreal compile/package smoke test and the Win64 runner remain open. No packaged EXE verification is claimed for 0.0.33.
