# GTT 0.1.1 Playtest — Contract Board & Fleet Preparation Economy

## Goal

Verify the full player-facing loop `inspect contract -> see exact fleet risk/cost -> prepare service/refuel/dispatch -> accept the real contract -> get paid`, including the deliberate penalty path when an advisory cargo job is started from its older direct board without preparation.

## Setup

1. Use an Unreal Engine 5.8 development build or packaged Win64 candidate containing 0.1.1.
2. Start from a save that owns Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200.
3. Keep police wanted and game-warden alert at zero for the positive route.
4. Note starting cash before each preparation transaction.

## A — Unified board presentation

1. Walk to the garage-side contract boards.
2. Inspect Feed Cargo Run, Heavy Timber Haul, Timber Delivery and Field Mowing.
3. Confirm every card shows role, pay range, assigned saved loadout, readiness, C/F/T/B, PREP cost and NET MAX.
4. Confirm a READY + ACTIVE loadout displays `E - ACCEPT`; a non-active or worn loadout displays `E - PREP`.

Expected: the board data tracks the same garage loadout and condition values shown by garage/service UX; there is no independent fake mission vehicle state.

## B — Paid preparation and staging

1. Damage/de-fuel the assigned Mulebox or Fieldmaster, then return to the unified board.
2. Record the displayed PREP quote and cash balance.
3. Use PREP once.
4. Confirm cash decreases by the quoted amount.
5. Confirm the assigned vehicle is staged beside the selected contract card when dispatch was required.
6. Confirm road-vehicle damage/tires/fuel are serviced through the Native Chaos state; for Fieldmaster confirm the native pawn reflects the prepared mirror state.
7. Re-open the garage view and confirm the vehicle is ACTIVE and remains the correct CARGO/TRACTOR role loadout.
8. Save/load and confirm prepared service + loadout selection persist.

Expected: preparation is a real economy transaction and changes the same authoritative vehicle state used by driving and save/load.

## C — Accept routes use existing directors

1. Prepare Feed Cargo Run, then interact again with its board.
2. Confirm the normal farm objective becomes active and proceed to feed pickup/delivery.
3. Repeat with Heavy Timber Haul and verify the existing hitch/load/deliver stages are used.
4. Repeat Timber Delivery and Field Mowing and verify their existing rural objectives are used.

Expected: the unified board starts existing mission directors; it does not create duplicate objectives or duplicate payouts.

## D — No overlapping legal contracts

1. Start any contract from the unified board.
2. Interact with a second contract card before finishing/failing the first.

Expected: the second board refuses PREP/ACCEPT and tells the player to finish the active legal contract.

## E — Police and ranger locks

1. Gain wanted level 1+ and attempt PREP/ACCEPT.
2. Clear wanted, trigger a game-warden alert and attempt again.

Expected: both states block legal contract-board actions without spending cash or moving/serviceing a vehicle.

## F — Sandbox bypass has an economic consequence

1. Damage or de-fuel the CARGO loadout until Farm Cargo shows CAUTION.
2. Do **not** use the unified PREP action; start Farm Cargo from the old direct job terminal.
3. Complete the run and confirm the result explicitly reports `UNPREPARED FLEET PENALTY` and the total payout is reduced by 10% before normal minimum payout clamping.
4. Repeat with SERVICE REQUIRED and confirm the multiplier is 75%.
5. Repeat the same advisory/service-required bypass with Timber Delivery.
6. Confirm cargo damage/integrity loss can still reduce payout independently of the readiness multiplier.

Expected: the player may intentionally gamble with a worn cargo loadout, but the choice has visible economic consequences.

## G — Hard tractor safety gates remain

1. Put the TRACTOR loadout into SERVICE REQUIRED.
2. Attempt Heavy Haul and Field Mowing through their existing/direct entry routes and the unified contract route.

Expected: critically unsafe tractor work remains blocked until prep/service makes the tractor viable.

## H — Win64/demo evidence

For demo readiness, repeat A–G on the packaged Win64 EXE and capture logs/screenshots that show the contract cards, payment delta, staged vehicle, objective handoff and final payout. Source-level Project sanity alone is not a packaged-runtime or visual acceptance pass.
