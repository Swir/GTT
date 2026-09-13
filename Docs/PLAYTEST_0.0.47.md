# GTT 0.0.47 — Native Fieldmaster Terrain & Heavy-Haul Integration

## Milestone goal

Prove that the Native Fieldmaster takeover participates in the same countryside gameplay as the legacy tractor instead of bypassing mud, tire wear and the articulated heavy-haul contract.

## Native mud / terrain test

1. Own the Rusty Fieldmaster 60 and allow Native Chaos takeover to activate.
2. Enter the Native Fieldmaster and drive across a normal road surface, then through each authored `GTTMudZone` used by farm/countryside routes.
3. Confirm mud visibly bleeds speed and reduces effective throttle/traction rather than behaving like asphalt.
4. Repeat with tire upgrade levels 0 and 3. Upgraded tires must retain more effective traction and lose integrity more slowly.
5. Drive through mud above roughly 9 km/h for a sustained period and verify tire integrity drops.
6. Exit the vehicle, quick-save/load and use the workshop/tuning terminals. The degraded tire/condition state must survive through the legacy compatibility mirror.

## Native impact damage contract

1. With Native takeover active, call/trigger `ApplyNativeImpactDamage` from collision gameplay at low and high impact speeds.
2. Impacts below the minimum threshold must not produce fake body damage.
3. Higher impacts must reduce condition; severe impacts must also reduce tire integrity.
4. Workshop repairs must restore the state through the existing economy path rather than a separate Native-only economy.

## Native trailer hitch

1. Start HEAVY TIMBER HAUL with the owned Native Fieldmaster near the Player Farm trailer yard.
2. Park the Native Fieldmaster rear hitch within the normal hitch radius.
3. Use the hitch interaction and verify the trailer constrains directly to the Native Fieldmaster skeletal mesh at the authored `rear_hitch` socket.
4. Verify the hidden legacy Fieldmaster mirror is not selected as the tow vehicle.
5. Tow the empty trailer to NORTH WOOD YARD, load timber, then deliver to HILL FARM.
6. Confirm hitch-load telemetry changes under articulation and the hitch can still break/re-hitch when stretched beyond the existing safety envelope.
7. Confirm cargo instability and trailer integrity continue to affect the payout.
8. Confirm Native Fieldmaster condition contributes to the heavy-haul condition payout factor.

## Regression matrix

- Legacy Fieldmaster remains able to hitch and complete heavy haul when Native takeover is unavailable.
- Rattleback and Mulebox remain unaffected by the new native-only path.
- Garage recall, quick-save/load, radio, workshop, tuning, wanted state and player economy still work while Native takeover is active.
- Mud still affects legacy vehicles through `ApplyTerrainDynamicsModifier` and physical drag.
- Native takeover never activates without the existing rig/wheels/powertrain/PhysicsAsset acceptance gates.

## DEMO / Win64 gate

This milestone is source-level integration only. It does **not** prove Native Chaos runtime acceptance, visual quality, packaged Win64 stability or EXE smoke success. Do not publish a demo from 0.0.47 unless the existing Win64 evidence workflow has produced a verified package/runtime artifact and a separate rendered visual playtest has passed.
