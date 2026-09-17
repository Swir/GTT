# GTT 0.1.10 Playtest — Shared ROAD/CARGO Priority Dispatch & Rush Contracts

This checklist validates the 0.1.10 source integration once a real Unreal Engine 5.8 runtime is available. It does **not** convert source sanity into packaged-Windows evidence.

## Preconditions
- Use a profile with the Rattleback 82 and Mulebox 1200 owned and mission-ready.
- Keep at least one save from 0.1.9 to verify additive schema-v8 compatibility.
- Exercise both legacy/native road takeover paths where the current runtime build permits it.
- Record current world day, commodity, depot stock, Hill demand, Wood demand and backlog before each market manipulation.

## Scenarios

1. **Standard dispatch baseline**
   - Reach a market state below both urgency thresholds.
   - Feed/Hill/Wood dispatcher detail should report `STANDARD DISPATCH` and a neutral ready-logistics recommendation.
   - Starting a Rattleback courier must keep the established road reward multiplier and the full 185-second pickup-to-delivery window.

2. **ROAD priority only on FARM PARTS**
   - Build Wood Yard/backlog pressure above the ROAD threshold while commodity rotation is not `FARM PARTS`.
   - ROAD urgency must remain STANDARD.
   - Advance to a FARM PARTS rotation with the same pressure; ROAD urgency should now activate.

3. **ROAD urgency thresholds**
   - Validate `WoodYardDemand + 2 * CargoBacklogPressure` around 4/5, 8/9 and 13/14.
   - Expected transitions: STANDARD -> PRIORITY -> RUSH -> CRITICAL.
   - Verify all three dispatcher briefings agree with the same urgency.

4. **ROAD risk/reward lock**
   - Accept a PRIORITY courier, then change market pressure before pickup.
   - Reward multiplier should remain the value locked at acceptance.
   - At pickup, the delivery timer should use the current contract's priority time scale: 94% for PRIORITY, 88% for RUSH, 82% for CRITICAL.

5. **Rush payout**
   - Complete clean comparable STANDARD and urgency runs.
   - Confirm urgency raises the final road payout by the configured bounded multiplier while existing fast/clean bonuses still depend on actual performance.
   - Confirm no extra CARGO payout multiplier is silently added by 0.1.10.

6. **Rush collision consequence**
   - On a RUSH/CRITICAL run, trigger one or more Native Rattleback impacts.
   - Parcel integrity must still fall and can erase the benefit of the higher reward.
   - Destroyed cargo must still fail the job and record the reputation consequence.

7. **Rush police consequence**
   - Gain wanted during either leg.
   - Hill Farm/North Wood Yard must continue refusing the legal handoff while wanted.
   - Losing police may allow completion if the tighter urgency timer has not expired; the police incident must still disqualify a clean run.

8. **CARGO priority thresholds**
   - Manipulate the stronger buyer need, backlog and active order tier around the 6/11/16 pressure thresholds.
   - Expected transitions: STANDARD -> PRIORITY -> RUSH -> CRITICAL.
   - Verify T2/T3 contributes more pressure than T1 without bypassing reputation, stock or demand availability.

9. **Stock-backed zero-free-stock case**
   - Reserve a real CARGO order so protected units leave free depot stock, then reduce free stock to zero.
   - CARGO urgency must remain available from the protected reservation instead of falsely reporting no deliverable load.
   - Starting the reserved job must consume the queue entry without a second stock debit, preserving 0.1.9 behavior.

10. **Priority vehicle recommendation**
    - Produce ROAD urgency greater than CARGO urgency; recommendation should be `RATTLEBACK 82 / ROAD`.
    - Produce CARGO urgency greater than ROAD urgency; recommendation should be `MULEBOX 1200 / CARGO`.
    - On equal FARM PARTS urgency, verify Wood-vs-Hill buyer pressure deterministically breaks the tie.

11. **Dispatcher presentation regression**
    - Feed world text must retain `E NEGOTIATE | DESK ...` and queue capacity while adding dispatch state.
    - Hill and Wood must retain their compact `E STATUS` hooks.
    - Detailed interactions may show the full priority briefing, but world-space labels must remain compact enough not to become a wall of text.

12. **Day rollover / backlog persistence**
    - Save with non-zero backlog, rotate to the next day, then reload.
    - Priority should recompute from the persisted/evolved market rather than saving a stale standalone urgency enum.
    - Confirm missed reservations still return stock once and add their existing backlog consequence.

13. **Schema-v8 compatibility**
    - Load a clean 0.1.9 schema-v8 save.
    - No migration/reset should occur solely for priority dispatch; urgency should be reconstructed from restored market/reservation fields.
    - Save again and verify 0.1.9 reservation arrays/history remain intact.

14. **Fleet-readiness gates**
    - Make Rattleback service-required and attempt a rush ROAD job: start must remain blocked.
    - Restore ROAD readiness and confirm dispatch works.
    - CARGO recommendation must not bypass Mulebox preparation, wanted/game-warden locks, dispatcher access, stock, buyer demand or shift hours.

15. **Demo-gate honesty**
    - Source sanity alone is insufficient.
    - Do not publish a demo unless UE 5.8 Win64 compile/package succeeds, the packaged EXE passes runtime smoke, rendered presentation is visually accepted, relevant Actions are green and no demo-critical blocker remains.

## Evidence to capture on a real runtime pass
- Exact commit SHA and packaged artifact hash.
- Screenshots of STANDARD and urgency dispatcher briefings without HUD clutter.
- Video or structured runtime log of one CRITICAL Rattleback run showing locked payout multiplier, shortened timer, one traffic-impact integrity loss and successful/failed handoff behavior.
- Save/reload evidence covering a protected CARGO reservation and reconstructed priority.
- Packaged-EXE launch/runtime smoke result and rendered visual-acceptance notes.
