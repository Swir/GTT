# GTT 0.1.51 — Workshop Priority, Pickup & Fleet Return Playtest

This milestone is a source-integrated gameplay/economy/save step. It does **not** claim Unreal Engine 5.8 Win64 packaging, packaged-EXE smoke success or rendered demo acceptance. No packaged Win64 proof is claimed by this document.

## Goal

Turn the 0.1.50 workshop board into a complete service handoff: waiting STANDARD jobs can be deliberately promoted to URGENT without pre-charge, successful paid service remains physically checked in as `READY_FOR_PICKUP`, and the exact repaired vehicle returns to garage dispatch only after explicit pickup at the workshop board.

## Core acceptance

- New appointments remain STANDARD and retain the request-time locked quote.
- A separate priority desk beside the job board requires two exact-ID interactions inside six seconds before promotion.
- URGENT promotion is allowed only before workshop check-in and while the regular workshop is closed.
- Promotion changes the locked checkout quote exactly once by +20%, advances ahead of earlier waiting STANDARD work where applicable, and shortens the production service timer to x0.80.
- Priority promotion does not spend cash or mutate repair state; the updated quote and priority are persisted in the existing additive `GTT_WorkshopQueue_01` sidecar.
- Successful timed checkout still spends the exact locked quote once and applies the existing native workshop service once.
- Instead of immediately deleting the completed appointment, checkout persists `READY_FOR_PICKUP`, paid amount/date/time and exact vehicle identity.
- The workshop job board releases only a nearby exact paid/repaired `READY_FOR_PICKUP` vehicle; pickup itself never charges or repairs.
- Garage slot recall displays `PICKUP HOLD` and refuses teleport/payment until pickup release, after which ordinary dispatch is available again.
- Hard `TOW/IMMOBILE WORKSHOP HOLD` remains a separate higher-priority emergency lane.
- Farm Cargo authority remains exact-ID and no source milestone closes a runtime/art roadmap gate.

## 80-case matrix

The manual/runtime matrix is the Cartesian product of **5 lifecycle states × 2 priority modes × 2 proximity states × 2 confirmation states × 2 authority contexts = 80 cases**.

Lifecycle state:
1. QUEUED
2. READY
3. IN_SERVICE
4. AWAITING_PAYMENT
5. READY_FOR_PICKUP

Priority mode:
1. STANDARD
2. URGENT

Proximity state:
1. exact appointment vehicle beside board/priority desk
2. exact vehicle absent or a different vehicle nearby

Confirmation state:
1. first interaction / no armed action
2. second interaction inside the six-second exact-ID window

Authority context:
1. ordinary owned native road vehicle
2. active Farm Cargo exact bound vehicle

### Expected matrix behavior

| Lifecycle | STANDARD | URGENT | Job board | Priority desk | Garage slot |
|---|---|---|---|---|---|
| QUEUED | cancellable with two-step guard | cancellable with two-step guard | read/cancel exact ID | STANDARD can arm/confirm +20%; URGENT view-only | normal dispatch rules |
| READY | cancellable before check-in | cancellable before check-in | read/cancel exact ID | promotion rejected once workshop is open | drive exact vehicle to workshop |
| IN_SERVICE | working | working | view only | no promotion | workshop lifecycle owns vehicle |
| AWAITING_PAYMENT | payment due | payment due | view only | no promotion | later appointments remain non-blocking |
| READY_FOR_PICKUP | paid/repaired | paid/repaired | exact pickup releases queue/fleet hold | no promotion | `PICKUP HOLD` until board release |

## Priority safety cases

- Book a STANDARD job after hours and record locked quote/ready slot. No cash changes.
- Arm URGENT promotion, let six seconds expire, interact again: it must re-arm rather than change the quote.
- Arm vehicle A, move A away and bring vehicle B to the desk: B needs its own first interaction and A remains STANDARD.
- Confirm A: locked quote becomes exactly `standard + ceil(standard * 0.20)` and no cash changes at promotion time.
- Confirmed URGENT cannot be promoted again; its locked quote must not compound.
- If a waiting STANDARD slot exists earlier than the promoted job, the URGENT job receives that earlier service slot and the displaced STANDARD job receives the old slot.
- Already checked-in, payment-pending and pickup-ready jobs reject promotion.
- Promotion while the regular workshop is open rejects cleanly with unchanged quote/state.
- Save/load preserves `bUrgent`, locked quote and exact ID.
- URGENT service duration equals the same workload-derived STANDARD duration multiplied by x0.80, clamped to the priority duration envelope.

## Pickup / fleet-return safety cases

- Complete timed service with sufficient cash: one locked-quote debit and one repair/refuel mutation occur before `READY_FOR_PICKUP` is persisted.
- Save/load a paid pickup entry: exact ID, paid amount and pickup state survive.
- A paid repaired vehicle remains visible on the job board as `PICKUP`, not as a new repair request.
- Garage slot label shows `PICKUP HOLD - JOB BOARD` for the exact pickup vehicle.
- Attempt garage dispatch before pickup: no teleport and no recall-service charge.
- Bring the wrong owned vehicle to the board: it cannot consume another exact-ID pickup.
- Move the exact paid vehicle away from the workshop: pickup release rejects and preserves the checkpoint.
- Return the exact paid vehicle to the workshop and interact with the board: the pickup checkpoint is removed, primary progress is saved and garage dispatch becomes available.
- Pickup release itself must contain no `SpendCash`, `AddCash` or `ApplyNativeWorkshopService` call.
- If the pickup checkpoint cannot be persisted immediately after a successful paid repair, the fallback auto-releases rather than permanently stranding a paid vehicle.
- Hard WORKSHOP HOLD must not be converted into a deferred priority/pickup appointment.
- Active Farm Cargo keeps its bound exact vehicle authority through priority, service, payment and pickup.

## Regression / compatibility cases

- Existing 0.1.45/0.1.46 schema-v1 sidecars load as STANDARD, not pickup-ready.
- Existing 0.1.47 four-slot/45-minute capacity remains intact for STANDARD bookings.
- Existing 0.1.49 check-in pause-on-leave and AWAITING_PAYMENT logic remains intact.
- Existing 0.1.50 exact-ID cancellation guard remains intact and never cancels checked-in/pickup work.
- Historical packaged queue/capacity evidence must use the production service path and may auto-release pickup only under its explicit evidence command-line flags so old same-SHA evidence expectations are not silently invalidated.
- Roadmap stays exactly 125/130 = 96.2%; no Native Chaos, trailer, Win64 package/runtime or visual gate is closed from this source work.

## Source verification

Run:

```bash
python Scripts/verify_workshop_priority_pickup.py
python Scripts/verify_workshop_job_board.py
python Scripts/verify_workshop_service_lifecycle.py
python Scripts/verify_workshop_multi_vehicle_capacity.py
python Scripts/verify_progress_presentation.py
```

## Demo-release consequence

This milestone closes a player-facing workshop lifecycle gap, but Demo remains **NOT READY** until the exact candidate has a verified Unreal Engine 5.8 Win64 compile/package, packaged EXE runtime smoke evidence, green required Actions and rendered visual acceptance.
