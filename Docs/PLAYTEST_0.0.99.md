# GTT 0.0.99 Playtest — Persistent Fleet Dispatch & Job Fit

This milestone turns the garage from a passive fleet list into a persistent dispatch choice that legal work can understand. Source/contract checks do not replace a real UE 5.8 Win64 playtest; complete the runtime checks below when an Unreal-capable machine is available.

## 1. Garage roles and active dispatch
1. Load a save with Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200 owned.
2. Open the garage office. Confirm the fleet summary labels Fieldmaster as `TRACTOR`, Rattleback as `ROAD`, and Mulebox as `CARGO`.
3. Use the Mulebox numbered bay terminal and pay the $15 dispatch charge.
4. Confirm the Mulebox is moved to the bay without repairing condition/body/tires, refilling fuel, or changing tuning.
5. Confirm the bay/office now marks Mulebox as `ACTIVE`.

**Pass:** the paid dispatch changes location + preferred fleet choice only; vehicle wear and upgrades are preserved.

## 2. Crime/warden dispatch locks
1. Raise wanted to at least 1 and try a garage dispatch.
2. Clear wanted, raise a game-warden alert, and try again.
3. Clear both alerts and repeat the dispatch.

**Pass:** wanted and warden states block paid dispatch; a clean player can dispatch normally.

## 3. Save/load persistence and migration
1. Dispatch Rattleback 82 and save/quit.
2. Reload the same slot.
3. Confirm the garage still marks Rattleback as the preferred active dispatch vehicle.
4. Repeat with Mulebox 1200.
5. Load a pre-v6 save if available.

**Pass:** v6 restores the exact preferred ID; older saves choose a valid owned fallback and still restore v5 structural damage records.

## 4. Farm cargo job fit
1. Make Mulebox active, then start the legal farm cargo contract.
2. Confirm the second player-facing message reports the Mulebox fleet choice as ready.
3. Complete the delivery with Mulebox and confirm the existing Mulebox role bonus still pays.
4. Dispatch Rattleback, start another farm cargo contract, and confirm the game recommends the Mulebox bay instead of pretending the road car is optimal.

**Pass:** legal work consumes the same live fleet-dispatch state the garage owns; the recommendation is advisory and the existing role reward remains tied to actually using Mulebox.

## 5. Native Chaos authority
1. With Native Rattleback/Mulebox takeover active, damage and partially refuel one vehicle.
2. Dispatch it from its garage bay.
3. Confirm Native motion is stopped, the pawn is moved to the bay, its mirror is synchronized, and damage/fuel/tuning values survive.
4. Save/load and confirm both the vehicle state and preferred dispatch selection survive.

**Pass:** dispatch never edits a stale hidden mirror in place of an active Native vehicle.

## 6. Demo-release gate reminder
Do **not** publish a demo from this milestone unless a real UE 5.8 Win64 package is produced, the packaged EXE completes the required runtime smoke/evidence route, relevant GitHub Actions are green, and rendered visual acceptance passes. Source-level Project sanity alone is not sufficient.
