# GTT 0.0.98 — Native Road Fleet Tuning & Persistent Garage Service Playtest

## Goal
Verify that Rattleback 82 and Mulebox 1200 use their active Native Chaos pawn as the tuning/service authority, that paid work survives normal save flow, and that the garage gives the player a useful next-service plan instead of showing only raw condition percentages.

## Setup
1. Own a Rattleback 82 and Mulebox 1200 and confirm their Native Chaos takeover is active.
2. Keep police wanted and game-warden alert at zero.
3. Leave one road vehicle below 80% tire integrity and partially empty the fuel tank of the other.
4. Record cash, fuel, condition, tire integrity, engine tune level and tire tune level in the garage fleet screen.

## Native road tire service and tuning
1. Park the damaged-tire Native road vehicle inside tuning-terminal range.
2. PASS: the interaction prompt names the real Native vehicle and displays the exact $65 tire-service price.
3. Buy tire service.
4. PASS: cash decreases exactly once, Native tire integrity becomes 100%, and the hidden compatibility mirror is not used as the tuning authority.
5. Interact again and buy engine tune L1.
6. PASS: Native engine level becomes 1 immediately and normal driving consumes that same level through throttle power and fuel-efficiency calculations.
7. Buy engine L2/L3 and then tire upgrades L1-L3 as funds permit.
8. PASS: tire tuning feeds the existing Native grip/wheel-risk assistance instead of being a display-only stat.
9. PASS: every successful paid operation calls normal SaveProgress.

## Persistence proof
1. Save after at least one engine or tire upgrade, exit to the normal reload path, then load progress.
2. PASS: the Native road vehicle resumes with the same engine/tire tune levels and tire integrity imported from its persistent compatibility record.
3. Drive for at least 30 seconds after reload.
4. PASS: engine/tire effects remain active and Native periodic mirror sync does not revert the upgrades.

## Fuel-only workshop service
1. Use an otherwise healthy Native Rattleback/Mulebox with a partially empty fuel tank.
2. Read the workshop prompt.
3. PASS: the prompt offers REFUEL with an exact per-litre quote rather than charging the full damage-repair service.
4. Buy fuel.
5. PASS: only missing litres are added, cash changes by the quoted amount, condition/body/tire state is unchanged, and SaveProgress runs.
6. Damage the same vehicle structurally and return to the workshop.
7. PASS: the terminal switches back to the existing damage-based repair+refuel quote and the full workshop recovery path.

## Garage service planner
1. Inspect the garage fleet office with Fieldmaster, Rattleback and Mulebox owned.
2. PASS: every slot still shows C/F/T/B state plus engine/tire tune levels.
3. PASS: the next action is useful and deterministic: WORKSHOP for mechanical/structural damage, TIRES for worn tires, REFUEL for a healthy low-fuel vehicle, then ENGINE TUNE / TIRE UPGRADE until L3/L3, finally COMPLETE.
4. PASS: the office shows applicable fuel, tire-service and next-upgrade costs before the player drives to a terminal.
5. PASS: active Rattleback/Mulebox values come from the Native pawn, not a stale hidden mirror.

## Regression pass
- Garage recall remains transport only and never repairs/tunes/refuels for free.
- Player-selectable roadside tow remains separate from garage recall and workshop service.
- Native structural damage, breakdown recommendations and tow estimates remain visible.
- Fieldmaster native tuning still uses its proven mirror-import bridge.
- Legacy non-Native owned vehicles retain their existing tuning/service paths.
- No GTA assets, names, maps, music, logos or code are introduced.

## Demo acceptance note
This milestone closes a real fleet progression/persistence gap, but it is not a demo-release approval. A public Windows demo still requires a real UE 5.8 Win64 compile/package, successful packaged-EXE runtime smoke/evidence, green relevant GitHub Actions, rendered visual acceptance and no demo-critical blockers.
