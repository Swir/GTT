# Changelog

All notable development steps for GTT are tracked here.

## [0.0.37] - 2026-09-13

### Added
- **Village Presentation Pass** milestone focused on making the current playable sandbox cleaner and more readable before any public demo is considered.
- Runtime `UGTTVillagePresentationSubsystem` scoped to the GTT prototype world, with 14 source-built roadside lamp fixtures tied directly to the existing `AGTTDayNightCycle`.
- Roadside reflector/delineator props around the main village loop plus non-colliding hay-bale and timber-stack dressing at Hill Farm and North Wood Yard.
- Dedicated `PLAYTEST_0.0.37.md` and `verify_village_presentation.py` coverage for lighting, label readability, rural dressing, regression safety and demo-acceptance honesty.

### Changed
- World-space prototype labels now use a 1900 cm readability radius, keeping nearby mission/service information while suppressing the distant wall-of-debug-text effect across the countryside.
- The stale runtime prototype banner is normalized to `GTT 0.0.37 | LIVING VILLAGE SANDBOX` by the presentation layer without changing gameplay triggers or save data.
- Street lights use bounded attenuation and no shadow casting, and automatically switch with the real day/night state instead of running a disconnected presentation clock.
- Roadmap remains exactly `125/130 (96.2%)`: this is a genuine visual/readability milestone but does not substitute for Native Chaos runtime acceptance, authored trailer assets or a verified Win64 build runner.

### Limitations / Next
- The visual pass deliberately uses original source-built Unreal primitives and is not a claim of final environment art, final vehicle art or a visually approved packaged demo.
- Full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation is still unavailable on the current repository sanity runner; no packaged EXE verification is claimed.
- Next large package should continue reducing prototype presentation debt while advancing the first truly activatable Native Chaos vehicle path or the missing Unreal-capable Windows runner.

## [0.0.36] - 2026-09-13

### Added
- **Native Rig Acceptance & Hitch Integration** milestone hardening the fleet Chaos bridge so an arbitrary skeletal mesh can no longer trigger native takeover.
- Runtime validation of every required `FGTTChaosRigContract` root/wheel bone and driver/exit/hitch socket before native movement is considered ready.
- Human-readable rig diagnostics through `GetRigValidationSummary()` plus rig-valid state in the existing Chaos bridge status summary.
- Validated native `rear_hitch` world-transform handoff to the articulated farm trailer, while current greybox vehicles retain the proven legacy hitch fallback.
- Dedicated `PLAYTEST_0.0.36.md` and `verify_native_rig_acceptance.py` CI coverage.

### Changed
- `UGTTChaosVehicleBridgeComponent` now requires canonical vehicle spec + `UChaosWheeledVehicleMovementComponent` + a complete matching skeletal rig contract before disabling `UGTTVehicleDynamicsComponent`.
- Half-migrated or incorrectly named skeletal rigs remain safely on legacy dynamics instead of silently disabling the working drivetrain.
- Trailer attachment now measures and anchors from the validated authored hitch socket when available rather than always using an approximate vehicle-relative point.
- Roadmap remains exactly `125/130 (96.2%)`: runtime acceptance gating is substantial migration progress but does not substitute for authored skeletal/physics assets or a real UE 5.8 packaged runtime test.

### Limitations / Next
- The repository still lacks the final original GTT skeletal meshes/physics assets required to activate Native Chaos in an actual play session.
- Full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation remains unavailable on the current sanity runner; no packaged EXE verification is claimed.
- Next package should create/activate the first real Fieldmaster native pawn/rig path and then perform the same acceptance process for Rattleback and Mulebox.

## [0.0.35] - 2026-09-13

### Added
- **Native Rig Architecture** milestone with six native `UChaosVehicleWheel` classes: front/rear classes for Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200.
- Wheel geometry, suspension, friction and steering values sourced directly from the canonical `FGTTChaosVehicleSpec` profiles rather than duplicated tuning data.
- Persistent-ID-aligned skeletal rig contracts defining `root`, four wheel bones, `driver_seat`, `driver_exit` and required `rear_hitch` sockets for Fieldmaster/Mulebox.
- Dedicated `PLAYTEST_0.0.35.md` and `verify_native_chaos_rig.py` CI coverage.

