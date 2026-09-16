# GTT 0.1.5 Playtest — Rural Dispatchers, Depot Stock & Contract Rotation

This milestone turns the existing CARGO chain into a persistent local market. Use a profile with access to the Mulebox 1200 and the unified Contract Board. The source/sanity checks below do not substitute for a real UE 5.8 Win64 package/runtime test.

## 1. Staffed depot and live stock
1. Set the world time between 07:00 and 17:30.
2. Visit the Feed Depot CARGO board/pickup, Hill Farm handoff, and North Wood Yard final handoff.
3. Confirm the purpose-built Feed Dispatcher, Hill Receiver, and Wood Foreman are at their work locations.
4. Confirm the board shows the current rotating commodity plus `DEPOT`, `HILL NEED`, and `WOOD NEED` values.
5. Confirm the existing vehicle readiness, preparation price, market multiplier, reputation tier, schedule, and recent-history lines still render.

Expected: staffing and the market use the same rural work-day clock; no separate fake logistics clock is introduced.

## 2. Contract reservation consumes real depot inventory
1. Note the Feed Depot stock on the board.
2. Accept a T1 contract as a newcomer, or a T2/T3 contract on a profile with the required reputation.
3. Re-read the board/market state after acceptance.

Expected: T1 reserves 2 load units; T2/T3 reserves 3. The reservation is saved immediately and the active objective names the exact commodity and reserved amount. Reloading must not duplicate the reserved stock.

## 3. Successful delivery settles destination demand
1. Complete a T1 Feed Depot → Hill Farm delivery.
2. Compare Hill Farm demand before/after.
3. On a TRUSTED/RELIABLE profile, complete the extended Hill Farm relay → North Wood Yard chain.
4. Compare both destination-demand values.

Expected: a successful T1 reduces Hill Farm demand by the reserved units. An extended chain consumes part of the demand at Hill Farm and the remainder at North Wood Yard. Reputation, streak, history, revenue, payout and vehicle-state rules from 0.1.4 still update normally.

## 4. Failed load is not magically returned
1. Accept a load and destroy the cargo or let the delivery timer expire after pickup.
2. Inspect the market after the failure.

Expected: the reserved Feed Depot inventory stays consumed because the cargo was lost, while destination demand remains open. The existing reputation/streak failure consequence is saved in the same transaction.

## 5. Daily restock and contract rotation
1. Record the current commodity, stock and both destination-demand values.
2. Advance to the next in-game day through the existing day/night system.
3. Re-open the Contract Board.

Expected: Feed Depot restocks within its cap, buyer demand is deterministically refreshed from the new day and logistics history, and the commodity rotation can advance among ANIMAL FEED / SEED PALLETS / FARM PARTS. The market is deterministic for a given save/day rather than random on every board refresh.

## 6. NPC-aligned depot schedule
1. Observe the dispatchers just before and after 17:30, then again around 07:00.
2. Try to accept a CARGO contract outside the staffed window.
3. Use the existing PREP path while the depot is closed.

Expected: dispatchers leave their work posts off shift and return for 07:00–17:30. Contract acceptance remains closed while vehicle preparation stays available, matching the existing citizen work schedule.

## 7. Sold-out / satisfied-market behavior
1. Consume enough same-day depot stock or satisfy one destination's demand through repeated valid deliveries.
2. Revisit the CARGO board.

Expected: the board changes to a no-load state and explains the current stock/demand instead of offering an impossible contract. The next daily restock/rotation can reopen work.

## 8. Police and fleet regressions
1. Start a valid CARGO run in the Mulebox and trigger wanted before Hill Farm or North Wood Yard.
2. Verify legal staff still refuse the handoff while the timer continues.
3. Clear wanted and finish before expiry.
4. Repeat with an underprepared Mulebox to verify existing 10%/25% preparation penalties still apply.

Expected: 0.1.5 market logic does not bypass wanted gating, Mulebox physical cargo load, Native/legacy vehicle state, fleet readiness, or payout penalties.

## 9. Save compatibility
1. Load an existing schema-v8 profile created before 0.1.5.
2. Open the CARGO board, save, reload, and inspect the market again.

Expected: old v8 logistics reputation/history remain intact; missing market fields receive safe bounded defaults and become persistent after the next save. No save-version bump is required for this additive state.

## 10. Win64/demo acceptance boundary
Run the repository sanity suite, then—only on an Unreal-capable Windows runner—compile/package UE 5.8, launch the packaged EXE, exercise the market loop above, and visually inspect dispatcher/world/board presentation.

Expected for this source milestone: Project sanity can pass. **Do not** treat that as a packaged Win64/runtime/visual PASS. The demo remains blocked until the repository's real build, packaged smoke, and rendered visual-acceptance gates are actually satisfied.
