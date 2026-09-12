# Changelog

All notable development steps for GTT are tracked here.

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
- Restricted fishing now builds wildlife/game-warden heat instead of directly using the normal police wanted system.
- Fishing, police and ranger consequences are now separate gameplay loops.
- Project sanity checks now validate traffic routing, ranger pursuit, wildlife alerts, fish confiscation and the new authority split.

### Next
- Add garage slot selection and vehicle recall.
- Add traffic obstacle avoidance, honking and multiple civilian vehicle variants.
- Add forest/poaching gameplay that also feeds the ranger system.
- Add breakable vehicle body panels, smoke and mechanical faults.
- Continue toward dedicated Chaos wheeled vehicle movement and a real Win64 compile runner.

## [0.0.6] - 2026-09-12

### Added
- Version-2 save format with `FGTTStoredVehicleData` and a persistent `OwnedVehicles` array.
- Multi-vehicle persistence by stable vehicle ID, including transform, condition and fuel for every owned vehicle.
- Migration path for existing 0.0.5 saves that only stored the Rusty Fieldmaster tractor.
- Four-slot prototype garage capacity with occupancy shown directly on the HUD.
- Centralized garage registration flow in `AGTTGameMode`.
- New old compact car: **Rattleback 82**.
- New farm van: **Mulebox 1200**.
- Distinct mass, condition, acceleration, steering, tank capacity, fuel burn and theft heat profiles.
- Runtime spawn locations for the Rattleback and Mulebox.
- Four visible parking-bay markers at the player farm.

### Improved
- Registering a vehicle clears its stolen-report state and permanently disables theft heat for that owned vehicle.
- Mission theft progression only advances for the actual Rusty Fieldmaster mission tractor.
- Save/load reports current garage occupancy.
- HUD shows `GARAGE owned/capacity` alongside day/time and job status.

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
