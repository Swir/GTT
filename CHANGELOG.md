# Changelog

All notable development steps for GTT are tracked here.

## [0.0.12] - 2026-09-12

### Added
- New `AGTTRuralWorkDirector` for legal countryside contracts beyond the original feed-delivery job.
- **Legal Timber Haul** contract: accept at North Wood Yard, load logs with a nearby working vehicle, timed delivery to the workshop, cargo-integrity payout scaling and a fast-delivery bonus.
- **Field Mowing** contract: requires a tractor near Hill Farm, then tracks five ordered drive-through field gates and pays an efficiency bonus for a quick route.
- `AGTTFieldCheckpoint` overlap gates that detect the actual driven tractor and advance field work without requiring the player to leave the cab at every pass.
- New North Wood Yard, expanded Hill Farm field, five mowing gates and workshop timber-unload point in the runtime countryside.
- First multi-stage side mission, **Night Shift Favor**: start at The Bent Axle during 18:30–02:30, collect an emergency alternator crate at the workshop, help a stranded neighbor on East Road, then return to the tavern for payment.
- Dedicated HUD objective rows for rural work and the Night Shift Favor side mission.

### Changed
- Feed-cargo loading and unloading now accept a working vehicle parked near the terminal, fixing the old possession-flow limitation where interacting on foot could not satisfy a vehicle-required checkpoint.
- Legal-work eligibility consistently rejects contracts while police wanted or ranger alert is active.
- Timber cargo condition reacts to both vehicle body condition and tire integrity, connecting tuning/repair choices to contract income.
- Prototype world version/signage updated for the new work and story loops.
- Structural CI now verifies rural-work archetypes, tractor field checkpoints, side-mission stages, nearby-vehicle cargo handling and new world/HUD hooks.

### Next
- Dedicated Chaos wheeled drivetrain/suspension implementation for tractor, old car and van.
- Road-node-aware police interception and authored roadblock positions.
- Explicit garage slot-selection UI and per-vehicle ownership costs.
- Towing/recovery jobs with a physical hitch/trailer system.
- Larger connected countryside and additional mission chain chapters.
- Deeper social dialogue/combat encounters around village nights.
- Real Unreal-equipped Win64 compile/package smoke-test runner.

## [0.0.11] - 2026-09-12

### Added
- Player radio framework with four original fictional stations: **Gravel FM**, **BarnBeat 96**, **Rust & Diesel**, and **Night Shift**.
- Original placeholder track titles and timed track rotation without bundling copyrighted audio assets.
- `R` radio cycling both on foot and while controlling a vehicle; the station state stays on the hidden driver component during vehicle possession.
- HUD radio row showing current station and current fictional track.
- Village nightlife director active from 18:30 to 02:30.
- Temporary six-NPC party crowd around the community hall during nightlife hours.
- Random night-event system with interactive **Broken-down Neighbor**, **Midnight Tractor Meet**, **Suspicious Bonfire Run**, and **Mystery Crate** encounters.
- Event payouts/entry costs and a ranger consequence for the suspicious bonfire activity.
- New **The Bent Axle Tavern** greybox location beside the community hall.
- Police roadblock escalation starting at wanted level 4.
- Up to two simultaneous roadblocks at wanted level 5.
- Physical roadblock barriers, visible warning sign and spike-strip prototype.
- Spike-strip tire damage integrated with the existing tire-integrity/grip system.
- HUD interception diagnostics showing active roadblock count.

### Changed
- Police response now has three escalation layers: foot units, pursuit vehicles and roadblocks.
- Tire damage is exposed as a shared vehicle function so collisions and police spike strips use the same upgrade-aware tire durability model.
- Prototype world signage now advertises nightlife, radio-era content and 4+ wanted roadblocks.
- Structural CI now verifies radio controls/state, nightlife events/crowd, spike strips and roadblock escalation.

## [0.0.10] - 2026-09-12

### Added
- Police pursuit-vehicle escalation starting at wanted level 3.
- Physics-driven patrol interceptors with wanted-scaled tiers.
- HUD police-response diagnostics.
- Staged legal farm cargo contract with pickup checkpoint, timed delivery, cargo integrity and reward scaling.

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
