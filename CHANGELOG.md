# Changelog

All notable development steps for GTT are tracked here.

## [0.0.10] - 2026-09-12

### Added
- Police pursuit-vehicle escalation starting at wanted level 3.
- `AGTTPolicePursuitVehicle`, a physics-driven patrol interceptor that actively steers, accelerates, brakes and attempts arrests.
- Wanted-scaled pursuit tiers and up to three simultaneous police vehicles at maximum escalation.
- HUD police-response diagnostics showing foot units, pursuit cars and vehicle escalation state.
- New staged legal farm contract driven by `AGTTFarmJobDirector`.
- Farm contract stages: accept contract -> reach feed depot -> load cargo -> timed cross-map delivery to Hill Farm.
- Cargo integrity simulation tied to vehicle damage while transporting the load.
- Reward scaling based on cargo integrity plus a fast-delivery bonus.
- New Feed Depot and Hill Farm delivery points integrated into the runtime world.
- Structural CI coverage for pursuit vehicles, wanted escalation, job stages, timer, cargo integrity and reward hooks.

### Changed
- Legal farm work is no longer a direct start/finish interaction; it now requires a vehicle, pickup checkpoint and timed delivery.
- Police response now mixes foot officers with pursuit cars instead of only increasing pedestrian police count.
- Runtime village signage and HUD now expose the new response/job states.

### Next
- Dedicated Chaos wheeled movement/suspension for player and police vehicles.
- Police roadblocks and interception tactics.
- Additional farm contracts: hay, timber, towing and field-work variants.
- Explicit garage slot-selection UI.
- Story/side-mission chain connecting farms, ranger and police systems.
- Full Unreal-equipped Win64 compile/smoke-test runner.

## [0.0.9] - 2026-09-12

### Added
- Garage recall flow with sequential owned-vehicle recall.
- Persistent SaveGame v3 tuning.
- Three engine and tire upgrade levels.
- Tire integrity, grip loss and `FLAT TIRE` state.
- Workshop tuning terminal.
- East-side forest and illegal poaching tied to the game-warden alert.

## [0.0.8] - 2026-09-12

### Added
- Shared staged vehicle-damage framework.
- Breakable vehicle parts, damage smoke, engine temperature/overheating and mechanical stalls.
- Smarter bidirectional traffic with obstacle probes, horn feedback and stuck recovery.

## [0.0.7] - 2026-09-12

### Added
- Autonomous village traffic prototype.
- Separate game-warden authority system with ranger pursuit/citations.

## [0.0.6] - 2026-09-12

### Added
- Version-2 multi-vehicle save format and four-slot garage.
- Rattleback 82 and Mulebox 1200 ownership.

## [0.0.5] - 2026-09-12

### Added
- Persistent SaveGame model, quick-save/load and autosaves.
- Player garage, police arrest, day/night, NPC schedules and first legal farm job.

## [0.0.4] - 2026-09-12

### Added
- Economy, fishing, fuel, workshop, fish buyer and civilian witnesses.

## [0.0.3] - 2026-09-12

### Added
- Rusty Fieldmaster 60 and Borrowed Tractor gameplay loop.
- Runtime-generated greybox countryside and Windows packaging helper.

## [0.0.2] - 2026-09-12

### Added
- Physics driving fallback, vehicle damage, theft crimes, wanted integration, HUD and police chase AI.

## [0.0.1] - 2026-09-12

### Added
- Initial Unreal Engine 5 C++ project structure and core gameplay foundations.