### Changed
- Chaos migration now has concrete wheel classes and one canonical fleet skeletal naming contract ready for authored meshes/physics assets.
- Roadmap intentionally stayed at `125/130 (96.2%)` because source architecture alone is not runtime acceptance.

### Limitations / Next
- No final skeletal/physics vehicle asset is present yet, so production Native Chaos handling is not claimed.
- Full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation remains unavailable on the current sanity runner; no packaged EXE verification is claimed.

## [0.0.34] - 2026-09-13

### Added
- **Chaos Fleet Runtime Bridge** milestone extending the gameplay-aware Chaos migration path from Rusty Fieldmaster 60 to all three owned vehicles: Fieldmaster, Rattleback 82 and Mulebox 1200.
- Rattleback and Mulebox now each own `UGTTChaosVehicleBridgeComponent`, mirror the existing throttle/steering axes into the shared bridge and preserve the base/legacy driving path until a real native rig exists.
- Fleet-wide canonical persistent-ID/spec resolution so `RustyFieldmaster60`, `Rattleback82` and `Mulebox1200` all feed the same fuel/condition/tuning/tire-aware Chaos bridge contract.
- Dedicated `PLAYTEST_0.0.34.md` and `verify_chaos_fleet_bridge.py` coverage for all three vehicles, fallback safety and roadmap honesty.

### Changed
- The 0.0.33 single-tractor bridge is now a reusable fleet migration layer rather than a Fieldmaster-only integration path.
- Rattleback workshop upgrades and Mulebox cargo/farm-job state remain authoritative because native input scaling consumes the existing persisted vehicle condition, fuel, engine tuning and tire state instead of duplicating gameplay data.
- Roadmap remains exactly `125/130 (96.2%)`: source/runtime bridge readiness is meaningful progress, but Native Chaos tasks remain open until authored skeletal/physics/wheel rigs and UE 5.8 runtime acceptance exist.

### Limitations / Next
- The repository still lacks validated native skeletal/physics/wheel rigs for the fleet, so no vehicle is claimed to have switched to production Chaos handling yet.
- Full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation remains unavailable on the current sanity runner; no packaged EXE verification is claimed.
- Next package should author the first real Fieldmaster native rig/wheels/hitch path, then apply the same asset/runtime acceptance process to Rattleback and Mulebox.

## [0.0.33] - 2026-09-13

### Added
- **Fieldmaster Chaos Runtime Bridge** milestone that moves the Rusty Fieldmaster 60 beyond a spec-only migration plan and into gameplay-state-aware Chaos integration.
- `UGTTChaosVehicleBridgeComponent` with native-rig detection, canonical vehicle-spec resolution, bridge-state telemetry and automatic fallback protection.
- Native input routing for throttle, steering, forward/reverse intent and braking when a real `UChaosWheeledVehicleMovementComponent` plus skeletal vehicle body are present.
- Shared vehicle-state integration so fuel, engine-running state, condition, engine tuning, tire integrity and tire tuning influence the inputs sent to Chaos instead of creating disconnected stats.
- Automatic disabling of the legacy source-driven dynamics tick once a valid native rig is detected, preventing two drivetrain systems from applying force simultaneously.
- Dedicated `PLAYTEST_0.0.33.md` and `verify_fieldmaster_chaos_bridge.py` coverage.

### Changed
- Rusty Fieldmaster 60 now mirrors its existing keyboard/controller vehicle axes into the Chaos bridge while preserving the current proven source-driven dynamics fallback when native skeletal/physics prerequisites are absent.
- The migration path now has an explicit runtime state machine (`WAITING / READY / DRIVING / BLOCKED`) rather than relying only on documentation and static target specs.
- Roadmap remains honestly unchanged at exactly `125/130 (96.2%)`: the bridge is substantial implementation progress, but neither Native Chaos checkbox is closed until an authored skeletal/physics rig passes real UE 5.8 runtime acceptance.

