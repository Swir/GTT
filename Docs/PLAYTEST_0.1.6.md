# GTT 0.1.6 Playtest — Dynamic Rural Orders & Supply-Chain Consequences

This milestone makes the 0.1.5 stock/demand market carry consequences across work days and turns reputation tiers into capability rather than a fixed route. Use a profile with the Mulebox 1200 and unified Contract Board. Repository sanity/source checks do not substitute for an actual UE 5.8 Win64 package/runtime test.

## 1. Dynamic order selection from real buyer demand
1. Open the CARGO Contract Board during 07:00–17:30 on a TRUSTED or RELIABLE profile.
2. Record `HILL NEED`, `WOOD NEED`, reputation capability tier, offered ORDER tier, order units and priority label.
3. Deliver direct loads to Hill Farm until Hill demand falls relative to Wood Yard demand, then re-open the board.

Expected: reputation still caps what the player is qualified to run, but the active order changes from a Hill-direct T1 to T2/T3 relay work when Wood Yard pressure becomes more important. The board must show both capability and current order tier so route changes are explainable rather than random.

## 2. T3 bulk order has a physical vehicle consequence
1. Reach RELIABLE reputation with enough depot stock and Wood Yard/backlog pressure to produce ORDER T3.
2. Accept the order and load the Mulebox 1200.
3. Compare acceleration/high-speed steering against a normal T1/T2 load.

Expected: T3 reserves 4 units and applies a 1.20 cargo load factor instead of the ordinary 1.0 load. Existing Native/legacy Mulebox cargo dynamics consume that factor, so the larger order is not merely a bigger text payout. The T3 route receives a small extra delivery-time allowance rather than silently using the lighter-load timing.

## 3. Failed delivery creates persistent backlog pressure
1. Record CARGO backlog on the board.
2. Accept a valid load, then destroy it or let the timer expire.
3. Save/reload before advancing the day.
4. Advance to the next rural work day and inspect the board again.

Expected: the lost depot stock remains lost, buyer demand remains open, `CargoBacklogPressure` survives save/load, and the next-day market adds fresh orders on top of unserved demand rather than resetting demand to unrelated values. Severe failures create more pressure than ordinary failures.

## 4. Successful delivery works down the backlog
1. With non-zero backlog pressure, complete a valid direct CARGO order.
2. Confirm destination demand falls and backlog pressure falls by one.
3. Complete an extended T2/T3 chain and confirm pressure can fall faster.

Expected: successful logistics is the active way to reduce the consequences of prior failed loads. This should create a recoverable feedback loop rather than a permanent punishment.

## 5. Backlog-driven payout remains bounded
1. Build several failed-load backlog points without exceeding the saved cap.
2. Compare CARGO market multiplier before/after.
3. Combine backlog with morning/late demand, reputation and clean streak.

Expected: the old market calculation remains bounded at 1.38x before backlog pressure; backlog can add at most 0.10x and the final CARGO market multiplier is hard-capped at 1.48x. No failure loop should create an unbounded money multiplier.

## 6. ROAD courier reacts to the same supply chain
1. Advance commodity rotation until `FARM PARTS` is active.
2. Build Wood Yard demand/backlog through the CARGO loop.
3. Inspect the ROAD Contract Board and accept the Rattleback parts courier.

Expected: the board reports a Wood-parts backlog supply signal and the existing road-courier reward multiplier receives at most a 10% parts-pressure bonus. On non-FARM-PARTS days it reports a standard parts route. ROAD and CARGO therefore share one village supply state instead of independent fake economies.

## 7. Persistence and schema-v8 compatibility
1. Load a schema-v8 save created before 0.1.6.
2. Open the board, save, reload, fail one CARGO order, save, reload again.

Expected: old reputation/history/market state remains intact. The new backlog field defaults safely to zero, then persists after first use. SaveVersion remains 8 because the field is additive and has a valid default.

## 8. Police, staff and fleet regressions
1. Verify CARGO acceptance is still closed outside 07:00–17:30 while PREP remains available.
2. Trigger wanted during Hill Farm and North Wood Yard handoffs.
3. Clear wanted and finish before expiry.
4. Repeat with an underprepared Mulebox.

Expected: dynamic orders do not bypass dispatcher schedules, wanted refusal, game-warden gating, preparation penalties, cargo integrity, physical Mulebox load or the authoritative save transaction.

## 9. Roadmap/style and source sanity
Run the full Project sanity workflow and confirm `Scripts/verify_dynamic_rural_orders.py` passes together with all prior logistics verifiers. Re-read `Docs/ROADMAP.md` after the change.

Expected: `SWIR-ROADMAP-STANDARD:v1` remains intact and the exact checklist result remains 125/130 = 96.2% because this gameplay milestone does not close any authored-asset/Native Chaos/Win64 build gate.

## 10. Win64/demo acceptance boundary
Only on an Unreal-capable Windows runner: compile/package UE 5.8, launch the packaged EXE, exercise the direct/relay/bulk order rotation plus failure/backlog/save/reload flow, and visually inspect the Contract Board, dispatcher world presence, Mulebox handling and ROAD supply signal.

Expected for this source milestone: Project sanity may pass. **Do not** report packaged Win64/runtime/visual PASS from source checks. The first public demo remains blocked until real packaging, packaged-EXE smoke, green relevant Actions and rendered visual acceptance all pass on the exact candidate artifact.
