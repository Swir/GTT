# GTT 0.0.49 — Trailer Axle & Impact Physics

## Milestone goal

Turn the heavy-haul farm trailer wheels from decorative non-colliding cylinders into constrained rolling rigid bodies that can physically fail, affect cargo stability and participate in the existing trailer-integrity economy.

## Rolling axle test

1. Start Heavy Timber Haul and hook the trailer to Rusty Fieldmaster 60 using the normal hitch flow.
2. Drive slowly on flat road and confirm both trailer wheels contact the ground, roll around their axle and remain laterally constrained to the trailer body.
3. Turn through a tight junction and confirm the trailer articulates at the hitch while both wheel bodies remain attached to the axle.
4. Repeat unloaded and loaded. Confirm loaded body mass remains heavier while wheel mass/collision remain active.
5. Drive over a shallow roadside edge or uneven terrain and verify the wheels react as independent rigid bodies instead of clipping through the ground as presentation-only meshes.

## Axle failure / recovery

1. Subject one trailer wheel to a severe lateral/vertical physics load until its breakable axle constraint fails.
2. Confirm `HasIntactAxle()` becomes false and trailer integrity is capped to the damaged range.
3. Continue with cargo loaded and verify the missing-wheel axle penalty accelerates cargo instability/loss instead of leaving the haul unaffected.
4. If both wheel constraints fail, confirm trailer integrity enters the critical range and the trailer becomes visibly difficult to tow.
5. Reset/restart the trailer and confirm wheel-loss state is cleared and both axle constraints are recreated.

## Trailer collision damage

1. Touch a wall or prop below roughly 18 km/h and confirm no meaningful trailer/cargo penalty.
2. Strike an obstacle at moderate speed and confirm trailer integrity decreases once rather than draining repeatedly from one sustained contact.
3. Strike above roughly 42 km/h while loaded and confirm both trailer integrity and cargo integrity can decrease.
4. Verify the short collision cooldown suppresses duplicate hit callbacks from one contact manifold.

## Heavy-haul gameplay regression

1. Complete North Wood Yard pickup and Hill Farm delivery with an intact trailer; payout behavior must remain unchanged except for any real cargo/trailer damage incurred.
2. Repeat after axle damage and verify the existing condition-sensitive reward reacts to reduced trailer/cargo integrity.
3. Break/re-hook the main hitch and verify axle state survives normal hitch detach/reconnect.
4. Run the same contract with the active Native Fieldmaster and verify `rear_hitch` routing remains functional.
5. Verify Rattleback/Mulebox and unrelated vehicle jobs are unaffected.

## DEMO / acceptance gate

This milestone is source-level gameplay physics plus CI verification. Engine basic-shape meshes are still used as temporary trailer visuals, so the roadmap item for authored skeletal trailer wheel assets/final sockets remains open. It also does not prove UE 5.8 Win64 compile/package, packaged EXE runtime smoke, Physics Asset quality or rendered visual acceptance. Do not publish a demo until the existing Win64 evidence and visual gates pass.
