# GTT Prototype Playtest

This document describes the current source-driven prototype loop for **GTT 0.0.11**.

## Requirements
- Unreal Engine 5.8
- Visual Studio 2022 with **Game development with C++**
- Windows 10/11 x64

## Launch in editor
1. Clone the repository.
2. Generate Visual Studio project files for `GTT.uproject`.
3. Build the `GTTEditor` target.
4. Open `GTT.uproject` and press **Play**.

The project boots from Unreal's built-in Entry map and creates the greybox countryside at runtime.

## Controls
- `WASD` — walk / drive
- Mouse — camera
- `Space` — jump
- `E` — interact / enter / fish / poach / garage / tuning / jobs / night events
- `F` — exit vehicle
- `R` — cycle radio station (also works while driving)
- `F5` — quick-save
- `F9` — quick-load

## Core loop
Complete **Borrowed Tractor**, earn the Rusty Fieldmaster, build a garage, take staged cargo contracts, tune vehicles, fish/poach, join absurd night events and deal with escalating police or the separate game-warden system.

## 0.0.11 radio test
1. Press `R` while on foot. The radio cycles through **Gravel FM**, **BarnBeat 96**, **Rust & Diesel**, **Night Shift**, then back to **RADIO OFF**.
2. Enter any controllable vehicle while a station is active.
3. HUD should show `RADIO | station | NOW: track` while driving.
4. Press `R` while the vehicle is possessed. The station should still cycle because the vehicle forwards radio input to the hidden driver pawn.
5. Stay on a station long enough to verify the fictional placeholder track title rotates automatically.
6. No copyrighted audio is bundled in this milestone; the framework is ready for original/royalty-cleared audio later.

## 0.0.11 village-night test
1. Let the game clock reach **18:30**. Nightlife remains active until approximately **02:30**.
2. Verify extra villagers appear around **COMMUNITY HALL** as a temporary party crowd.
3. HUD should show a magenta `VILLAGE NIGHT` row.
4. A random interactive encounter should appear after a short prototype delay at the hall/tavern/shop/forest-edge event points.
5. Possible events:
   - **Broken-down Neighbor** — help for cash.
   - **Midnight Tractor Meet** — pay an entry fee and roll for a larger prize.
   - **Suspicious Bonfire Run** — earns cash but increases WARDEN alert.
   - **Mystery Crate** — return it for a small reward and absolutely no explanation.
6. Resolve an event with `E`; the marker should disappear and another can appear later.
7. After 02:30, party crowd and unresolved random event should be removed.
8. Confirm the new **THE BENT AXLE TAVERN** greybox building is visible beside the community hall.

## 0.0.11 police-roadblock test
1. Raise wanted to 3 and confirm pursuit cars work as in 0.0.10.
2. Raise wanted to **4**. HUD should switch to `INTERCEPTION MODE` and show `ROADBLOCKS 1` after response evaluation.
3. A physical roadblock should appear ahead of the current movement direction with two barriers and a visible `POLICE ROADBLOCK / SPIKE STRIP` sign.
4. Hit the spike strip in a player vehicle and verify tire integrity drops through the same upgrade-aware tire system used by collision damage.
5. At wanted **5**, the director may maintain up to two roadblocks and up to three pursuit vehicles.
6. Clear wanted or get arrested; roadblocks should despawn as response requirements fall.
7. Tire upgrades should reduce effective spike-strip tire loss because `ApplyTireDamage` respects tire reinforcement.

## 0.0.10 systems to regression-test
- Physics-driven police pursuit cars at wanted 3+.
- Staged farm cargo contract: feed depot pickup, timer, cargo integrity, Hill Farm delivery and fast bonus.

## Existing systems to regression-test
- Borrowed Tractor mission and permanent tractor ownership.
- Multi-vehicle garage, sequential recall and SaveGame v3.
- Engine/tire tuning and tire integrity.
- Police foot pursuit, pursuit cars, arrest/fines and now roadblocks.
- Separate ranger/game-warden pursuit, citations and confiscation.
- Fishing, fish buyer and forest poaching.
- Day/night and ordinary citizen schedules.
- Six-car bidirectional ambient traffic.
- Vehicle breakable parts, smoke, overheating and engine stalls.
- Workshop repair/refuel.

## Persistence
Save v3 stores cash, fish, player transform, mission completion, day/time and every owned vehicle's ID, transform, condition, fuel, engine tune level, tire tune level and tire integrity. Radio station, active random-night event, party crowd, police roadblocks and an active cargo contract are session state in 0.0.11.

## Current prototype limitations
- Player, traffic and police vehicles still use the source-only physics fallback rather than tuned Chaos wheel/suspension movement.
- Roadblocks are spawned dynamically ahead of the player, not yet snapped to an authored road-node/intersection graph.
- Radio contains framework + fictional metadata only; no final music/audio assets are bundled.
- Night events are interaction-driven prototypes without dialogue trees/animations.
- Community hall and Bent Axle Tavern remain exterior greybox locations with no authored interiors yet.
- Garage recall remains sequential rather than a graphical slot-selection UI.
- World/vehicles remain primitive-mesh prototypes rather than final art.
- Repository CI is structural sanity checking, not a full Unreal Win64 compile.

## Build a Windows package
```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\package_windows.ps1
```

Custom engine location:
```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\package_windows.ps1 -EngineRoot "D:\Epic Games\UE_5.8"
```

Shipping:
```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\package_windows.ps1 -Configuration Shipping
```

Output is written under `Releases/` by default and is ignored by Git.
