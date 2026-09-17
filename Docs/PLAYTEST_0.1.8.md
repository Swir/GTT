# GTT 0.1.8 Playtest — Dispatcher Relationships & Multi-Order Contract Desk

This milestone makes the rural logistics staff remember the player's real delivery performance and turns the Feed Dispatcher into a shared ROAD/CARGO contract desk. Relationship is derived from the already-persistent logistics history, so existing schema-v8 saves should gain a deterministic state immediately. Repository/source sanity is not a substitute for a real UE 5.8 Win64 package/runtime test.

## 1. New-driver desk access
1. Use a fresh/low-history profile during 07:00–17:30.
2. Walk to the Feed Dispatcher.
3. Read the world label and interaction prompt.
4. Interact several times.

Expected: the established `E NEGOTIATE` label remains present and now also shows `DESK T1`, `NEW DRIVER` plus a score. Only manual CARGO choices at the relationship-backed access tier are permitted. The message includes the shared desk state with CARGO options and ROAD open/closed status. The dispatcher does not randomly select a locked higher-tier order.

## 2. Relationship grows from real deliveries
1. Complete several clean CARGO runs without police incidents or destroyed cargo.
2. Revisit Feed Dispatcher, Hill Receiver and Wood Foreman.
3. Compare their relationship labels/scores with the earlier state.

Expected: scores rise deterministically from the persistent CARGO completion/reputation/clean-streak record. Feed and Hill react most strongly to CARGO history. Wood also reflects completed ROAD courier work. No separate relationship save counter is required.

## 3. Failures matter
1. Record the current relationship scores.
2. Fail CARGO runs through timeout/destroyed cargo and, separately, fail or damage ROAD courier performance where practical.
3. Revisit the staff.

Expected: failed work pushes the appropriate relationship scores down. A good global reputation cannot completely hide repeated failures because the formulas include explicit failure penalties.

## 4. T2 trust gate
1. Use a profile whose global logistics reputation already permits T2 but whose Feed Dispatcher relationship is below the T2 desk threshold.
2. Ensure depot stock and Hill/Wood demand would otherwise allow T2.
3. Check the Farm CARGO board, then talk to Feed Dispatcher and cycle orders.

Expected: manual desk access remains T1 and the board uses `BUILD DISPATCHER TRUST` if the active order is above the earned relationship tier. The dispatcher cycles past relationship-locked tiers to a legal choice. Vehicle preparation remains available.

## 5. T2 unlock
1. Build Feed Dispatcher relationship to at least the T2 threshold through successful lower-tier deliveries.
2. Ensure the existing reputation/stock/demand requirements for T2 are also met.
3. Interact with Feed Dispatcher until T2 is selected.
4. Accept the contract from the board.

Expected: T2 becomes a genuine selectable/acceptable relay contract. Existing 3-unit reservation, Hill -> Wood chain, timer, cargo integrity, wanted refusal and +$70 route bonus remain unchanged.

## 6. Preferred-driver T3 access
1. Build Feed Dispatcher relationship into `PREFERRED` while also meeting the existing RELIABLE reputation, 4-unit stock and Hill/Wood demand requirements.
2. Cycle the desk through its options.
3. Select and run T3.

Expected: T3 is now accepted. It still reserves 4 units, keeps the real 1.20 cargo-load handling factor, full relay route and existing +$120 route bonus. Relationship unlocks access; it does not replace the stock/demand/reputation gates or create a text-only fake tier.

## 7. Shared ROAD/CARGO desk
1. Visit Feed Dispatcher and inspect Farm CARGO plus Village Parts Courier boards at different world times.
2. Test before 06:00, during ROAD-only time, during the shared staffed window and after 17:30.

Expected: the compact desk summary consistently reports the currently relationship-accessible CARGO options and the existing ROAD open/closed state. ROAD remains governed by its existing 06:00–21:30 schedule, while CARGO still requires staffed 07:00–17:30 dispatch.

## 8. Hill Receiver relationship reaction
1. Complete clean CARGO deliveries to Hill Farm.
2. Talk to Hill Receiver on shift.
3. Repeat after intentionally failed CARGO work.

Expected: the established `HILL NEED n | E STATUS` label remains present. The receiver reports live Hill demand, current order/commodity and relationship-aware dialogue. Wanted drivers are still refused by the legal handoff path; the relationship layer cannot bypass police rules.

## 9. Wood Foreman mixed-work relationship
1. Complete a mixture of CARGO relay work and Rattleback ROAD courier runs.
2. Talk to Wood Foreman.
3. Compare the result with a profile containing repeated CARGO/ROAD failures.

Expected: the established `WOOD NEED n | E STATUS` label remains present. Wood relationship reflects both transport streams and the status message still includes Wood demand, backlog and the shared ROAD/CARGO supply signal.

## 10. Save/reload determinism
1. Note all three relationship scores plus the current same-day negotiated CARGO order.
2. Save and reload without changing delivery history.
3. Revisit all staff and the boards.

Expected: relationship scores/labels are identical because they are recomputed from persisted authoritative logistics history. The existing 0.1.7 same-day negotiated tier still restores from schema-v8 fields. No new save migration or duplicated relationship counters appear.

## 11. Existing market invalidation remains authoritative
1. Select the highest relationship-accessible CARGO tier.
2. Change stock or destination demand through completed work until that load can no longer be fulfilled.
3. Re-open the board and Feed Dispatcher desk.

Expected: the 0.1.7 living-market validity check still clears/falls back from the stale negotiated order. Relationship never reserves cargo that authoritative stock/demand can no longer supply.

## 12. Fleet/police/regression pass
1. Run T1/T2/T3 where unlocked using Mulebox Native/legacy paths available in the build.
2. Test underprepared fleet state, wanted, game-warden alert and damaged vehicle state.
3. Verify ROAD Rattleback work after the relationship changes.

Expected: relationship gating does not bypass fleet preparation, wanted/warden locks, cargo integrity, Native load handling, stock reservation, backlog, ROAD payout/schedule, garage state or save behavior. Existing 0.1.7 dispatcher prompts/interaction strings remain compatible while the relationship desk adds information around them.

## 13. Roadmap/style sanity
Run Project sanity and confirm `Scripts/verify_dispatcher_relationship_desk.py` passes with all earlier logistics/fleet verifiers. Re-read `Docs/ROADMAP.md`.

Expected: `SWIR-ROADMAP-STANDARD:v1` remains intact and the checklist is still exactly 125/130 = 96.2%, with the 20-segment `███████████████████░` bar and five real build/asset/runtime tasks open.

## 14. Win64/demo acceptance boundary
Only on a real Unreal-capable Windows runner: compile/package UE 5.8, launch the packaged EXE, exercise relationship progression, T1/T2/T3 access, same-day negotiation, save/reload, ROAD/CARGO desk presentation and police/fleet restrictions, then visually inspect dispatcher labels and board readability.

Expected for this source milestone: Project sanity may pass. **Do not** report Win64/package/runtime/visual PASS from source checks. The first public demo remains blocked until the exact candidate has a real Win64 package, packaged-EXE runtime smoke, green relevant Actions and rendered visual acceptance.
