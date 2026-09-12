# Changelog

All notable development steps for GTT are tracked here.

## [0.0.28] - 2026-09-13

### Added
- **Village Social Life** milestone with playable Bent Axle Tavern and Community Hall interiors built as physical runtime spaces with bidirectional interactive doors.
- Four social NPC roles — bartender, hall organizer, local mechanic and hill farmer — with repeatable contextual conversations instead of static scenery.
- Time-aware dialogue tied directly to the existing `AGTTDayNightCycle`, including the real 18:30–02:30 village-party window.
- Vehicle-aware mechanic dialogue that reads the nearest owned vehicle's real display name, condition, fuel percentage and tire integrity, so damage/repair/refuel/tire systems feed social feedback.
- Dedicated `verify_social_venues.py` regression coverage and `PLAYTEST_0.0.28.md` for interiors, dialogue, vehicle-state advice, controller interaction and existing nightlife regression.

### Changed
- Village nightlife now has enterable destinations and social interactions connected to the existing day/night, vehicle, workshop, controller and activity loops rather than exterior-only landmarks.
- The 0.0.27 release regression check now validates its completed packaging milestone without freezing the whole roadmap at an old exact count, preventing legitimate later milestones from failing CI.
- Roadmap advances from `118/130 (90.8%)` to exactly `120/130 (92.3%)`; 10 tasks remain and the mathematically rounded 20-segment bar remains 18/20 while preserving `SWIR-ROADMAP-STANDARD:v1`.

### Limitations / Next
- Interiors and social characters intentionally use original runtime greybox primitives; this milestone does not claim final authored environment art, character models, animation or voice acting.
- Full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation is still unavailable on repository CI; no packaged EXE verification is claimed.
- Next major package should target native Chaos Vehicles and/or authored vehicle/combat assets, with cleared original radio/audio also remaining before the first complete release.

## [0.0.27] - 2026-09-13

### Added
- **Release Pipeline** milestone with a dedicated Windows/UE 5.8 GitHub Actions workflow for project-controlled self-hosted runners.
- Post-package validation that requires a real `GTT.exe`, cooked `.pak`/`.utoc`/`.ucas` containers, expected runtime folders and a plausible package size.
- Release metadata and integrity artifacts: `BUILD_INFO.json`, `PACKAGE_VALIDATION.json`, per-file `SHA256SUMS.txt`, compressed release ZIP and ZIP SHA-256 sidecar.
- Dedicated `verify_release_pipeline.py`, `RELEASE_WINDOWS.md` and `PLAYTEST_0.0.27.md` covering repository, artifact-integrity and packaged-runtime acceptance gates.

### Changed
- `package_windows.ps1` now defaults to Shipping, enables IoStore/prerequisites, clears stale output, validates the result, records the exact Git SHA/build metadata and creates a compressed distributable artifact.
- Project sanity now verifies that release automation remains wired, honest about its runner requirements and consistent with the SWIR roadmap dashboard.
- Roadmap advances from `117/130 (90.0%)` to exactly `118/130 (90.8%)`; the 20-segment bar remains mathematically correct at 18/20 and preserves `SWIR-ROADMAP-STANDARD:v1`.

### Limitations / Next
- The release workflow requires a project-controlled `self-hosted, Windows, X64, unreal-5.8` runner. No such runner is claimed to have executed in this source-only milestone.
- Therefore full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation remains open and no EXE verification is claimed.
- Next major package should target native Chaos Vehicles or one of the remaining player-facing content gaps: interiors/dialogue, authored combat/vehicle visuals or cleared radio/audio assets.

## [0.0.26] - 2026-09-12

### Added
- **Player Experience** milestone with a custom persistent `UGTTGameUserSettings` class for look sensitivity, invert-Y, HUD/subtitle scale, reduced camera motion preference, color-vision mode, master/radio volume, controller vibration and stick dead-zone preference.
- Full current-gameplay Xbox-compatible controller mappings for on-foot movement/look, jump, interaction, combat, weapon cycling/drop, quick save/load, radio, vehicle throttle/reverse, steering and exit.
- Dedicated `verify_player_experience.py` sanity suite and `PLAYTEST_0.0.26.md` covering controller and persistence regressions.

