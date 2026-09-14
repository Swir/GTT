# GTT 0.0.67 Playtest — Native Road Fleet Collision & Wheel-State Control

## Scope

This milestone hardens the Native Chaos takeover path for **Rattleback 82** and **Mulebox 1200**. The test is specifically about real driving consequences: Chaos wheel-state traction intervention, collision damage that persists through the legacy/save mirror, tire damage, breakdown behavior, and cargo-linked impact severity.

This document is a runtime acceptance plan. Source-level Project sanity can verify the contract, but it does **not** replace an Unreal Engine 5.8 Win64 package + executable smoke test.

## Preconditions

- Use an authored Rattleback 82 or Mulebox 1200 skeletal vehicle that passes the existing native rig, wheel, powertrain and Physics Asset acceptance gates.
- Own the corresponding legacy garage vehicle so native takeover is allowed.
- Start with condition, fuel and tire integrity visibly below/at known values so persistence can be checked after impacts.
- For Mulebox cargo cases, start the legal farm cargo contract and load the van at the feed depot.

## A. Baseline Native driving

1. Enter Rattleback 82 through the Native takeover.
2. Drive slowly on a flat road without collisions.
3. Confirm steering, throttle and idle braking remain stable.
4. Confirm logs periodically emit `NATIVE_ROAD_WHEEL_STATE_EVIDENCE` with four valid Chaos wheel states.
5. At low risk the reported throttle/steering limits should stay at 1.0 and brake assist at 0.

**Pass:** normal road driving is not penalized merely because the controller exists.

## B. Wheel slip / loss-of-contact intervention

1. Drive one side across a ditch/road shoulder or another authored surface that can unload a wheel.
2. Repeat with worn tires.
3. Observe `contacts`, slipping/skidding wheel counts, slip magnitude/angle and the runtime risk.
4. When risk passes the intervention threshold, verify available throttle is reduced first.
5. At stronger risk verify brake assist appears; at very high risk steering authority is reduced.
6. Repeat with a tire upgrade and confirm the same event is slightly less intrusive.

**Pass:** Rattleback/Mulebox use actual `FWheelStatus` data, not a detached placeholder stat.

## C. Collision damage persistence

1. Record vehicle condition and tire integrity.
2. Hit a rigid world object above the minimum impact threshold.
3. Confirm `NATIVE_ROAD_IMPACT_DAMAGE` includes vehicle id, impact speed, condition delta and tire delta.
4. Exit the vehicle and inspect/service/save through the existing garage loop.
5. Reload or otherwise restore the persistent vehicle state.

**Pass:** the condition loss survives through `SyncLegacyMirror()` and the existing save/service source of truth. No second save model is created.

## D. Severe impact and breakdown

1. Perform a substantially harder collision above the severe-impact threshold.
2. Confirm tire integrity also decreases.
3. Continue only in a controlled test until condition reaches zero.
4. Verify `NATIVE_ROAD_BREAKDOWN` is emitted.
5. Confirm throttle and steering are cut and full braking is requested.
6. Exit, repair through the existing service path, then verify the repaired mirror can be taken over again.

**Pass:** a destroyed Native road vehicle cannot continue driving normally and repair still uses the established economy/workshop loop.

## E. Loaded Mulebox consequence

1. Start the legal farm cargo job and load Mulebox 1200.
2. Verify its existing cargo handling penalty is active.
3. Perform the same controlled impact used in the unloaded comparison.
4. Confirm the loaded van takes the cargo-inertia multiplier into the native impact calculation.
5. Continue the job and verify existing cargo-integrity/reward logic observes the reduced vehicle condition.
6. Finish or fail the contract and confirm cargo state is cleared as before.

**Pass:** collision damage is connected to the legal-job/economy loop rather than being standalone telemetry.

## F. Regression / fallback

- Rattleback and Mulebox takeover still require authored rig, canonical wheels, canonical powertrain and Physics Asset.
- A failed runtime acceptance check still triggers `NATIVE_ROAD_FALLBACK`.
- Hidden legacy mirrors remain non-colliding and non-ticking during takeover.
- Fieldmaster-specific trailer, mud, hill-start and heavy-haul controllers are unaffected.
- Fuel depletion still stops the road vehicle.
- Roadmap runtime-only items remain open until genuine UE/Win64 evidence exists.

## Demo gate

**Do not create a Demo Release from source-level sanity alone.** The demo remains blocked until a genuine UE 5.8 Win64 package is built, the packaged `GTT.exe` passes runtime smoke testing, rendered visual acceptance passes, relevant GitHub Actions are green, and there are no demo-critical blockers.
