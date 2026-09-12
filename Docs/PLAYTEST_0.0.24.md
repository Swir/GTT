# GTT 0.0.24 — Unified World State Playtest

This milestone consolidates the previously separate combat, story, Arc 3, Arc 4, rural-faction and rural-economy progression into the primary `GTT_Prototype_01` sandbox SaveGame while retaining the old slots as compatibility mirrors.

## Migration regression

1. Start from a 0.0.23 profile with at least one collected Rural Arsenal weapon, shotgun ammunition, non-zero faction notoriety, Arc 3/4 or main-story progress, and rural-economy state such as contraband/insurance/citations.
2. Launch 0.0.24 and allow world startup to complete.
3. Save normal sandbox progress, quit, and relaunch.
4. Confirm cash, fish, player position, owned vehicles/tuning, combat inventory/ammo, story stages, faction counters, contraband, insurance and impound/citation state all survive together.
5. Confirm no campaign stage moves backward and no collected weapon is duplicated or removed.

## Compatibility mirrors

- Delete or temporarily move one dedicated legacy slot after a successful 0.0.24 save, then relaunch. The unified primary snapshot must recreate a usable compatibility mirror without resetting the primary sandbox state.
- Existing dedicated saves from 0.0.23 must import into the primary snapshot on first consolidation.
- A completely new profile must not have `GTT_Prototype_01` manufactured by the migration subsystem before normal GameMode save creation.

## Live consolidation

1. Pick up/drop a rural weapon or fire shotgun ammunition.
2. Win a hostile-faction encounter.
3. Advance a story stage or Arc 4 state.
4. Add/sell contraband or change insurance/impound state.
5. After the consolidation interval, perform a normal save and relaunch.
6. Verify all four domains restore at their latest values together.

## Failure cases

- Corrupt/missing optional legacy slots must not prevent the primary save from loading.
- The migration layer must not create a primary save on a brand-new profile.
- Values are clamped by their owning gameplay systems after load; the consolidator only mirrors persisted state and must not invent rewards, weapons, faction wins or story advancement.

## CI / build boundary

`Scripts/verify_unified_save.py` verifies field coverage, legacy-slot migration hooks, compatibility mirrors, workflow integration and exact SWIR roadmap arithmetic. Repository CI still does **not** provide a full Unreal Engine 5.8 Win64 compile/package/smoke environment, so this milestone does not claim a verified packaged EXE.
