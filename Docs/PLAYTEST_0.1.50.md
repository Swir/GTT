# GTT 0.1.50 — Workshop Job Board & Safe Appointment Control Playtest

This milestone is a source-integrated gameplay/UX step. It does **not** claim Unreal Engine 5.8 Win64 packaging, packaged-EXE smoke success or rendered demo acceptance. No packaged Win64 proof is claimed by this document.

## Goal

Connect the existing 0.1.47–0.1.49 multi-vehicle workshop lifecycle to a physical, player-facing operations board beside the garage. The board must expose all exact-ID appointments without becoming a second repair/economy authority, and cancellation must be deliberate rather than a destructive one-tap action.

## Core acceptance

- The garage spawns a source-built workshop job board only when no authored board is already nearby.
- The board lists up to four queue snapshots with lifecycle state, exact `PersistentVehicleId`, locked quote and relevant ETA.
- Lifecycle triage is readable as SCHEDULED / CHECK-IN / WORKING / PAYMENT while the authoritative queue keeps the real state machine.
- A nearby waiting/ready appointment can be cancelled only after two interactions inside a six-second confirmation window.
- Cancellation calls the existing exact-ID `CancelQueuedRepair()` path. The board never calls `SpendCash`, `AddCash` or `ApplyNativeWorkshopService`.
- Checked-in IN_SERVICE/AWAITING_PAYMENT work cannot be cancelled through the board.
- Hard TOW/IMMOBILE WORKSHOP HOLD remains on its separate emergency service lane.
- Farm Cargo, locked quotes, no-precharge behavior, service timers and persistence remain owned by existing systems.

## 48-case matrix

The manual/runtime matrix is the Cartesian product of **4 lifecycle states × 3 proximity situations × 2 confirmation-window states × 2 authority contexts = 48 cases**.

Lifecycle state:
1. QUEUED
2. READY
3. IN_SERVICE
4. AWAITING_PAYMENT

Proximity situation:
1. exact queued vehicle within 450 cm of the board
2. different/non-queued vehicle within 450 cm
3. no road vehicle within 450 cm

Confirmation state:
1. first interaction / no armed cancellation
2. second interaction within the six-second exact-ID window

Authority context:
1. ordinary vehicle appointment
2. active Farm Cargo exact vehicle (queue authority must remain compatible with the bound cargo ID)

### Expected matrix behavior

| State | Exact vehicle nearby | First interaction | Confirmed second interaction | Board-only view |
|---|---|---|---|---|
| QUEUED | yes | arm exact-ID cancellation + show board | cancel via existing queue API, zero charge | no mutation |
| READY | yes | arm exact-ID cancellation + show board | cancel via existing queue API, zero charge | no mutation |
| IN_SERVICE | yes | show board only | show board only | cancellation denied by lifecycle |
| AWAITING_PAYMENT | yes | show board only | show board only | payment remains queue-owned |
| any | different vehicle | show board only | show board only | no target substitution |
| any | no nearby vehicle | show board only | show board only | no target invented |

## Safety / regression cases

- Let the confirmation timer expire, then interact again: cancellation must re-arm instead of firing.
- Arm cancellation for vehicle A, move A away and bring vehicle B nearby: B must require its own first interaction; A must not be cancelled.
- Fill all four appointment slots and verify board output preserves queue positions and exact IDs.
- Verify the garage still books only while the regular workshop is closed.
- Verify a hard WORKSHOP HOLD vehicle never becomes cancellable through the deferred appointment board.
- Verify queue cancellation preserves cash exactly and does not repair/refuel the vehicle.
- Verify direct workshop terminal guard still blocks walk-up repair/refuel for an active queue appointment.
- Verify leaving IN_SERVICE still uses the existing queue pause/reset behavior.
- Verify insufficient checkout funds still produce AWAITING_PAYMENT and do not block later appointments.
- Verify source-built board spawning does not duplicate an authored/nearby board.

## Source verification

Run:

```bash
python Scripts/verify_workshop_job_board.py
python Scripts/verify_progress_presentation.py
```

The verifier checks integration tokens, exact-ID two-step cancellation, absence of economy/repair mutation authority inside the board, garage spawn wiring and unchanged roadmap mathematics.

## Demo-release consequence

This milestone improves player-facing workshop usability, but it does not close any of the five remaining roadmap blockers. Demo remains NOT READY until the exact candidate has a verified UE 5.8 Win64 compile/package, packaged EXE runtime smoke evidence, green required Actions and rendered visual acceptance.
