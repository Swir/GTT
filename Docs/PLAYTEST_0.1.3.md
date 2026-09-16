# GTT 0.1.3 Playtest — Living Rural Logistics & Traffic Reputation

## Goal

Verify the complete ROAD loop `inspect board -> prepare Rattleback -> collect parts -> Hill Farm relay -> North Wood Yard handoff -> reputation/payout -> save/load`, including schedule boundaries, traffic risk, police consequences and persistent progression.

## Setup

1. Use an Unreal Engine 5.8 development build or packaged Win64 candidate containing 0.1.3.
2. Load a profile that owns Rattleback 82 and has enough cash for ROAD preparation.
3. Keep wanted and game-warden alert at zero for the positive route.
4. Record cash, logistics reputation, clean streak, completed/failed runs and current village time.

## A — Day/night contract window

1. Set/advance world time to 05:50 and inspect **Village Parts Courier**.
2. Confirm a mission-ready Rattleback shows the board as **CLOSED 21:30-06:00** and interaction cannot accept the contract.
3. If the Rattleback needs service/dispatch, confirm PREP remains available while closed and charges the exact displayed amount.
4. Advance to 06:00 and confirm the same staged vehicle can now accept without another dispatch charge.
5. Inspect the board at 18:29, then after 18:30. Confirm the schedule changes to **LATE SHIFT +10%** and the displayed ROAD pay range/net maximum increases.
6. At 21:30 confirm new ROAD acceptance closes again.

Expected: schedule uses the real `AGTTDayNightCycle`; prep and acceptance are separate, and closing time never creates a fake free service action.

## B — Multi-stop route

1. Accept the contract with a READY + ACTIVE Rattleback during the open window.
2. Confirm **PARTS DEPOT / COURIER PICKUP** is visible while Hill Farm and North Wood Yard markers are hidden.
3. Collect the shipment in the real Rattleback.
4. Confirm the loaded timer starts and **HILL FARM / COURIER RELAY** becomes the current world/HUD objective.
5. Reach Hill Farm and confirm the relay signs without resetting timer or shipment integrity.
6. Confirm **NORTH WOOD YARD / COURIER DROP** becomes the final objective.
7. Complete the handoff.

Expected: one continuous loaded job crosses two rural destinations and keeps the same integrity/timer state through the relay.

## C — Traffic, condition and crash risk across both legs

1. Run a healthy Rattleback below the safe cruise threshold as a baseline.
2. Repeat with sustained overspeed before Hill Farm and confirm shipment integrity falls.
3. Continue overspeed after Hill Farm and confirm the same integrity value continues falling; the relay must not reset it.
4. Repeat with worn condition/tires/body and confirm continuous mechanical-risk decay.
5. In Native Chaos takeover, cause a real impact before and after the relay. Confirm each new Native impact immediately reduces integrity and the impact count carries through to final scoring.
6. Destroy the shipment and confirm the run fails, resets the clean streak and persists the failure.

Expected: both loaded legs consume the same existing vehicle-risk signals; the new route is not two disconnected mini-games.

## D — Police incident memory

1. Start a loaded run at wanted 0.
2. Gain wanted before Hill Farm and enter the relay zone.
3. Confirm Hill Farm refuses the signature while the delivery clock continues.
4. Clear wanted and complete the relay.
5. Finish the final delivery cleanly without another collision.

Expected: completion remains possible after escaping, but the run remembers the earlier police incident, does not grant the clean-run condition and does not build a clean streak as if nothing happened.

## E — Reputation and payout progression

1. On a fresh/legacy v7 profile confirm reputation starts at 0 / NEWCOMER rather than reading garbage from old save data.
2. Complete a high-integrity, no-impact, no-police daytime run and note reputation/streak increase.
3. Return to the board and confirm the advertised ROAD pay range increases from the same reputation multiplier.
4. Complete multiple clean runs and confirm the clean-streak bonus grows but remains capped.
5. Complete a damaged run and confirm reputation grows more slowly and the clean streak resets.
6. Trigger a police incident and confirm the reputation gain is reduced.
7. Fail a run and confirm reputation drops and failed-run count increments.
8. Compare daytime with an 18:30–21:30 start; confirm late shift locks +10% at contract start even if the run completes after 21:30.

Expected: payout progression is bounded, visible before acceptance and based on actual delivery quality rather than a disconnected XP counter.

## F — Save/load schema v8

1. Record reputation, clean streak, completed runs, failed runs and lifetime logistics revenue.
2. Trigger normal SaveProgress, quit/relaunch or load the primary slot, and verify all five values restore exactly.
3. Confirm ROAD/TRACTOR/CARGO mission loadouts still restore.
4. Confirm active garage dispatch still restores.
5. Confirm Native road structural damage/panels still restore.
6. Load a pre-v8 save and verify logistics values start safely at zero while the older v5/v6/v7 data remains intact.

Expected: schema v8 adds logistics persistence without regressing structural damage, fleet dispatch or mission loadouts.

## G — Board UX and economy consistency

1. With nonzero reputation and a late shift, compare the board's displayed minimum/maximum/net values with the completed run.
2. Confirm PREP cost is subtracted only from the board's net-best-case display, not silently from the actual delivery payout twice.
3. Confirm the board shows a compact single reputation/schedule row rather than additional debug telemetry.
4. Confirm non-ROAD contract boards retain their existing layout and values.

Expected: what the player sees before accepting matches the multiplier locked into that run.

## H — Win64/demo evidence

For demo readiness, repeat A–G on the packaged Win64 EXE and capture logs/screenshots showing the closed/open/late schedule states, paid preparation, all three route markers, live shipment integrity, a wanted-blocked relay/handoff, reputation change, and save/load persistence. Source-level Project sanity is regression evidence only; it is **not** a UE 5.8 compile/package pass, packaged-runtime smoke pass or rendered visual acceptance.
