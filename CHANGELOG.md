# Changelog

All notable development steps for GTT are tracked here.

## [0.0.9] - 2026-09-12

### Added
- Garage recall flow: when no persistent vehicle is parked by the terminal, repeated interactions cycle through owned vehicles and teleport the next available one into the recall bay.
- Persistent vehicle tuning saved in SaveGame v3.
- Three engine upgrade levels with additional power, better fuel efficiency, lower operating temperature and improved low-condition reliability.
- Three tire upgrade levels with better steering grip and impact resistance.
- Tire integrity simulation that degrades after significant collisions and directly affects acceleration/steering.
- `FLAT TIRE` fault state and tire-health diagnostics on the HUD.
- Tuning/service terminal near the workshop with progressive engine/tire costs and tire repair.
- First east-side forest expansion using runtime greybox trees.
- Illegal forest-poaching interaction tied to the existing three-level game-warden alert.
- Three prototype poaching outcomes: forest hare, wild boar and red deer with risk-scaled black-market rewards.
- Structural CI coverage for save v3, garage recall, tuning, tire degradation and forest poaching.

### Changed
- Save format advances from v2 to v3 while keeping v2 and legacy tractor-save migration paths.
- Vehicle persistence now stores engine tune level, tire tune level and tire integrity per owned vehicle.
- Garage terminal now doubles as a fleet recall terminal instead of being registration/save only.
- HUD exposes tuning level and tire health while driving.
- Workshop district now contains a dedicated tuning terminal.

### Next
- Dedicated Chaos wheeled vehicle movement and suspension.
- Garage UI/explicit slot selection instead of sequential recall.
- Replacement body-panel economy and visual upgrades.
- Expanded forest work, legal timber jobs and a deeper poaching loop.
- Police vehicle pursuit escalation.
- Full Unreal-equipped Win64 compile/smoke-test runner.

## [0.0.8] - 2026-09-12

### Added
- Shared staged vehicle-damage framework in `AGTTVehicleBase`.
- Breakable vehicle parts that physically detach, collide and tumble after condition thresholds are crossed.
- Repair integration that reattaches staged breakable parts after a sufficiently strong/full repair.
- Source-only visible damage smoke made from animated primitive-mesh puffs.
- Engine-temperature simulation, overheating, power loss and low-condition engine stalls.
- Vehicle HUD diagnostics for engine temperature, active fault state and detached-part count.
- Rattleback 82, Mulebox 1200 and Rusty Fieldmaster staged damage profiles.
- Traffic obstacle probe, visible `BEEP!`, stuck recovery and bidirectional six-car flow.

## [0.0.7] - 2026-09-12

### Added
- Autonomous village traffic prototype.
- Separate game-warden authority system for wildlife crime.
- Three-level `WARDEN` alert, ranger pursuit, citation and fish confiscation.

## [0.0.6] - 2026-09-12

### Added
- Version-2 multi-vehicle save format and migration from the earlier tractor-only save.
- Four-slot garage and persistent Rattleback 82 / Mulebox 1200 ownership.

## [0.0.5] - 2026-09-12

### Added
- Persistent SaveGame model, quick-save/load and autosaves.
- Player garage and vehicle ownership.
- Police arrest flow, day/night cycle, NPC schedules and first legal farm job.

## [0.0.4] - 2026-09-12

### Added
- Economy, fishing inventory, fuel, workshop, fish buyer and civilian crime witnesses.

## [0.0.3] - 2026-09-12

### Added
- Rusty Fieldmaster 60 and complete Borrowed Tractor gameplay loop.
- Runtime-generated greybox countryside and Windows packaging helper.

## [0.0.2] - 2026-09-12

### Added
- Physics driving fallback, vehicle damage, theft crimes, wanted integration, HUD and police chase AI.

## [0.0.1] - 2026-09-12

### Added
- Initial Unreal Engine 5 C++ project structure and core gameplay foundations.
