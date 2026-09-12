# Changelog

All notable development steps for GTT are tracked here.

## [0.0.19] - 2026-09-12

### Added
- **Main Story Arc 3 — Red Barn Reckoning**, a combat-driven campaign chapter unlocked after Timber Ghosts.
- New west-side **Red Barn** compound and **County Evidence Drop** runtime locations, expanding the playable countryside beyond the previous road-graph footprint.
- Four-hostile Red Barn ambush using the existing villager combat AI, player Rural Arsenal, knockout system and live HUD hostile counter.
- Payoff-ledger evidence pickup that triggers a major real wanted response instead of a scripted fake chase; the player must clear police heat before continuing.
- County evidence handoff gated by an owned **Mulebox 1200** parked nearby in at least 35% condition, connecting story progression to vehicle ownership, repair and driving.
- Arc 3 rewards: `$900` for the county evidence delivery and `$1100` for closing the chapter with a three-vehicle farm fleet.
- Persistent Arc 3 stage save and automatic hostile respawn/refresh when loading during the Red Barn fight.
- Persistent Rural Arsenal loadout save: collected weapons, equipped weapon and shotgun shells now survive quit/relaunch.
- Dedicated `verify_arc3.py` sanity suite added to CI; combat sanity now also verifies loadout persistence.

### Changed
- HUD now displays Arc 3 objectives, hostile count, escape state and completion alongside the existing main-story objective.
- Arc 3 uses the shared road graph for route hints to the Red Barn, County Drop and final farm meeting.
- Combat status explicitly reports that the loadout is persistent.

### Limitations / Next
- Combat persistence currently uses a dedicated combat save slot; consolidation into the primary sandbox SaveGame remains desirable.
- Red Barn enemies reuse the current source-driven citizen combat body/behavior; authored faction meshes, animations and more specialized hostile archetypes remain outstanding.
- Native Chaos wheel assets/configuration, articulated trailer/hitch work and a real UE 5.8 Win64 compile/package smoke runner remain outstanding.
- Next major package: articulated farm trailers/heavy haul jobs, deeper hostile faction behaviors and countryside/road metadata expansion.

## [0.0.18] - 2026-09-12

### Added
- New reusable `UGTTCombatComponent` on the player with health, attack cooldowns, weapon inventory, weapon cycling, dropping and combat HUD state.
- **Rural Arsenal** with eight countryside weapon archetypes: Pitchfork, Wood Axe, Heavy Branch, Rake, Cattle Chain, Shovel, Workshop Wrench and Old Farm Shotgun.
- Physical interactable weapon pickups placed across Player Farm, barn/farm areas, workshop, forest, Hill Farm and neighbour property instead of granting the arsenal through a menu.
- Melee sweep combat with per-weapon reach, damage, knockback, cooldown and police-heat profiles.
- Old Farm Shotgun gameplay with limited shells, ranged spread traces, high wanted heat and vehicle/civilian damage integration.
- Player health and defeat loop: hostile villagers can damage the player; knockout returns the player to Player Farm, clears wanted, costs `$85` and removes two shotgun shells when available.
- Civilian combat AI: villagers can retaliate, chase the attacker, flee when badly hurt, receive knockback, become temporarily knocked out and later recover.
- **Bent Axle Brawl** nightlife activity: zero-wanted/nighttime entry, three hostile local brawlers, two-minute limit, live HUD objective and `$260` win purse.
- Organized brawl participants are exempt from normal melee-assault heat, while firing the shotgun still creates police heat.
- New controls: `LMB` attack, `Q` next weapon, `G` drop current weapon.
- Dedicated `verify_combat.py` sanity suite added to GitHub Actions alongside the existing project sanity suite.

### Changed
- Existing wanted/economy/vehicle-damage systems now receive combat events rather than combat living as an isolated prototype.
- HUD now exposes player HP, equipped rural weapon, inventory count or shotgun shells, plus active Bent Axle Brawl status.
- Nightlife now contains a repeatable aggressive activity instead of only social/random-event content.

### Limitations / Next
- Combat uses source-driven traces and primitive placeholder visuals; final skeletal animations, hit reactions and authored weapon meshes are still outstanding.
- Weapon inventory is session-state in 0.0.18 and is not yet persisted in the primary sandbox SaveGame.
- Next: Main Story Arc 3 using combat + towing, deeper hostile NPC archetypes, inventory/save persistence, more rural improvised weapons and authored animations.
- Native Chaos wheel setup and a real UE 5.8 Win64 compile/package smoke runner remain outstanding.

## [0.0.17] - 2026-09-12

### Added
- New reusable `UGTTVehicleDynamicsComponent` replacing the old one-force driving path with a source-driven four-contact vehicle dynamics layer.
- Four suspension traces per vehicle with spring force, damping, per-contact lateral stabilization and live ground-contact/compression telemetry.
- Multi-gear drivetrain model with speed-based automatic gear selection, gear-dependent drive force, speed-limited power falloff, reverse/braking response and rolling resistance.
- Dedicated dynamics profiles for all three player vehicles instead of only changing raw acceleration numbers:
  - **Rusty Fieldmaster 60** — 5 short gears, 58 km/h target speed, long-travel suspension, heavy chassis and 42% off-road grip recovery.
  - **Rattleback 82** — 5 road-oriented gears, 128 km/h target speed, shorter suspension and strongest lateral road grip.
  - **Mulebox 1200** — 4 utility gears, 104 km/h target speed, long wheelbase and heavier/stabler damping.
- Live `VEHICLE DYNAMICS` HUD telemetry for current gear, grounded wheel-contact count, suspension compression and effective grip.

### Changed
- Vehicle condition, overheating, engine tuning, tire upgrades and tire damage now feed power/grip multipliers into the shared drivetrain rather than only scaling the old direct force input.
- Mud volumes now reduce drivetrain grip and increase rolling resistance through the same vehicle-dynamics component while retaining physical momentum drag and tire wear.
- Fieldmaster's dedicated off-road bias makes the tractor materially more useful for Hill Farm, timber and recovery terrain instead of merely being slower/heavier.
- Base vehicle physics damping was reduced because rolling resistance and suspension damping are now modeled by the dynamics layer.
- Structural sanity checks now verify suspension traces, spring/damper forces, gearing, all three vehicle profiles, terrain integration and HUD telemetry.

### Limitations / Next
- This milestone is a substantial source-driven suspension/drivetrain foundation, **not yet a verified native ChaosWheeledVehicleMovement wheel setup**. `ChaosVehiclesPlugin` remains enabled/linked for the planned migration.
- Next: native Chaos wheel assets/configuration when a UE-equipped compile runner is available, authored hitch sockets/articulated trailers, Main Story Arc 3 and lane/speed-limit metadata.
- Real Unreal-equipped Win64 compile/package smoke-test runner remains required before claiming packaged vehicle behavior verified.

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
