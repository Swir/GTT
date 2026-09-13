# GTT 0.0.50 — Heavy-Haul Roadside Recovery

## Milestone goal
Keep severe trailer damage inside the live heavy-haul gameplay loop instead of forcing a full contract reset: the player can pay for an imperfect roadside repair, recover broken wheel constraints, lose contract time, and continue with the same cargo state.

## Roadside repair gameplay
1. Start Heavy Timber Haul, hitch the Fieldmaster and damage the trailer until integrity drops or one axle constraint breaks.
2. Approach within roughly 6.5 m of the trailer and invoke the heavy-haul roadside repair action.
3. Confirm cash is charged, the broken wheel is returned to its axle position, collision/physics are active again and the axle constraint is recreated.
4. Confirm trailer integrity improves but is capped at 85%, preserving consequences from the crash.
5. Confirm existing cargo integrity is not reset to 100% and the current contract stage is preserved.
6. Confirm the repair costs contract time (22 seconds), creating a genuine delivery-risk tradeoff.

## Economy / abuse protection
1. Attempt repair with insufficient cash and confirm no trailer mutation occurs.
2. Repair the trailer more than once and confirm the price increases each time.
3. Break two wheels and verify the quote is higher than for body-only damage.
4. Attempt a repair on a healthy trailer and confirm no cash is taken.
5. Force a repair failure path and confirm the charge is refunded.

## Hitch / Native Fieldmaster regression
1. Perform a repair while previously attached to the legacy Fieldmaster and confirm the system attempts to restore the hitch after axle recovery.
2. Repeat with the active Native Fieldmaster takeover and confirm native `rear_hitch` routing is still used.
3. Break the hitch separately and confirm normal re-hitch gameplay still works.
4. Finish the contract after a roadside repair and verify payout still uses actual cargo/trailer/tractor condition.

## Contract reset regression
1. Finish or fail a contract and start another one.
2. Confirm roadside repair count resets to zero and the new trailer starts at full integrity with both wheel constraints restored.
3. Confirm Rattleback, Mulebox and unrelated jobs are unaffected.

## DEMO / acceptance gate
This is a source-level gameplay milestone verified by repository sanity checks. It does not prove final authored trailer visuals, UE 5.8 Native Chaos runtime, packaged Win64 execution or rendered visual quality. Do not create a demo Release until the existing runtime and visual gates are genuinely satisfied.