### Changed
- Character camera input now consumes persistent GTT look sensitivity and invert-Y settings instead of hard-wiring raw input values.
- Unreal now uses `UGTTGameUserSettings` as the project GameUserSettings implementation so player preferences live in the normal per-user configuration path and remain separate from campaign/world saves.
- Roadmap advances from `115/130 (88.5%)` to exactly `117/130 (90.0%)`; the 20-segment bar remains mathematically correct at 18/20 and preserves `SWIR-ROADMAP-STANDARD:v1`.

### Limitations / Next
- The persistent settings API is ready for a polished settings menu, but this source milestone does not claim final menu art/layout or runtime validation of every controller model.
- Full Unreal Engine 5.8 Win64 compile/package/smoke validation remains unavailable in repository CI; no packaged EXE verification is claimed.
- Next major package should target packaging/release automation and/or native Chaos Vehicles once an Unreal-capable runner exists, while remaining content gaps are interiors/dialogue, final authored combat/vehicle art and cleared audio.

## [0.0.25] - 2026-09-12

### Added
- **World Performance** subsystem with distance-based `Critical / Near / Mid / Far / Dormant` simulation tiers shared by ambient world actors.
- Central tick-budget API with explicit player-distance thresholds and a separate expensive-query gate so future NPC, traffic and world systems can consume one consistent budget policy instead of inventing their own timers.
- Dedicated `verify_world_performance.py` sanity suite covering simulation tiers, civilian/traffic wiring, combat wake-up behavior, CI integration and exact SWIR roadmap arithmetic.
- Dedicated `PLAYTEST_0.0.25.md` regression/performance plan with dense-village, long-distance, traffic and combat checks.

### Changed
- Civilian NPCs now reduce schedule/movement update frequency as they move farther from the player; very distant ambient civilians become simulation-dormant instead of consuming full-rate wandering work.
- Combat, brawl, knockout and hostile-faction civilians explicitly force the Critical tier, preserving immediate chase/attack/hit/recovery behavior regardless of normal ambient distance budgeting.
- Traffic cars now scale AI tick frequency by distance while retaining a capped update interval for physical road progression.
- Expensive traffic obstacle line traces run only in Critical/Near tiers; farther traffic continues following the shared road graph without paying the full avoidance-query cost.
- Roadmap advances from `114/130 (87.7%)` to the exactly recalculated `115/130 (88.5%)`; the 20-segment bar remains mathematically correct at 18/20 while preserving `SWIR-ROADMAP-STANDARD:v1`.

### Limitations / Next
- Repository CI verifies structural performance-budget integration but cannot measure real frame-time gains; Game Thread/FPS comparison still requires an Unreal-equipped local or CI runner.
- Police/ranger and mission directors are not yet migrated to the shared budget because active pursuit/mission logic is intentionally kept conservative until UE runtime profiling proves safe throttling points.
- Full Unreal Engine 5.8 Win64 compile/package/smoke validation remains unavailable in repository CI; no packaged EXE verification is claimed.
- Next major package should focus on accessibility/settings + controller support or continue toward native Chaos Vehicles once an Unreal-capable runner is available.

## [0.0.24] - 2026-09-12
- Added **Unified World State** using the primary sandbox SaveGame `GTT_Prototype_01`, with non-destructive legacy migration, compatibility mirrors and change-aware synchronization.

## [0.0.23] - 2026-09-12
- Added Main Story Arc 4 — North Pass Run, faction/contraband/police integration and North Pass countryside expansion.

## [0.0.22] - 2026-09-12
- Added persistent rural law/economy: contraband and Backlot Fence, Farm Mutual insurance, vehicle impound/release, authored road speed/lane/priority metadata and speeding/reckless-driving enforcement.

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
