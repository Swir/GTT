# GTT 0.0.48 — Native Fieldmaster Collision & Breakdown Integration

## Milestone goal

Connect real Unreal collision notifications from the active Native Fieldmaster to the shared condition/tire state, breakdown response and legacy compatibility mirror.

## Collision damage test

1. Own the Rusty Fieldmaster 60 and allow Native Chaos takeover to activate.
2. Enter the Native Fieldmaster and strike a static obstacle below roughly 12 km/h. Confirm no meaningful collision damage is applied.
3. Repeat at moderate speed. Confirm condition decreases once for the impact rather than being repeatedly drained by the same contact manifold.
4. Repeat above roughly 34 km/h. Confirm body condition decreases and tire integrity can also degrade.
5. Verify rapid repeated physics hit notifications within the short impact cooldown do not multiply one collision into many damage events.

## Breakdown behavior

1. Reduce Native Fieldmaster condition to zero through repeated severe impacts.
2. Confirm throttle and steering are cleared and Chaos braking is applied.
3. Exit and re-enter: the broken tractor must remain unusable until serviced.
4. Use the normal workshop repair path and confirm the repaired state synchronizes back into the Native Fieldmaster.

## Persistence / economy regression

1. Damage the Native Fieldmaster, quick-save, quick-load and confirm condition/tire integrity survive via the existing compatibility mirror.
2. Repair/refuel/tune the vehicle and confirm no duplicate Native-only economy is created.
3. Verify heavy haul payout still observes Native tractor condition.
4. Verify mud wear plus collision damage combine without resetting tire integrity.

## Collision safety matrix

- Contacts while Native takeover is inactive must not mutate the migration snapshot.
- Low-speed parking touches must not cause fake body damage.
- The hidden legacy mirror must not also process the same world collision.
- Collision damage must synchronize to the legacy mirror promptly so save/load and service terminals see the same state.
- Rattleback 82 and Mulebox 1200 remain on their existing damage path.

## DEMO / Win64 gate

This milestone verifies source-level collision integration and CI only. It does **not** prove authored Physics Asset behavior, final collision shapes, UE 5.8 runtime handling, packaged Win64 stability or rendered visual acceptance. Do not publish a demo unless the existing Win64 evidence workflow and rendered playtest both pass.
