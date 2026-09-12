# Changelog

All notable development steps for GTT are tracked here.

## [0.0.8] - 2026-09-12

### Added
- Shared staged vehicle-damage framework in `AGTTVehicleBase`.
- Breakable vehicle parts that physically detach, collide and tumble after condition thresholds are crossed.
- Repair integration that reattaches staged breakable parts after a sufficiently strong/full repair.
- Source-only visible damage smoke made from animated primitive-mesh puffs, so damage feedback works without third-party VFX assets.
- Engine-temperature simulation affected by throttle and vehicle condition.
- Overheating power loss, critical overheat damage and automatic engine shutdown.
- Random low-condition engine stalls with a short restart delay and throttle-based restart attempt.
- New vehicle HUD diagnostics: engine temperature, active fault state and detached-part count.
- Rattleback 82 staged damage: front/rear bumpers, both doors, trunk lid and right-rear wheel.
- Mulebox 1200 staged damage: front bumper, sliding cargo door, both rear doors and left-rear wheel.
- Rusty Fieldmaster 60 staged damage: both fenders, exhaust stack and hood.
- Traffic forward obstacle probe with braking/steering avoidance.
- Visible `BEEP!` horn pulse for blocked traffic cars without requiring an audio asset yet.
- Traffic stuck detection with route recovery and a small physics recovery impulse.
- Six-car ambient traffic default with clockwise and counter-clockwise traffic flows.

### Changed
- Vehicle power now degrades further when the engine is overheating.
- Damaged vehicles communicate mechanical state instead of only exposing a condition percentage.
- Persistent vehicle condition automatically reconstructs the appropriate damage stage after load.
- Ambient traffic can now react to obstacles instead of blindly pushing toward the next route point.
- Project sanity checks now validate breakable-part counts, mechanical faults, smoke hooks, traffic avoidance and damage HUD diagnostics.

### Next
- Add garage slot selection and vehicle recall.
- Add proper tire-specific grip loss and puncture behaviour.
- Add player tuning/upgrades and replacement body panels.
- Add forest/poaching gameplay feeding the ranger system.
- Start dedicated Chaos wheeled tractor/car drivetrain implementation.
- Add a real Unreal-equipped Win64 build/smoke-test runner.

## [0.0.7] - 2026-09-12

### Added
- Autonomous village traffic prototype with physics-driven route following.
- `AGTTTrafficDirector` that spawns multiple ambient cars around the main road loop.
- Dedicated traffic vehicles that do not consume garage slots or behave like parked theft targets.
- Separate game-warden authority system for wildlife crime, independent from police wanted.
- Three-level `WARDEN` alert with quiet-time delay and gradual heat decay.
- Ranger director, ranger pawn and ranger pursuit AI with alert-scaled chase speed.
- Ranger citation flow with cash fine and confiscation of carried fish.
- HUD `WARDEN [---]` / alert display alongside police wanted.
- Runtime game-warden outpost near the private lake and visible live-traffic signage.
- Legal jobs and garage service now refuse interaction while either police or the game warden are actively looking for the player.

### Changed
- Restricted fishing builds wildlife/game-warden heat instead of directly using the normal police wanted system.
- Fishing, police and ranger consequences are separate gameplay loops.

## [0.0.6] - 2026-09-12

### Added
- Version-2 multi-vehicle save format and migration from the earlier tractor-only save.
- Four-slot garage, vehicle registration and garage HUD occupancy.
- **Rattleback 82** old compact car and **Mulebox 1200** farm van.
- Distinct mass, condition, handling, fuel and theft-heat profiles.

## [0.0.5] - 2026-09-12

### Added
- Persistent SaveGame model, quick-save/load and autosaves.
- Player garage and vehicle ownership.
- Police arrest flow and fines.
- Day/night cycle and NPC schedules.
- First legal farm-job loop.

## [0.0.4] - 2026-09-12

### Added
- Player economy, fishing inventory, fuel, workshop and fish buyer.
- Illegal fishing prototype and civilian crime witnesses.
- Eight runtime-spawned villagers.

## [0.0.3] - 2026-09-12

### Added
- Dedicated prototype tractor `Rusty Fieldmaster 60`.
- Complete Borrowed Tractor source-driven gameplay loop.
- Runtime-generated greybox countryside and Windows packaging helper.

## [0.0.2] - 2026-09-12

### Added
- Physics driving fallback, vehicle damage, theft crimes and wanted integration.
- Native HUD, police chase AI and automatic police response.

## [0.0.1] - 2026-09-12

### Added
- Initial Unreal Engine 5 C++ project structure.
- Character, interaction, vehicle, wanted, police and mission foundations.
- Input, packaging, Git LFS, roadmap, design docs and GitHub Actions sanity workflow.
