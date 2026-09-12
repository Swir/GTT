# Changelog

All notable development steps for GTT are tracked here.

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
- Economy can now restore a saved state and apply clamped fines.
- Prototype world now contains garage, farm-job start and farm-job delivery locations.
- HUD controls include persistence shortcuts and active legal-job state.
- Repository sanity checks validate persistence, ownership, arrests, time-of-day and farm-job hooks.

### Next
- Add multiple owned vehicle slots and an old car/van.
- Add traffic and road-driving NPCs.
- Add ranger/game-warden response for fishing and forest crime.
- Add farm-job checkpoints and more legal work variants.
- Continue dedicated Chaos wheeled tractor drivetrain/suspension work.

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
