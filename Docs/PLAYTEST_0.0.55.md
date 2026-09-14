# GTT 0.0.55 — Native Stability & Contact Control

## Goal
Turn the wheel-ground evidence added in 0.0.54 into a gameplay-facing stability layer for the Native Fieldmaster, so partial wheel contact and dangerous body attitude can affect real driving rather than existing only in logs.

## What changed
- Added `UGTTNativeStabilitySubsystem`, active only during an accepted Native Fieldmaster takeover.
- Samples the four canonical Fieldmaster wheel bones against the world every tick using the same authored rig contract as Native Physics acceptance.
- Computes a stability risk from roll, pitch, wheel-support count and speed.
- Sustained 0–2 wheel contact at road speed cuts throttle and applies proportional braking after a short grace interval, avoiding intervention on a single harmless bump frame.
- Severe roll/pitch or one-wheel-support states also neutralize steering to reduce rollover amplification.
- Existing tire upgrade level slightly reduces intervention strength, connecting the system to real tuning progression instead of a duplicate stability stat.
- Emits `NATIVE_STABILITY_EVIDENCE` with speed, 4-wheel contact count, clearances, roll, pitch, risk, low-contact duration, intervention state and brake strength.

## Runtime playtest
1. Launch UE 5.8 with an accepted authored Native Fieldmaster rig and activate native takeover.
2. Drive straight on flat road. Confirm four contacts are normally reported and stability intervention remains off.
3. Cross a shallow ditch or curb diagonally. Confirm a brief contact-count dip does not immediately apply braking before the grace interval.
4. Drive across uneven farm terrain at moderate speed until two wheel probes remain unsupported for more than the grace period. Confirm throttle is cut and controlled braking appears.
5. Create a safe test slope and increase body roll. Confirm risk rises progressively and intervention begins before the severe-attitude threshold.
6. In a controlled editor-only rollover test, exceed the severe roll/pitch threshold. Confirm throttle is cut, braking is applied and steering is neutralized while the dangerous attitude persists.
7. Repeat with tire tune levels 0 and 3. Confirm the upgraded state modestly reduces brake intervention without bypassing severe rollover protection.
8. Attach the heavy-haul trailer and repeat low-speed uneven-terrain tests. Confirm the subsystem remains tied to the active Fieldmaster and does not alter trailer cargo/condition state.
9. Verify legacy Fieldmaster, Rattleback 82 and Mulebox 1200 remain unchanged when native takeover is inactive.

## Acceptance limits
- Project sanity proves source structure and documentation consistency only.
- This does not close `Dedicated native Chaos drivetrain/suspension/wheel setup` without actual UE runtime handling evidence from the authored rig.
- Demo remains blocked until a verified Win64 package, packaged EXE runtime smoke, rendered visual acceptance and green relevant Actions exist.
