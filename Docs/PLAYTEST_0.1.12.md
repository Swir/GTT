# GTT 0.1.12 Playtest — Priority Route Planner & Consequence Ledger

This checklist validates 0.1.12 in a real Unreal Engine 5.8 runtime. Source sanity is not packaged-Windows evidence.

## Preconditions
- Use a profile with Rattleback 82 and Mulebox 1200 owned, with ROAD and CARGO mission loadouts assigned.
- Keep a schema-v8 save from 0.1.11 for compatibility testing.
- Record world day/time, commodity rotation, depot stock, Hill/Wood demand, backlog, clean streak, ROAD/CARGO failures, current reservation queue and both vehicle readiness states.

## Scenarios

1. **Planner consumes live fleet readiness**
   - Put both vehicles in READY state and inspect the Contract Desk summary.
   - Damage or starve one assigned vehicle until its assessment becomes ADVISORY/SERVICE REQUIRED.
   - Its planner score must fall through real readiness/preparation cost; the other lane must not inherit fake damage.

2. **ROAD wins a real emergency conflict**
   - Create FARM PARTS pressure so ROAD and CARGO are both urgent, with Rattleback ready and Mulebox needing preparation.
   - Planner should recommend `ROAD / RATTLEBACK 82` when the score gap is material.
   - The Contract Desk must show both scores, urgency, readiness and prep estimates.

3. **CARGO wins a real emergency conflict**
   - Create high Hill/Wood demand, a protected CARGO load and weaker ROAD pressure/readiness.
   - Planner should recommend `CARGO / MULEBOX 1200`.
   - No cross-lane CARGO hold penalty is allowed when CARGO itself is the stronger lane.

4. **Close conflict exposes a player choice**
   - Tune both urgent lane scores to within eight points.
   - Planner should report `SPLIT / CHOOSE COMMITMENT` rather than pretending one route is clearly superior.
   - Both original contracts must remain independently playable subject to their existing requirements.

5. **ROAD conflict shortens a protected CARGO hold**
   - With both lanes urgent and ROAD ahead by at least 15 points, reserve CARGO at Feed Dispatcher.
   - Existing relationship hold and 0.1.11 emergency SLA still apply first.
   - Effective hold must additionally respect the displayed `ROUTE CONFLICT CAP`.

6. **Conflict cap scales with route-score gap**
   - Exercise score gaps 15+, 30+ and 50+ while both lanes remain urgent and ROAD wins.
   - Expected conflict scales are 75%, 65% and 55% of the existing CARGO priority SLA, rounded to minutes with a 10-minute floor.

7. **STANDARD work never receives a conflict cap**
   - Set either ROAD or CARGO urgency to zero.
   - `GetCargoConflictHoldCapMinutes` behavior must be neutral and the relationship/emergency hold logic from 0.1.11 must remain unchanged.

8. **Reservation feeds back into the planner**
   - Record plan scores, then reserve a real stock-backed CARGO order.
   - CARGO receives committed-load weight and the protected stock remains removed from free inventory.
   - Planner must re-evaluate from the authoritative queue instead of caching an obsolete recommendation.

9. **Expired hold changes future planning**
   - Allow a protected CARGO reservation to expire.
   - Stock must return exactly once and backlog must increase exactly once.
   - The next planner summary must reflect the new backlog/queue state; repeated queries must not duplicate the consequence.

10. **Consequence ledger carries failed ROAD history**
    - Fail enough ROAD jobs that failed runs exceed completed-runs relief.
    - ROAD debt must rise but remain capped at four.
    - Complete clean ROAD work; debt should eventually earn down as completion history catches up.

11. **Consequence ledger carries failed CARGO history**
    - Repeat for CARGO failures and verify CARGO debt separately.
    - Existing CargoBacklogPressure remains an independent contribution and must not be overwritten by the derived debt.

12. **Police and game-warden gates remain authoritative**
    - Follow either planner recommendation while wanted or under wildlife alert.
    - Existing legal-work acceptance/handoff blocks must still win; planner advice is not permission to bypass law systems.

13. **No-double-debit CARGO regression**
    - Reserve an urgent CARGO load under a route conflict, then collect it before expiry.
    - Starting the oldest queued job must consume protected units without debiting Feed Depot stock a second time.

14. **Relationship favors remain intact**
    - Test NEW DRIVER, KNOWN, TRUSTED and PREFERRED.
    - Ordinary relationship allowances remain 35/50/80/110 minutes and TRUSTED/PREFERRED retain two queue slots.
    - The planner only narrows an actually urgent conflicting hold; it never expands one.

15. **Schema-v8 compatibility**
    - Load an untouched 0.1.11 v8 save containing nonzero history, backlog and a valid same-day reservation.
    - No migration/reset should occur solely for 0.1.12.
    - Recommendation and debt must reconstruct from saved authoritative inputs after load.

16. **Win64/demo acceptance boundary**
    - Do not infer runtime correctness from source sanity.
    - A demo remains blocked until UE 5.8 Win64 compile/package succeeds, packaged EXE runtime smoke passes, visual presentation is accepted, relevant Actions are green and no demo-critical blocker remains.

## Evidence to capture on a real runtime pass
- Exact commit SHA and packaged artifact hash.
- Contract Desk screenshots for ROAD win, CARGO win and SPLIT conflict.
- Feed Dispatcher before/after screenshots showing relationship allowance, priority SLA and route-conflict cap.
- Save/reload proof that recommendation reconstructs rather than being persisted independently.
- One expiry trace showing stock return once, backlog +1 once and changed planner score.
- Packaged-EXE launch/runtime smoke result and rendered visual-acceptance notes.
