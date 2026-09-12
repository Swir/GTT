# Changelog

All notable development steps for GTT are tracked here.

## [0.0.23] - 2026-09-12

### Added
- **Main Story Arc 4 — North Pass Run**, unlocked after Red Barn Reckoning and deliberately reusing the existing hostile-faction, poaching/fence, wanted/police and shared-routing systems.
- Arc 4 progression: win one fresh hostile-territory encounter, prepare at least two persistent contraband units, sell the stash through Backlot Fence, hit North Pass, escape a real police response, complete Ridge Exchange, then return to Player Farm.
- Persistent `GTT_MainStory_Arc4_01` state for campaign stage, baseline faction victories and prepared-stash state.
- Five new shared road nodes: North Pass Approach, North Pass Checkpoint, River Ford, Ridge Exchange and Quarry North Cut.
- Runtime North Pass/Ridge landmarks and physical Arc 4 interaction terminals through `UGTTArc4WorldSubsystem`.
- `$1250` Ridge Exchange payout and `$1500` Arc 4 completion payout.
- Dedicated `verify_arc4.py` sanity suite and GitHub Actions step.

### Changed
- Shared road graph expands from 23 to 28 connected nodes.
- Campaign now consumes persistent faction victories and contraband state instead of creating parallel story-only counters.
- Roadmap advances from `111/129 (86.0%)` to the exactly recalculated `113/130 (86.9%)`, preserving `SWIR-ROADMAP-STANDARD:v1` and the 17/20 progress bar.

### Limitations / Next
- North Pass/Ridge landmarks are runtime greybox geometry rather than final authored art/interiors.
- Arc 4 still uses a dedicated SaveGame; consolidation of combat/story/faction/economy state remains outstanding.
- Full Unreal Engine 5.8 Win64 compile/package/smoke validation is still unavailable in repository CI; no packaged EXE verification is claimed.
- Next major package: save consolidation and performance foundations, followed by native Chaos wheel migration when a UE-equipped runner is available, plus dialogue/interior/content polish.

## [0.0.22] - 2026-09-12
- Added persistent rural law/economy: contraband and Backlot Fence, Farm Mutual insurance, vehicle impound/release, authored road speed/lane/priority metadata and speeding/reckless-driving enforcement.
- Roadmap: `111/129 (86.0%)`.

## [0.0.21] - 2026-09-12
- Added Rust Dogs, Stone Crows and Mud Jackals persistent factions, four hostile archetypes, escalating proximity encounters and three new countryside territory branches.

## [0.0.20] - 2026-09-12
- Added articulated rigid-body farm trailer, breakable hitch, cargo stability and Heavy Timber Haul contract.

## [0.0.19] - 2026-09-12
- Added Main Story Arc 3 — Red Barn Reckoning, persistent Rural Arsenal loadout/ammo, combat ambush, police escape and county evidence haul.

## [0.0.18] - 2026-09-12
- Added player health, eight-item Rural Arsenal, physical pickups/drops, melee/shotgun combat, civilian retaliation and Bent Axle Brawl.

## [0.0.17] - 2026-09-12
- Added four-contact vehicle dynamics, suspension traces, multi-gear drivetrains, differentiated Fieldmaster/Rattleback/Mulebox profiles, mud integration and HUD telemetry.

## [0.0.16] - 2026-09-12
- Added shared road graph, rural commuter routes and Main Story Arc 2 — Timber Ghosts with ranger escalation and route guidance.

## [0.0.15] - 2026-09-12
- Added persistent Main Story Arc 1 — County Ledger / Backroad Deal / Final Farm Meet.

## [0.0.14] - 2026-09-12
- Added predictive police interception, roadblocks and explicit garage slots.

## [0.0.13] - 2026-09-12
- Added physical roadside recovery towing and mud/tire-wear zones.

## [0.0.12] - 2026-09-12
- Added Legal Timber Haul, Field Mowing and Night Shift Favor side story.

## [0.0.11] - 2026-09-12
- Added fictional radio stations, village nightlife, night events, roadblocks and spike strips.

## [0.0.10] - 2026-09-12
- Added police pursuit vehicles and staged feed-cargo delivery.

## [0.0.9] - 2026-09-12
- Added garage recall, tuning, tire integrity and forest poaching.

## [0.0.8] - 2026-09-12
- Added staged vehicle damage, detachable parts, smoke, overheating and smarter traffic avoidance.

## [0.0.7] - 2026-09-12
- Added autonomous village traffic and separate ranger/game-warden response.

## [0.0.6] - 2026-09-12
- Added multi-vehicle garage/save plus Rattleback 82 old car and Mulebox 1200 van.

## [0.0.5] - 2026-09-12
- Added persistent save/load, garage, police arrest, day/night, NPC schedules and first legal farm job.

## [0.0.4] - 2026-09-12
- Added economy, fishing, fuel, workshop, fish buyer and civilian witnesses.

## [0.0.3] - 2026-09-12
- Added Rusty Fieldmaster 60, Borrowed Tractor mission, runtime countryside and Windows packaging helper.

## [0.0.2] - 2026-09-12
- Added physics driving fallback, damage, theft crime, wanted integration, HUD and police chase AI.

## [0.0.1] - 2026-09-12
- Initial Unreal Engine 5 C++ project structure and core gameplay foundations.
