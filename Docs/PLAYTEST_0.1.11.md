# GTT 0.1.11 Playtest — Emergency Dispatch Windows & Priority Job Chains

This checklist validates 0.1.11 in a real Unreal Engine 5.8 runtime. Source sanity is not packaged-Windows evidence.

## Preconditions
- Use a profile with Rattleback 82 and Mulebox 1200 owned and mission-ready.
- Keep a schema-v8 save from 0.1.10 for compatibility testing.
- Record world day/time, commodity rotation, Feed Depot stock, Hill/Wood demand, backlog, clean streak, ROAD urgency and CARGO urgency before each market manipulation.
- Exercise Native and legacy takeover paths where the current runtime permits it.

## Scenarios

1. **STANDARD ROAD has no emergency pickup timer**
   - Reach a market state where ROAD urgency is 0.
   - Accept the parts courier contract.
   - Objective should show the normal Village Parts Depot pickup without an SLA countdown.
   - Waiting before pickup must not fail the contract and the new chain multiplier must stay neutral.

2. **ROAD PRIORITY pickup SLA**
   - Enter FARM PARTS rotation with ROAD urgency 1 and clean chain 0.
   - Accept the Rattleback contract and verify a 90-second pickup SLA is shown immediately.
   - Reach Village Parts Depot before expiry; the existing two-leg delivery timer must begin only after pickup.

3. **ROAD RUSH and CRITICAL pickup SLAs**
   - Exercise urgency 2 and 3.
   - With clean chain 0, expected pickup SLAs are 70 and 50 gameplay seconds.
   - The existing delivery scales must remain 88% and 82% respectively after pickup.

4. **ROAD SLA failure is consequential**
   - Accept an urgent ROAD order and deliberately miss the depot pickup deadline.
   - Contract must fail, markers must clear and the existing courier-failure reputation/clean-streak consequence must be saved.
   - A reload must not resurrect the failed active contract.

5. **Pickup-radius expiry edge**
   - Approach the pickup radius as the countdown reaches zero.
   - If the Rattleback enters the valid radius on that frame, pickup should win before expiry and start delivery.
   - Confirm there is no double failure or stale pickup marker.

6. **Accepted ROAD terms are locked**
   - Accept a PRIORITY or RUSH job and note reward multiplier, label, pickup SLA and delivery scale.
   - Change market pressure before reaching Village Parts Depot.
   - Accepted terms must not silently change; pickup must start the delivery timer using the acceptance-time scale.

7. **Priority chain builds across lanes**
   - Complete clean legal CARGO and ROAD work until clean streak reaches at least 3.
   - Priority-chain display should cap at 3/3 even if the underlying clean streak is larger.
   - Urgent ROAD pickup gains +10 seconds per chain step; urgent CARGO pickup gains +5 world-clock minutes per step.

8. **Priority chain reward remains bounded**
   - On urgent ROAD work, compare chain 0 and chain 3 payouts with otherwise equivalent performance.
   - The chain layer is +3% per step and the combined urgency layer is hard capped at 1.28x before the existing courier market/reputation multiplier.
   - STANDARD ROAD must never gain this emergency-chain multiplier solely because CARGO is urgent.

9. **Failure breaks the shared chain**
   - Build a clean chain, then fail either ROAD or CARGO using an existing legitimate failure path.
   - Existing `CleanStreak` should reset, and the next emergency-dispatch briefing should immediately reconstruct chain 0 without a separate migration field.

10. **CARGO PRIORITY effective hold**
    - Create urgency 1 and reserve an available order at Feed Dispatcher.
    - Effective reservation window must be no longer than 40 world-clock minutes at chain 0, even if relationship favor permits longer.
    - Stock must be debited exactly once when the hold is written.

11. **CARGO RUSH / CRITICAL effective hold**
    - Exercise urgency 2/3 at the desk.
    - Chain-0 effective ceilings are 25/15 world-clock minutes; a relationship allowance shorter than the SLA still wins via the minimum.
    - TRUSTED/PREFERRED queue capacity remains intact but cannot bypass the emergency deadline.

12. **CARGO chain grace and expiry consequence**
    - Repeat CRITICAL CARGO with chain 3; SLA should receive +15 world-clock minutes before relationship min-cap is applied.
    - Let a protected load expire. Existing behavior must return stock once and add backlog pressure once.
    - Query the desk repeatedly after expiry to verify the consequence is idempotent.

13. **Queued CARGO no-double-debit regression**
    - Reserve a real urgent CARGO order, then start the oldest queued job before expiry.
    - The queue entry must be consumed without a second Feed Depot stock debit.
    - Buyer demand and settlement remain driven by the existing FarmJobDirector route.

14. **Police, cargo and collision consequences remain authoritative**
    - Trigger wanted during an urgent ROAD/CARGO delivery after pickup.
    - Hill Farm/North Wood Yard must still refuse legal handoff while wanted and the delivery clock must continue.
    - Native impacts/poor vehicle condition must continue damaging parcel/cargo quality; urgency must not bypass readiness or risk systems.

15. **Schema-v8 compatibility**
    - Load a clean 0.1.10 schema-v8 save with nonzero clean streak and at least one valid CARGO reservation.
    - No migration/reset should occur solely for 0.1.11.
    - Priority chain must reconstruct from clean streak and current market; reservation timestamps remain valid and bounded.

16. **Dispatcher/HUD presentation**
    - Feed world prompt must remain compact (`E NEGOTIATE | DESK ...`) while detailed interaction reports effective SLA/chain.
    - Hill and Wood compact `E STATUS` prompts must remain readable.
    - ROAD objective must show urgent pickup countdown only while waiting for parts, then revert to the existing delivery objective.

17. **Win64/demo acceptance boundary**
    - Source sanity is insufficient.
    - Do not publish a demo until UE 5.8 Win64 compile/package succeeds, the packaged EXE passes runtime smoke, rendered presentation is visually accepted, relevant Actions are green and no demo-critical blocker remains.

## Evidence to capture on a real runtime pass
- Exact commit SHA and packaged artifact hash.
- Video/log of STANDARD vs CRITICAL ROAD acceptance, pickup expiry and locked delivery timer.
- Screenshots of Feed Dispatcher CARGO SLA with NEW DRIVER and PREFERRED relationship states.
- Save/reload evidence showing chain reconstruction and protected reservation continuity.
- Police/collision regression evidence on one urgent delivery.
- Packaged-EXE launch/runtime smoke result and rendered visual-acceptance notes.
