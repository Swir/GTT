# Changelog

All notable development steps for GTT are tracked here.

## [0.0.6] - 2026-09-12

### Added
- Version-2 save format with `FGTTStoredVehicleData` and a persistent `OwnedVehicles` array.
- Multi-vehicle persistence by stable vehicle ID, including transform, condition and fuel for every owned vehicle.
- Migration path for existing 0.0.5 saves that only stored the Rusty Fieldmaster tractor.
- Four-slot prototype garage capacity with occupancy shown directly on the HUD.
- Centralized garage registration flow in `AGTTGameMode`, including wanted checks, capacity checks, registration cost and autosave.
- New old compact car: **Rattleback 82**.
- New farm van: **Mulebox 1200**.
- Distinct mass, condition, acceleration, steering, tank capacity, fuel burn and theft heat profiles for the tractor, car and van.
- Primitive-mesh bodies and wheels for both new vehicle classes without third-party art dependencies.
- Runtime spawn locations for the Rattleback near the village shop and the Mulebox near the workshop.
- Four visible parking-bay markers at the player farm.
- Structural CI checks for unique persistent IDs, multi-vehicle save migration, new vehicle classes and garage capacity hooks.

### Improved
- Registering a vehicle now clears its stolen-report state and permanently disables theft heat for that owned vehicle.
- Mission theft progression only advances when the actual Rusty Fieldmaster mission tractor is stolen.
- Save/load reports current garage occupancy.
- Garage logic now lives in the game mode instead of duplicating ownership/payment logic inside the terminal.
- HUD now shows `GARAGE owned/capacity` alongside day/time and job status.
- `Docs/PLAYTEST.md` now documents save/load, arrests, day/night, legal work and the complete three-vehicle garage loop.
- Roadmap updated to reflect completed 0.0.5 systems and the new 0.0.6 vehicle milestone.

### Next
- Add road traffic routes and civilian driver AI.
- Add game-warden/ranger response distinct from police for fishing/forest crime.
- Add garage slot selection and vehicle recall.
- Add breakable body panels and stronger degradation feedback.
- Move the prototype drivetrain toward dedicated Chaos wheeled vehicle movement.

## [0.0.5] - 2026-09-12

### Added
- Persistent SaveGame model for cash, fish inventory, player position, first-mission completion, world time and tractor state.
- Automatic load on startup when a save exists, plus `F5` quick-save and `F9` quick-load.
- Tractor persistence for position, condition, fuel and player ownership.
- Player garage terminal with vehicle registration and save integration.
- The Rusty Fieldmaster becomes player-owned after completing `Borrowed Tractor`.
- Police arrest flow: close pursuit can now end in arrest, wanted reset, cash fine and release at the police station.
- Fine calculation scales with wanted level without allowing negative cash.
- Full day/night clock with configurable game-day duration and dynamic sun/skylight intensity.
- HUD day/time display and owned-vehicle marker.
- Civilian schedules: daytime work area, evening social area and night-time return home.
- Legal farm-job loop with start terminal, field-delivery terminal and $180 reward.
- Autosave after mission completion, arrest, garage use and legal job completion.

### Improved
- Owned vehicles no longer retrigger theft heat when entered.
- Economy can restore a saved state and apply clamped fines.
- Prototype world now contains garage, farm-job start and farm-job delivery locations.
- HUD controls include persistence shortcuts and active legal-job state.
- Repository sanity checks validate persistence, ownership, arrests, time-of-day and farm-job hooks.

## [0.0.4] - 2026-09-12

### Added
- Player economy component with starting cash, spending, rewards and short gameplay messages.
- Fish inventory tracking by count and total weight.
- Mission cash reward for completing `Borrowed Tractor`.
- Vehicle fuel capacity, starting fuel, idle/full-throttle consumption and out-of-fuel shutdown.
- Low-condition power loss layered on top of existing collision damage/breakdown behaviour.
- Tractor-specific 55 L fuel tank and deliberately low starting fuel.
- Workshop terminal that repairs and fully refuels the nearest parked vehicle for cash.
- Village fish buyer that converts the entire carried catch into cash by weight.
- Illegal fishing interaction at the private prototype lake.
- Three prototype catches: River Perch, Village Carp and Old Pike.
- Wanted heat for illegal fishing attempts.
- Wandering civilian prototype NPCs and line-of-sight crime witnesses.
- Eight runtime-spawned villagers.
- HUD economy and fuel information.

### Improved
- First mission pays $300 and bridges story gameplay into free roam.
- Prototype village contains a repeatable fish/sell/service loop.

## [0.0.3] - 2026-09-12

### Added
- Dedicated prototype tractor `Rusty Fieldmaster 60`.
- Mission safe-zone and complete Borrowed Tractor source-driven gameplay loop.
- Runtime-generated greybox countryside, roads, landmarks, lighting and mission actors.
- Windows packaging helper and expanded sanity checks.

## [0.0.2] - 2026-09-12

### Added
- Physics driving fallback, vehicle condition/damage, theft crimes and wanted integration.
- Native HUD, police chase AI and automatic police response.
- Borrowed Tractor mission start and first theft transition.

## [0.0.1] - 2026-09-12

### Added
- Initial Unreal Engine 5 C++ project structure.
- Third-person character, interaction, vehicle, wanted, police and mission foundations.
- Input, packaging, Git LFS, roadmap, design docs and GitHub Actions sanity workflow.