### Limitations / Next
- The current repository still does not contain the final Fieldmaster skeletal mesh, physics asset, wheel assets or validated native Chaos vehicle pawn, so handling is not claimed to have switched in the playable greybox build yet.
- Full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation remains unavailable on the current repository sanity runner; no packaged EXE verification is claimed.
- Next package should add the authored Fieldmaster skeletal/physics/wheel rig and activate this bridge against a real native movement component, then repeat the same integration for Rattleback 82 and Mulebox 1200.

## [0.0.32] - 2026-09-13

### Added
- **Combat Presentation** milestone integrated with the existing Rural Arsenal rather than a parallel demo system.
- Source-authored runtime weapon visuals for all eight non-hand arsenal items: Pitchfork, Axe, Branch, Rake, Cow Chain, Shovel, Workshop Wrench and Old Farm Shotgun.
- Player melee swing and shotgun recoil presentation driven directly from the equipped `UGTTCombatComponent` weapon state.
- Hostile NPC combat props for Scrapper, Runner, Bruiser and Enforcer plus visible attack-swing presentation when their existing retaliation damage fires.
- Directional body/head hit reactions and a persistent visible knockout pose that recovers through the existing knockout timer.
- Dedicated `PLAYTEST_0.0.32.md` and `verify_combat_presentation.py` coverage, including a guard against imported third-party combat model files.

### Changed
- Combat-critical world-performance simulation now also treats active hit/attack presentation windows as urgent so reactions are not throttled while a fight is visible.
- Dropping or cycling a player weapon immediately resynchronizes the held visual with the real persistent inventory/equipped state.
- Roadmap advances from `124/130 (95.4%)` to exactly `125/130 (96.2%)`; 5 tasks remain and the 20-segment bar remains mathematically correct at 19/20 while preserving `SWIR-ROADMAP-STANDARD:v1`.

### Limitations / Next
- Weapon models and motion are deliberately original source-built runtime geometry/animation, not a claim of final skeletal mocap-quality art.
- Full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation remains unavailable on the current sanity runner; no packaged EXE verification is claimed.
- Native Chaos vehicle tasks, authored trailer wheel/hitch assets and the Unreal-capable Win64 runner remain open until they can be validated honestly.

## [0.0.31] - 2026-09-13

### Added
- **Original Radio Audio** runtime: the four existing fictional stations now produce audible, project-owned music beds instead of metadata-only track names.
- Deterministic procedural PCM synthesis for all 16 fictional programs, with station-specific tempo and sound palettes for GRAVEL FM, BARNBEAT 96, RUST & DIESEL and NIGHT SHIFT.
- Runtime `UAudioComponent` + `USoundWaveProcedural` playback path with automatic 36-second program transitions and immediate stop on `RADIO OFF`.
- Live integration with the existing persistent `MasterVolume` and `RadioVolume` player settings.
- Dedicated `PLAYTEST_0.0.31.md` and `verify_radio_audio.py` coverage, including a guard that no third-party WAV/MP3/OGG/FLAC/AAC/M4A files are required by this milestone.

### Changed
- The radio HUD now explicitly identifies the active program as original GTT audio while preserving the existing station/title rotation and keyboard/controller cycling flow.
- The 0.0.30 Chaos migration regression test is now forward-compatible with later roadmap progress while still requiring both Native Chaos tasks to remain open until real skeletal/physics assets and UE runtime acceptance exist.
- Roadmap advances from `123/130 (94.6%)` to the exactly recalculated `124/130 (95.4%)`; 6 tasks remain and the 20-segment bar remains mathematically correct at 19/20 while preserving `SWIR-ROADMAP-STANDARD:v1`.

### Limitations / Next
- Audio is intentionally synthesized at runtime from original GTT code, avoiding licensed samples or copyrighted recordings; it is not a claim of final studio-produced soundtrack quality.
- Full Unreal Engine 5.8 audio-device validation and Win64 compile/package/runtime smoke testing remain unavailable on the current repository sanity runner, so no packaged EXE verification is claimed.
- Next major package should return to vehicle feel with the first true Fieldmaster Chaos pawn/assets or close another remaining release gap such as authored combat animations/models and the Unreal-capable Win64 runner.

