# Changelog

All notable development steps for GTT are tracked here.

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
- Three prototype catches with different weight ranges: River Perch, Village Carp and Old Pike.
- Wanted heat for illegal fishing attempts.
- Wandering civilian prototype NPCs built from Unreal primitive meshes.
- Crime witness system using distance and line-of-sight checks.
- Extra wanted heat when villagers actually witness a vehicle theft.
- Eight runtime-spawned villagers positioned around the prototype village.
- HUD economy row showing cash, fish count and total catch weight.
- HUD vehicle row showing fuel percent and litres.
- Runtime activity messages for catches, sales, witness reports and services.

### Improved
- The first mission now pays $300, creating a bridge from story gameplay into free-roam economy.
- Vehicle theft outcome depends on nearby witnesses instead of always being identical.
- The prototype village now contains a repeatable free-roam loop: fish -> risk police -> sell catch -> service vehicle -> keep exploring.
- Economy and wanted components remain discoverable while the player possesses a vehicle.
- Repository sanity checks now validate economy, fuel, fishing, witnesses and service gameplay hooks.

### Next
- Add save/load for money, fish, player position and owned vehicles.
- Add a player garage and vehicle ownership state.
- Add fines/arrest flow and cash penalties.
- Add day/night progression and simple NPC schedules.
- Add a legal farm job to balance criminal activities.
- Continue toward dedicated Chaos wheeled tractor suspension/drivetrain.

## [0.0.3] - 2026-09-12

### Added
- Dedicated prototype tractor class: `Rusty Fieldmaster 60`.
- Visible tractor body assembled entirely from built-in Unreal primitive meshes.
- Tractor-specific mass, condition, steering, acceleration and theft heat tuning.
- Mission safe-zone actor that continuously checks whether the stolen tractor has reached home.
- `Borrowed Tractor` completion logic requiring the player to lose wanted level before delivery.
- Automatic mission completion and small vehicle-condition reward on successful delivery.
- Runtime-generated prototype countryside so the project no longer depends on a hand-authored `.umap` for the first playable slice.
- Greybox player farm, neighbour farm, barn goal, village shop, police station, community hall, workshop, lake placeholder, fences and road loop.
- Runtime lighting for the prototype area.
- Runtime spawn of the mission tractor and barn return zone.
- Visible placeholder player body using Unreal built-in geometry.
- HUD controls legend and mission-complete state.
- Default boot map configuration using Unreal's Entry map plus runtime world generation.
- Windows packaging helper: `Scripts/package_windows.ps1`.
- Expanded repository sanity checks for the complete first mission loop.

### Improved
- First mission now forms a coherent gameplay loop: walk to neighbour farm -> steal tractor -> gain wanted -> escape police -> lose heat -> return tractor to barn goal.
- Prototype can be assembled almost entirely from source code without committing third-party art assets.
- Packaging settings now explicitly include the bootstrap map and `.pak` output.

## [0.0.2] - 2026-09-12

### Added
- Self-contained physics driving fallback for prototype vehicles.
- Vehicle engine state, speed readout and impact-based condition damage.
- Vehicle theft crimes that immediately add wanted heat.
- Driver enter/exit and vehicle-stolen gameplay delegates.
- Wanted lookup helper that keeps police/HUD aware of the player's heat while driving.
- Native HUD with wanted stars, vehicle condition, speed, stolen status and mission objective.
- Police AI controller that actively pursues the currently controlled player pawn.
- Spawnable default police pawn with wanted-scaled chase speed.
- Police response fallback spawning when no map spawn points have been authored yet.
- Automatic police unit cleanup as wanted level falls.
- Automatic Police Director creation from the game mode.
- `BorrowedTractor` mission auto-start and first theft stage transition.

### Improved
- Vehicle base is now directly physics-driven instead of depending entirely on Blueprint input events.
- Police wanted resolution now works while the player possesses a vehicle.
- Prototype systems need less manual level setup before they can be tested.

## [0.0.1] - 2026-09-12

### Added
- Initial Unreal Engine 5 C++ project structure.
- Third-person character movement and camera foundation.
- Generic Blueprint-friendly interaction interface.
- Enter/exit vehicle framework.
- Vehicle health/condition, damage and repair foundation.
- Wanted heat system with 0–5 levels and delayed decay.
- Police response director with configurable spawn points and response strength.
- Lightweight mission runtime component.
- Initial input, engine and packaging configuration.
- Git LFS patterns for Unreal/binary assets.
- Project roadmap and game-design foundation.
- Repository sanity-check script and GitHub Actions workflow.
