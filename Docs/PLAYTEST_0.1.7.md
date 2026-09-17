# GTT 0.1.7 Playtest — Interactive Dispatchers & Rural Order Negotiation

This milestone turns the scheduled rural logistics workers into gameplay-facing dispatchers and lets the player choose among genuinely available CARGO risk/reward tiers. Use a profile with the Mulebox 1200, preferably RELIABLE reputation for the full T1/T2/T3 choice set. Repository/source sanity is not a substitute for a real UE 5.8 Win64 package/runtime test.

## 1. Feed Dispatcher negotiation
1. Set the world clock between 07:00 and 17:30.
2. Walk to the Feed Dispatcher at the Feed Depot.
3. Confirm the interaction prompt says `Negotiate CARGO order` and the world label shows `E NEGOTIATE`.
4. Interact repeatedly.

Expected: each press cycles deterministically through the currently fulfillable tiers. The message explains the selected tier, units, route, route bonus/load risk, the market-recommended tier and the available option set. The selection is not random on repeated board refreshes.

## 2. T1 direct is the low-risk choice
1. On a profile capable of higher tiers, negotiate until T1 is selected.
2. Re-open the Contract Board and confirm `ORDER T1`, 2 units and Hill Farm direct priority/route.
3. Accept, load and complete the run.

Expected: the existing short direct route is used, without the T2/T3 chain bonus and without the T3 1.20 heavy-load factor. This is the quickest/lowest-load option when the player prefers safety over maximum payout.

## 3. T2 relay is the balanced choice
1. Ensure Hill Farm and North Wood Yard both have demand and at least 3 depot units exist.
2. Negotiate T2.
3. Confirm the board shows T2 and 3 units.
4. Complete Feed Depot -> Hill Farm -> North Wood Yard.

Expected: the existing single-timer relay remains active and the +$70 trusted chain bonus is available. Wanted still blocks legal handoffs while the timer continues.

## 4. T3 bulk is the high-risk choice
1. Use RELIABLE reputation, at least 4 depot units and positive Hill/Wood demand.
2. Negotiate T3.
3. Load the Mulebox and compare handling with T1/T2.
4. Complete the full chain.

Expected: T3 reserves 4 units, keeps the existing +$120 reliable chain bonus and applies the real 1.20 cargo-load factor to Native/legacy Mulebox handling. The existing extra bulk-route time remains in force. The choice is therefore physically different, not only a text/reward toggle.

## 5. Invalid choices fall back safely
1. Negotiate a T3 order.
2. Change the authoritative market so fewer than 4 depot units remain or Wood demand reaches zero before starting another run.
3. Re-open the board or talk to the dispatcher again.

Expected: the stale T3 selection is rejected automatically and the active order falls back to a currently fulfillable market recommendation/tier. The board must never offer a negotiated load the same stock/demand state cannot reserve.

## 6. Dispatcher negotiation save/reload
1. Negotiate a non-default tier.
2. Save and reload on the same world day.
3. Re-open the Contract Board.

Expected: `CargoNegotiatedOrderTier` and `CargoNegotiationDay` restore the same valid choice. Existing reputation, stock, demand, backlog, history and fleet state remain intact because SaveVersion stays 8 with additive defaults.

## 7. Selection expires on the next work day
1. Negotiate a tier but do not accept it.
2. Advance the existing day/night system into the next world day.
3. Visit the Feed Dispatcher/Contract Board after the new shift starts.

Expected: the old choice is cleared before the new day's restock/demand picture is used. The board returns to the living-market recommendation until the player negotiates again.

## 8. Completed/failed run clears the deal
1. Negotiate and complete one CARGO order, then inspect the next offer.
2. Repeat with a negotiated order that fails from timeout or destroyed cargo.

Expected: both success and failure clear the one-run negotiation. Reputation/backlog/history consequences still apply exactly as before, and the next contract is freshly market-driven unless the player talks to the dispatcher again.

## 9. Hill Receiver and Wood Foreman are useful interactions
1. Talk to Hill Receiver during the staffed shift.
2. Talk to Wood Foreman during the staffed shift.
3. Repeat outside 07:00–17:30.

Expected: Hill reports live Hill demand, selected order and commodity. Wood reports Wood demand, backlog and the shared ROAD/CARGO supply signal. Off shift, staff report that they return at 07:00 and cannot change negotiation state.

## 10. Regression and police/fleet checks
1. Run T1, T2 and T3 with the Mulebox Native and legacy paths where available.
2. Trigger wanted at a handoff and confirm refusal remains.
3. Repeat with underprepared fleet state.
4. Verify Contract Board prep, payout penalties, stock reservation, failed-load loss, backlog, history and save behavior.

Expected: negotiation only selects among existing authoritative routes; it does not bypass police/warden locks, fleet readiness, inventory reservation, cargo integrity or market consequences.

## 11. Roadmap/style sanity
Run Project sanity and confirm `Scripts/verify_interactive_dispatch_negotiation.py` passes together with every earlier logistics/fleet verifier. Re-read `Docs/ROADMAP.md`.

Expected: `SWIR-ROADMAP-STANDARD:v1` is intact and the exact checklist remains 125/130 = 96.2%. This milestone must not claim Native Chaos authored-asset, trailer or Win64 build gates.

## 12. Win64/demo acceptance boundary
Only on a real Unreal-capable Windows runner: compile/package UE 5.8, launch the packaged EXE, exercise dispatcher interaction plus T1/T2/T3 selection, save/reload, next-day reset, police refusal and Native Mulebox load handling, then visually inspect staff prompts, board readability and world presentation.

Expected for this source milestone: Project sanity may pass. **Do not** report Win64/package/runtime/visual PASS from source checks. The first public demo remains blocked until the exact candidate has a real Win64 package, packaged-EXE smoke, green relevant Actions and rendered visual acceptance.
