# GTT 0.0.46 — Native Fieldmaster Workshop & Tuning Integration

This milestone closes a gameplay-state regression created by the 0.0.45 Native Fieldmaster takeover. When the Native Chaos representation is active, workshop and tuning interactions must mutate the active native gameplay state rather than silently modifying the hidden legacy compatibility mirror and then losing those changes on the next mirror sync.

## Workshop repair + refuel

1. Activate the authored Native Fieldmaster takeover with the tractor owned and unoccupied.
2. Reduce tractor condition and fuel, then park the active Native Fieldmaster inside workshop range.
3. Use the normal Repair + Refuel terminal.
4. Confirm the existing $75 charge occurs once.
5. Confirm the service mutates the canonical legacy compatibility implementation and immediately re-imports condition/fuel into the active Native Fieldmaster snapshot.
6. Wait longer than the 0.5 s mirror sync interval and confirm repaired/refuelled state does not revert.
7. Quick-save, reload and verify the serviced state persists.
8. Repeat with Rattleback and Mulebox and confirm their legacy workshop behavior is unchanged.

## Native tuning lifecycle

1. Park the active owned Native Fieldmaster in tuning range.
2. Damage tires below 80%; use the terminal and confirm the existing tire-service price is charged once, tires return to 100%, and native state refreshes immediately.
3. With healthy tires, buy engine tune L1/L2/L3 in sequence. Confirm costs, existing three-level cap and save behavior remain unchanged.
4. Buy tire upgrades L1/L2/L3 and verify the same behavior.
5. Wait through several native mirror sync intervals after every operation; no upgrade or tire state may roll back.
6. Quick-save/quick-load and garage recall after tuning; state must persist through the legacy-compatible save path.
7. Confirm Rattleback/Mulebox tuning is unchanged.

## Target selection / hidden-mirror safety

- While Native takeover is active, workshop/tuning must prefer the visible active `AGTTFieldmasterNativePawn` and must not present the hidden legacy Fieldmaster as an independent nearby service target.
- The hidden mirror remains the implementation bridge for existing repair/refuel/tuning rules, but every successful mutation must be immediately imported back into the Native Fieldmaster before the next periodic sync.
- If the compatibility mirror cannot be resolved, service/tuning must fail visibly rather than charging the player and losing state.
- Failed native synchronization must return the cash charged by the attempted operation.

## Regression pass

- Borrowed Tractor ownership/wanted flow before native takeover.
- Native enter/drive/exit, fuel consumption, radio and garage recall.
- Police and game-warden service restrictions.
- Heavy haul and trailer attachment.
- Farm/timber/recovery jobs.
- Save/load of all three garage vehicles.

## DEMO / Win64 gate

This is source-level gameplay integration only. Keep both Native Chaos roadmap items open until an authored skeletal mesh + Physics Asset is actually compiled and driven under UE 5.8. Keep the Win64/runtime tasks open until the dedicated evidence workflow produces and smoke-tests a packaged `GTT.exe`. Do not publish a demo without rendered visual acceptance and the existing technical gates.
