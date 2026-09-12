# Changelog

All notable development steps for GTT are tracked here.

## [0.0.16] - 2026-09-12

### Added
- Shared `FGTTRoadGraph` with 20 named countryside nodes and explicit links spanning the village loop, East Road, Hill Farm, Feed Depot, workshop, private lake, game-warden outpost, forest and North Wood Yard.
- Breadth-first route generation, closest-node lookup and route labels so gameplay systems can share one road topology rather than duplicating coordinates.
- Civilian rural commuter routes from village/farm approaches toward North Wood and Hill Farm using the shared graph in addition to the original village traffic loop.
- **Main Story Arc 2 — Timber Ghosts**: farm briefing, clean meeting at the game-warden outpost, illegal forest evidence cache, ranger-alert escape/clearance stage, tractor-required Hill Farm evidence handoff and final farm closure.
- Arc 2 rewards: `$500` for the evidence haul and `$700` for completing the arc.
- Story objective routing now reports the next shared road node and remaining node count for travel stages.
- Three new persistent story interaction points at the warden outpost, forest cache and Hill Farm.

### Changed
- Police interception now loads its runtime road-node list from the same shared graph used by civilian traffic and story routing.
- Story save format advances to version 2. Existing stage `8` saves from 0.0.15 naturally become the Arc 1 complete/start-Arc-2 state rather than losing progress.
- Arc 2 deliberately uses the existing ranger/game-warden heat system instead of a parallel scripted chase, and later requires an owned usable tractor to connect campaign progress to vehicle ownership/repair.
- Structural sanity checks now verify road-graph APIs, cross-system graph consumption, every Arc 2 stage, ranger heat injection/clearance, tractor gating, new contacts and story-save v2.

### Next
- Dedicated Chaos wheeled drivetrain/suspension for tractor, old car and van.
- Main Story Arc 3 and campaign finale with heavier towing/trailer work.
- Lane metadata, speed limits and junction priority for the shared road graph.
- Garage management screen with repair/fuel/tuning/storage costs.
- Consolidate story stage into the primary sandbox SaveGame schema.
- Real Unreal-equipped Win64 compile/package smoke-test runner.

## [0.0.15] - 2026-09-12

### Added
- Persistent Main Story Arc 1: County Ledger, Backroad Deal, real police escape, two-vehicle garage finale and dedicated story save.
- Main-story HUD objective and runtime story contacts across Player Farm, North Wood, Village Shop, Bent Axle, East Road and workshop.

## [0.0.14] - 2026-09-12

### Added
- Predictive police interception across named road nodes, route-aware pursuit entry and wanted 4–5 roadblocks.
- Explicit garage slots 1–4 with stable vehicle assignment, `$15` recall fee and authority lockout.

## [0.0.13] - 2026-09-12

### Added
- Physical roadside recovery contract with Unreal physics constraint towing, cable load/snap/re-hook behavior and condition-sensitive payout.
- Hill Farm/forest mud zones with drag and tire wear.

## [0.0.12] - 2026-09-12

### Added
- Legal Timber Haul, tractor-only Field Mowing and the multi-stage Night Shift Favor side story.
- North Wood Yard, expanded Hill Farm field and ordered mowing gates.

## [0.0.11] - 2026-09-12

### Added
- Four fictional radio stations, village nightlife, random night events, police roadblocks and spike strips.

## [0.0.10] - 2026-09-12

### Added
- Police pursuit vehicles and staged feed-cargo delivery with timer, cargo integrity and fast bonus.

## [0.0.9] - 2026-09-12

### Added
- Garage recall, SaveGame v3 tuning, engine/tire upgrades, tire integrity and east-side forest poaching.

## [0.0.8] - 2026-09-12

### Added
- Shared staged vehicle damage, detachable parts, smoke, overheating/stalls and smarter traffic avoidance.

## [0.0.7] - 2026-09-12

### Added
- Autonomous village traffic and separate ranger/game-warden pursuit/citation system.

## [0.0.6] - 2026-09-12

### Added
- Multi-vehicle save/garage plus the Rattleback 82 old car and Mulebox 1200 farm van.

## [0.0.5] - 2026-09-12

### Added
- Persistent save/load, player garage, police arrest, day/night, NPC schedules and first legal farm job.

## [0.0.4] - 2026-09-12

### Added
- Economy, fishing, fuel, workshop, fish buyer and civilian witnesses.

## [0.0.3] - 2026-09-12

### Added
- Rusty Fieldmaster 60, Borrowed Tractor mission, runtime countryside and Windows packaging helper.

## [0.0.2] - 2026-09-12

### Added
- Physics driving fallback, vehicle damage, theft crime, wanted integration, HUD and police chase AI.

## [0.0.1] - 2026-09-12

### Added
- Initial Unreal Engine 5 C++ project structure and core gameplay foundations.
