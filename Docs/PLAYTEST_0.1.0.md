# GTT 0.1.0 Playtest — Mission-Aware Fleet Dispatch & Vehicle Loadouts

This milestone connects garage ownership, persistent role loadouts, vehicle damage/service state, and legal work into one player-facing loop. Source/contract sanity is not a substitute for a real Unreal Engine 5.8 Win64 playtest; the runtime route below must still be performed on an Unreal-capable machine before any demo release.

## 1. Persistent role loadouts
1. Load a profile with Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200 owned.
2. Dispatch the Fieldmaster from its numbered garage bay. Confirm the bay/office marks it `ACTIVE` and `LOADOUT`, and the fleet summary lists it under the tractor loadout.
3. Dispatch Rattleback and then Mulebox. Confirm the road and cargo loadouts update independently while the most recently dispatched vehicle alone remains `ACTIVE`.
4. Save, quit, and reload.

**Pass:** tractor, road and cargo role loadouts all survive save/load independently; the active dispatch choice also survives.

## 2. Pre-v7 migration
1. Load a v6 save with at least one owned vehicle per role.
2. Open the garage office after load.
3. Confirm each available role receives a valid owned fallback instead of a missing/stale vehicle ID.
4. Confirm Native structural body damage from the v5 schema is still restored.

**Pass:** v7 migration never loses v5 structural damage or v6 active dispatch state and seeds valid role loadouts where possible.

## 3. Farm cargo readiness
1. Damage Mulebox below the recommended margin but keep it mobile; leave enough fuel to drive.
2. Start the farm cargo contract.
3. Confirm the player receives a `CAUTION`/service-oriented fleet message with the Mulebox garage slot and prep estimate.
4. Complete once with another healthy vehicle, then once with Mulebox.

**Pass:** farm cargo remains sandbox-flexible when the cargo loadout is worn, while the existing Mulebox role bonus still pays only when Mulebox is actually used.

## 4. Heavy haul tractor gate
1. Put the Fieldmaster tractor loadout into a critically unfit state (very low condition, fuel or tire integrity).
2. Attempt to accept heavy haul.
3. Confirm the contract refuses to start and reports that the TRACTOR loadout needs preparation.
4. Service/refuel the Fieldmaster to mission-ready state, dispatch it again if needed, and retry with it near the trailer yard.

**Pass:** a critically unfit tractor cannot silently start heavy haul; after real service the existing hitch/load/trailer gameplay starts normally.

## 5. Timber cargo guidance
1. Make Mulebox the cargo loadout and leave it healthy.
2. Start rural timber work and confirm a `READY` cargo-loadout message names the correct garage slot.
3. Damage/service-starve Mulebox and retry after the previous job ends.

**Pass:** timber work warns about a poor cargo loadout but remains playable with another suitable healthy vehicle.

## 6. Mowing tractor gate
1. Make the Fieldmaster tractor loadout critically unfit.
2. Attempt to start mowing.
3. Confirm the job is blocked before route activation.
4. Restore the tractor above readiness thresholds and park it at the field office.
5. Start mowing and complete the five existing field gates.

**Pass:** mowing requires both mission-ready tractor state and the existing physical tractor-near-office condition.

## 7. Garage/economy/crime regression
1. Raise wanted and try dispatch; repeat with a game-warden alert.
2. Clear both states and dispatch a vehicle.
3. Confirm exactly $15 is charged, damage/fuel/tuning are preserved, the role loadout changes, and progress saves.

**Pass:** 0.1.0 adds mission loadouts without bypassing existing legal-state locks or turning dispatch into a free repair.

## 8. Native Chaos authority
1. Activate Native Rattleback/Mulebox takeover, alter fuel/damage/tuning, and dispatch the vehicle.
2. Confirm Native motion is stopped and the persistence mirror is flushed before save.
3. Reload and verify both physical vehicle state and role-loadout selection.

**Pass:** mission planning reads the same authoritative Native-backed garage snapshot rather than stale compatibility state.

## 9. Demo-release reminder
A source-level PASS here is **not** a demo-release approval. Before a public Windows demo, separately verify:
- real UE 5.8 Win64 compile/package,
- packaged EXE runtime smoke on the exact artifact,
- green relevant GitHub Actions,
- rendered visual acceptance for world, vehicles, characters/weapons, HUD/UI and lighting,
- no demo-critical placeholder clutter or blockers.

Do not publish a demo Release until all of those gates are actually satisfied.