## [0.0.30] - 2026-09-13

### Added
- **Chaos Migration Foundation** with canonical UE 5.8 target specifications for the Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200.
- Source-level `FGTTChaosVehicleSpec` / `FGTTChaosWheelSpec` contracts covering mass, torque/RPM, final drive, steering, drive layout, front/rear wheel geometry, suspension, friction and explicit gear ratios.
- A persistent-ID lookup layer that maps the existing save-game vehicle identities directly to their future Chaos profiles.
- `CHAOS_VEHICLE_MIGRATION.md`, `PLAYTEST_0.0.30.md` and `verify_chaos_migration.py` so future native conversion is gated by repeatable source and runtime acceptance criteria.

### Changed
- Project sanity now verifies Chaos plugin/module wiring, all three canonical vehicle profiles, persistent-ID consistency and the migration acceptance boundary.
- Native Chaos roadmap tasks intentionally remain open at exactly `123/130 (94.6%)`: the current runtime vehicles still use static-mesh pawns and cannot honestly be called native Chaos vehicles until skeletal meshes, physics assets, Chaos wheel setups, a real `AWheeledVehiclePawn` path and UE 5.8 runtime validation exist.

### Limitations / Next
- This milestone does not replace the current source-driven vehicle simulation and therefore does not claim a gameplay handling change yet.
- Full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation remains unavailable on repository CI; no packaged EXE verification is claimed.
- Next major package should author the first true Fieldmaster skeletal/physics vehicle asset path and wire its fuel, damage, tire, mud, tuning, trailer and save systems into `UChaosWheeledVehicleMovementComponent` before migrating Rattleback and Mulebox.

## [0.0.29] - 2026-09-13

### Added
- **Workshop Customization** milestone with three new workshop bays tied to owned vehicles, player cash and existing persistent tuning/damage systems.
- Staged **Rusty Fieldmaster 60 visual packages**: brush guard, work-light bar/lamps and rear toolbox, driven by the same engine/tire tune levels already persisted in the sandbox save.
- Staged **Rattleback 82 street/performance variant** with hood scoop, ducktail and wider lip package while reusing the real drivetrain power/grip upgrades rather than introducing disconnected stats.
- **Replacement body-panel economy** that prices repairs from the actual detached-part count and routes restoration through the existing breakable-part/vehicle-repair logic.
- Automatic world-subsystem workshop spawning plus dedicated `verify_vehicle_customization.py` and `PLAYTEST_0.0.29.md` coverage.

### Changed
- Vehicle customization now consumes the existing workshop economy and ownership checks, and successful purchases immediately save through `AGTTGameMode::SaveProgress()`.
- Visual packages are rebuilt from existing persistent engine/tire tune levels, so the functional upgrade state survives quit/relaunch even though the current milestone deliberately uses runtime original greybox geometry.
- Roadmap advances from `120/130 (92.3%)` to exactly `123/130 (94.6%)`; 7 tasks remain and the mathematically rounded 20-segment bar is now 19/20 while preserving `SWIR-ROADMAP-STANDARD:v1`.

### Limitations / Next
- Vehicle styling is intentionally source-created greybox/procedural geometry; authored final meshes/materials remain outside this milestone.
- Full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation is still unavailable on repository CI; no packaged EXE verification is claimed.
- Next major package should target native Chaos vehicle movement, authored trailer/combat assets or cleared original radio/music content, with the full Win64 runner still required before release acceptance.

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
- Full Unreal Engine 5.8 Win64 compile/package/runtime smoke validation is still unavailable in repository CI; no packaged EXE verification is claimed.
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
- Roadmap advances from `115/130 (88.5%)` to exactly `117/130 (90.0%)`; the 20-segment bar remains mathematically correct at 18/20 while preserving `SWIR-ROADMAP-STANDARD:v1`.

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
