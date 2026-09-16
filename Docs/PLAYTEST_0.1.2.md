# GTT 0.1.2 Playtest — Rattleback Road Courier & Traffic-Risk Economy

## Goal

Verify the full ROAD-role loop `inspect courier -> prepare Rattleback -> collect parts -> drive through the live road sandbox -> preserve shipment -> clear police if needed -> deliver -> get paid`, using the same garage/service/wanted/save systems already used elsewhere.

## Setup

1. Use an Unreal Engine 5.8 development build or packaged Win64 candidate containing 0.1.2.
2. Load a save that owns the Rattleback 82 and has enough cash to service/dispatch it.
3. Keep wanted and game-warden alert at zero for the positive route.
4. Note the starting cash and Rattleback condition/fuel/tires/body state.

## A — Fifth unified contract card

1. Visit the garage-side unified contract boards.
2. Confirm a fifth card reads **Village Parts Courier | ROAD**.
3. Confirm it assigns the Rattleback 82 and displays C/F/T/B readiness, PREP cost and NET MAX.
4. Damage, de-fuel or de-tire the Rattleback and confirm the card quote changes from the same authoritative garage snapshot.

Expected: the ROAD card is not a duplicate state store; it reflects the same Rattleback used by garage/service UX.

## B — Paid ROAD preparation

1. With the Rattleback not active or below readiness, record the PREP quote and current cash.
2. Use PREP.
3. Confirm the exact displayed preparation cost is charged.
4. Confirm service/refuel/tire work is applied through Native road authority when takeover is active.
5. Confirm the Rattleback is staged by the board, becomes ACTIVE + ROAD loadout and the selection/service state survives save/load.
6. Confirm the preparation success message reports net best-case value after preparation rather than repeating gross maximum reward.

Expected: the contract board performs a real economy transaction and stages the same persistent road vehicle that will be driven.

## C — Pickup route and readable markers

1. Accept Village Parts Courier with a READY + ACTIVE Rattleback.
2. Confirm the compact **PARTS DEPOT / COURIER PICKUP** marker becomes visible and the drop marker remains hidden.
3. Try approaching the pickup in a tractor or Mulebox; confirm the contract does not advance.
4. Enter the Rattleback and reach the pickup zone.
5. Confirm the pickup marker hides, **NORTH WOOD YARD / COURIER DROP** becomes visible and the timed delivery stage begins.

Expected: ROAD role means actual Rattleback use, not merely selecting a garage entry.

## D — Speed, condition and crash risk

1. Start a clean run in a healthy Rattleback and drive below the safe cruise threshold for a baseline.
2. Repeat and sustain high speed above the configured safe cruise threshold; confirm shipment integrity decays.
3. Repeat with worn condition/tires/body and confirm the shipment loses integrity faster even without a collision.
4. In Native Chaos Rattleback, strike roadside geometry or traffic hard enough to increment the native impact counter.
5. Confirm the impact immediately removes shipment integrity and produces a compact courier-impact message.
6. Reduce shipment integrity close to zero and confirm the contract fails when the shipment is effectively destroyed.

Expected: preparation and careful driving affect money. Traffic/crash risk is not detached from the job economy.

## E — Police consequence during a legal run

1. Start the delivery leg with wanted zero.
2. Trigger police attention before arriving at North Wood Yard.
3. Enter the delivery zone while still wanted.

Expected: the handoff is refused while wanted, a short message tells the player to lose police, and the delivery timer keeps counting down.

4. Escape police before the timer expires and return to the drop zone.

Expected: the handoff becomes available again without resetting shipment integrity or the timer.

## F — Payout and persistence

1. Complete one slow/damaged run and record payout.
2. Complete one fast, clean run with shipment integrity at or above 97% and no Native impact during the loaded leg.
3. Confirm the clean fast run receives the advertised fast and clean bonuses and never exceeds the board maximum of $390.
4. Confirm damaged shipment reduces payout.
5. Confirm completion adds cash through the player economy and calls normal SaveProgress.
6. Save/load and verify the new cash total and prepared ROAD loadout remain persistent.

## G — Contract overlap and legal locks

1. While RoadRun is active, try to prepare/accept another legal contract from the unified board.
2. Confirm the global legal-work overlap lock refuses it.
3. Finish/fail RoadRun, gain wanted, then try to prepare or accept it again.
4. Clear wanted, trigger a game-warden alert and retry.

Expected: existing police/game-warden locks and single-active-contract rules also govern the new ROAD job.

## H — Win64/demo evidence

For demo readiness, repeat A–G on the packaged Win64 EXE and capture logs/screenshots showing the fifth card, paid PREP, actual Rattleback pickup, marker transition, impact/integrity consequence, wanted-blocked handoff and final payout. Source-level Project sanity is useful regression evidence but is **not** a packaged-runtime or rendered visual acceptance pass.
