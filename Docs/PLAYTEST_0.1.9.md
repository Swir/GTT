# GTT 0.1.9 Playtest — Reserved Contract Queue & Dispatcher Favors

This milestone converts Feed Dispatcher negotiation into a real stock-backed commitment system. The test target is not just whether a label changes: depot stock must be protected, save/load must preserve the queue, Farm CARGO must consume the protected load without a second debit, and missed holds must have a market consequence. Repository/source sanity is not a substitute for a UE 5.8 Win64 package/runtime test.

## 1. New-driver single reservation
1. Use a fresh/low-history profile during the 07:00–17:30 Feed Depot shift.
2. Note free Feed Depot stock.
3. Interact with Feed Dispatcher once.
4. Re-open the CARGO board/status.

Expected: the compact `E NEGOTIATE` hook remains. One T1-capable hold is created if stock/demand permits, `Q1/1` is visible, free depot stock drops immediately by the protected load size, and the detailed message reports the 35-minute standard hold. The reservation is not a text-only tier.

## 2. Stock debit and no double debit
1. Create one reservation and record the free stock after the dispatcher protects it.
2. Start the corresponding Farm CARGO contract from the normal contract path.
3. Record stock again immediately after acceptance.

Expected: the reserved units were already removed when the desk hold was made. Starting the matching job consumes the queue entry and reports that it used protected stock **without a second stock debit**. The existing job then owns the units and failure can still lose them.

## 3. Queue order is gameplay-authoritative
1. With enough relationship, stock and destination demand, reserve two loads.
2. Observe the board's active CARGO tier.
3. Start the first load, then finish or fail it.
4. Return to the board.

Expected: the oldest protected tier is the active contract. After it is consumed, the second protected tier becomes the next active contract automatically through the existing `GetActiveCargoOrderTier()` / FarmJobDirector path.

## 4. Relationship favor progression
1. Test Feed relationship in `NEW DRIVER`, `KNOWN`, `TRUSTED` and `PREFERRED`.
2. Interact with the dispatcher during a well-stocked shift.
3. Compare capacity and hold information.

Expected: NEW gets 35 minutes and one slot; KNOWN gets 50 minutes and one slot; TRUSTED gets 80 minutes and two slots; PREFERRED gets 110 minutes and two slots. Existing desk-access T1/T2/T3 rules still apply independently, so queue favors never unlock a tier that reputation/relationship/market rules forbid.

## 5. Two-slot trusted queue
1. Reach TRUSTED or PREFERRED Feed relationship.
2. Ensure at least two fulfillable orders worth of free stock and destination demand.
3. Negotiate twice.

Expected: two actual stock commitments can be created (`Q2/2`). Each reservation debits its own units. A third attempt is refused as queue-full and cannot over-reserve stock.

## 6. Expiry consequence
1. Reserve a load with enough time left in the shift.
2. Record stock and backlog pressure.
3. Advance world time past the reservation expiry without taking the job.
4. Trigger a board/dispatcher query.

Expected: the expired entry disappears, its units return to free stock exactly once, and backlog pressure increases once because the dispatcher protected scarce stock for a missed pickup. Repeated queries must not repeatedly refund stock or repeatedly increase backlog.

## 7. Closing-time clamp
1. Visit Feed Dispatcher shortly before 17:30 with a high relationship that normally grants an 80/110-minute favor.
2. Reserve an order.

Expected: expiry never extends beyond 17:30. If too little shift time remains, another hold is rejected. Reservation favors do not create an after-hours legal-work loophole.

## 8. Queue persistence
1. Reserve one or two orders and note tier/unit/expiry summaries plus free stock.
2. Save and reload during the same world day before expiry.
3. Revisit Feed Dispatcher and the board.

Expected: queue count/order, protected units, expiry hours and already-debited free stock match the pre-save state. No stock is duplicated on load. The saved queue is aligned and limited to two entries.

## 9. Next-day stale reservation cleanup
1. Save with an uncollected reservation.
2. Advance/load into a later world day.
3. Query the market.

Expected: stale reservation stock is released before the new-day market is settled, the missed commitment contributes backlog/demand pressure, the old queue is gone, and a fresh day's negotiation can proceed. Same-day reservation state never silently survives indefinitely.

## 10. Existing T1/T2/T3 physical behavior
1. Run a reserved T1, T2 and T3 when each is legitimately unlocked.
2. Observe route, load units, Mulebox handling and payout.

Expected: T1 remains the 2-unit Hill direct run. T2 remains the 3-unit Hill → Wood relay with its existing route bonus. T3 remains the 4-unit bulk chain and retains the 1.20 cargo-load handling factor. Reservation does not replace or fake the established physical/risk loop.

## 11. Police, warden, fleet and relationship regression
1. Test a valid reservation while wanted, under game-warden alert and with an underprepared Mulebox.
2. Test after Feed relationship drops below the required desk tier.

Expected: reservation never bypasses legal-work locks, vehicle preparation/readiness, destination demand or desk-access trust. Existing police/warden consequences and buyer handoff rules remain authoritative.

## 12. Dispatcher presentation regression
1. Inspect Feed, Hill and Wood world labels.
2. Talk to all three on shift and off shift.

Expected: `E NEGOTIATE | DESK Tn`, `HILL NEED n | E STATUS` and `WOOD NEED n | E STATUS` remain recognizable and compact. Detailed messages may show queue/favor data, but the world is not turned back into a wall of debug text.

## 13. Project sanity / Roadmap lock
Run Project sanity and confirm `verify_reserved_contract_queue.py` plus all earlier logistics, fleet, save, dispatcher and demo-gate verifiers pass. Re-read `Docs/ROADMAP.md`.

Expected: `SWIR-ROADMAP-STANDARD:v1` remains intact; checklist truth remains 125/130 = 96.2%, Remaining 5, with the 20-segment `███████████████████░` bar. No hardware/build/asset checkbox is closed by this source milestone.

## 14. Win64/demo acceptance boundary
Only on a real Unreal-capable Windows environment: compile/package UE 5.8, launch the packaged EXE, perform two-reservation save/reload, let a hold expire under live day/night time, run the protected load, and visually inspect the compact dispatcher/board presentation.

Expected for this source milestone: repository sanity may pass. **Do not** report Win64/package/runtime/visual PASS from source checks. The first public demo remains blocked until the exact candidate has a real Win64 package, packaged-EXE runtime smoke, green relevant Actions and rendered visual acceptance.
