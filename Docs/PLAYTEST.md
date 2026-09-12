# GTT Prototype Playtest

This document describes the current source-driven prototype loop for **GTT 0.0.6**.

## Requirements

- Unreal Engine 5.8
- Visual Studio 2022 with **Game development with C++**
- Windows 10/11 x64

## Launch in editor

1. Clone the repository.
2. Right-click `GTT.uproject` and generate Visual Studio project files if Windows offers that option.
3. Build the `GTTEditor` target in Visual Studio.
4. Open `GTT.uproject`.
5. Press **Play**.

The project boots from Unreal's built-in Entry map. `AGTTPrototypeWorld` creates the current greybox countryside at runtime.

## Controls

- `WASD` — walk / drive
- Mouse — camera
- `Space` — jump
- `E` — interact / enter vehicle / fish / shop / garage / job terminal
- `F` — exit vehicle
- `F5` — quick-save
- `F9` — quick-load

## Borrowed Tractor mission

1. Start at **PLAYER FARM / 4-SLOT GARAGE** with $120.
2. Reach **NEIGHBOUR FARM**.
3. Steal the **RUSTY FIELDMASTER 60**.
4. Witnesses can add extra wanted heat if they have line of sight.
5. Lose the police and return the tractor to **BARN - MISSION GOAL**.
6. Completion pays **$300** and makes the tractor player-owned.
7. The tractor becomes one persistent garage vehicle.

## Multi-vehicle garage loop

GTT 0.0.6 adds three distinct persistent prototype vehicles:

- **Rusty Fieldmaster 60** — heavy tractor, 55 L tank, high durability.
- **Rattleback 82** — light old compact car, faster acceleration, fragile body, 42 L tank.
- **Mulebox 1200** — heavy farm van, slower steering, 62 L tank, more durability than the car.

To test the garage:

1. Find the **RATTLEBACK 82 - OLD CAR** near the village shop or **MULEBOX 1200 - FARM VAN** near the workshop.
2. Enter it. The first unauthorized use creates wanted heat and can be witnessed.
3. Lose wanted level.
4. Drive the vehicle back to **PLAYER FARM / 4-SLOT GARAGE**.
5. Park close to **GARAGE REGISTER / SAVE - E** and exit the vehicle.
6. Interact with the terminal. Registration costs **$250**.
7. HUD should show the garage count increasing, for example `GARAGE 2/4`.
8. Press `F5`, move/damage/refuel the vehicles, then press `F9` to verify that every owned vehicle restores independently.

The save format is now version 2 and stores an array of owned vehicles by persistent vehicle ID. Existing 0.0.5 tractor saves retain a migration path.

## Free-roam economy loop

- Poach fish at **PRIVATE LAKE - NO FISHING**.
- Sell the catch at **VILLAGE SHOP / FISH BUYER**.
- Repair/refuel at **WORKSHOP** for $75.
- Take the legal farm job and complete **FIELD DELIVERY** for $180.
- Use mission/job/fishing income to register more garage vehicles.

## Police and arrest

- Theft, witnesses and illegal fishing add wanted heat.
- Police pursue the currently controlled pawn.
- A close police catch can trigger arrest.
- Arrest clears wanted, charges a wanted-scaled fine and releases the player at the police station.
- Arrest autosaves.

## Day/night and NPCs

The runtime day/night cycle updates the clock and lighting. Citizens use a simple schedule center: work during the day, social area in the evening and home at night. The HUD displays day/time alongside garage occupancy and legal-job status.

## Persistence to verify

Saved state includes:

- cash and carried fish,
- player transform,
- first mission completion,
- day and time,
- every owned vehicle's persistent ID,
- vehicle transform,
- vehicle condition,
- vehicle fuel.

Autosaves occur after important milestones such as mission completion, arrest, garage registration and legal-job completion.

## Current prototype limitations

- Vehicle movement still uses the source-only physics fallback; final Chaos wheel/suspension tuning is not implemented yet.
- Vehicles are primitive-mesh prototypes rather than final art assets.
- The four garage bays are visual markers; slot assignment is logical, not a parking-management UI yet.
- Traffic AI is not implemented yet.
- Police and citizens are still prototype AI.
- Fishing remains interaction-driven without rod animation/minigame.
- The bootstrap world is procedural and still has no authored village `.umap`.
- Repository CI is a structural sanity check, not a full Unreal Win64 compile.

## Build a Windows package

From PowerShell at the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\package_windows.ps1
```

If Unreal Engine is installed somewhere else:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\package_windows.ps1 -EngineRoot "D:\Epic Games\UE_5.8"
```

For a Shipping build:

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\package_windows.ps1 -Configuration Shipping
```

Output is written under `Releases/` by default and is ignored by Git.
