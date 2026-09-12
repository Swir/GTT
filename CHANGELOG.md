# Changelog

All notable development steps for GTT are tracked here.

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

### Next
- Add a dedicated Chaos wheeled tractor class and wheel configuration hooks.
- Add a visible placeholder tractor Blueprint/mesh setup guide.
- Add a mission safe-zone trigger to finish Borrowed Tractor.
- Create the first greybox village map and road loop.

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
