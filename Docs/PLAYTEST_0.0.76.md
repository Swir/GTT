# GTT 0.0.76 Playtest — Native Wheel/Axle Traction Authority

## Purpose

Verify that Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200 share one Native Chaos wheel/axle authority that reacts to real `FWheelStatus` contact, slip and suspension evidence without creating artificial grip or bypassing the existing drivetrain, tire, cargo, damage or workshop loops.

## Preconditions

1. Use an accepted Native takeover vehicle with its authored skeletal rig, Physics Asset, canonical wheel setup and canonical powertrain accepted by the existing runtime gates.
2. Keep Output Log visible and filter for `NATIVE_AXLE_TRACTION`.
3. Repeat the tests on the Fieldmaster, Rattleback and Mulebox.
4. For comparison runs, use both healthy tires and deliberately worn tires repaired through the existing workshop loop.

## Scenario A — four-wheel baseline

1. Drive on flat road at moderate speed with four wheels in contact.
2. Confirm `NATIVE_AXLE_TRACTION_EVIDENCE` reports four valid/contact wheels, two front contacts and two rear contacts.
3. Confirm front/rear slip risk remains low, axle imbalance remains small, traction authority stays near 1.0, torque cut is `NO` and brake assist is zero.

**Pass:** normal driving is not penalized simply because the subsystem is active.

## Scenario B — front or rear axle unload

1. Cross a ditch, crest, steep shoulder or obstacle so one axle temporarily loses most/all contact.
2. Confirm front/rear contact counts change from live Chaos wheel state.
3. With an entire axle unsupported, confirm traction authority falls sharply.
4. Above the intervention speed, confirm throttle is cut at severe risk and `NATIVE_AXLE_TRACTION_INTERVENTION` appears.
5. When contact returns, confirm the intervention clears naturally instead of latching.

**Pass:** unsupported axle torque is suppressed instead of continuing to request full propulsion.

## Scenario C — asymmetric left/right loading

1. Traverse a side slope or curb transition with two or more wheels still grounded.
2. Confirm left/right suspension load proxies diverge and `axle_imbalance` rises.
3. Confirm the subsystem only applies a bounded yaw correction while at least two wheels remain grounded.
4. Crest fully airborne and confirm this grounded correction cannot act with inadequate wheel support.

**Pass:** the correction is contact-gated and stabilizing, not airborne auto-leveling.

## Scenario D — slip/skid torque protection

1. Use mud, damaged tires or an aggressive surface transition to produce `bIsSlipping` / `bIsSkidding` evidence.
2. Confirm front/rear slip-risk values rise from `FWheelStatus`.
3. At moderate risk confirm limited brake assist can appear.
4. At severe risk confirm torque cut can remove drive request until contact/slip recovers.
5. Repeat with a tire upgrade and verify the existing upgrade slightly reduces intervention risk rather than creating perfect grip.

**Pass:** the controller reacts to actual Chaos wheel state and preserves the value of tire condition/upgrades.

## Scenario E — existing systems remain connected

### Fieldmaster
- Verify 0.0.75 safe forward/reverse direction interlock still wins during a requested high-speed direction change.
- Verify heavy-haul, terrain, mud, load transfer and rollover systems remain active.
- Verify fuel/critical-condition breakdown still stops the tractor.

### Rattleback / Mulebox
- Verify road-fleet wheel-state risk, local body damage, detached panels, collision consequences and workshop repair remain active.
- Load Mulebox cargo and confirm cargo handling penalties continue upstream of this safety layer.
- Trigger a police/traffic incident and confirm axle traction control does not create a second wanted or damage model.

**Pass:** 0.0.76 is a shared final wheel/axle safety authority, not a replacement gameplay stack.

## Scenario F — persistence/economy regression

1. Wear/damage tires through normal gameplay.
2. Observe stronger traction intervention with degraded tire integrity.
3. Repair at the existing workshop.
4. Confirm normal tire integrity and reduced intervention return after service/save synchronization.

## Demo / roadmap gate

- Run `python Scripts/verify_native_axle_traction.py` and the full Project sanity workflow.
- Roadmap must remain exactly **125/130 (96.2%)** unless genuine Unreal runtime evidence closes one of the five open tasks.
- This source/runtime milestone does **not** prove the remaining Native Chaos checkboxes are complete.
- Do not publish the Windows demo until a real UE 5.8 Win64 package exists, the packaged `GTT.exe` launches and passes runtime smoke testing, rendered visual acceptance is good enough to show publicly, relevant GitHub Actions are green and no demo-critical blocker remains.
