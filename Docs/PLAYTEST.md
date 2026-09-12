# GTT Prototype Playtest

This document describes the current source-driven prototype loop for **GTT 0.0.8**.

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

## Main gameplay loops

### Borrowed Tractor
1. Start at **PLAYER FARM / 4-SLOT GARAGE**.
2. Steal the **RUSTY FIELDMASTER 60** at the neighbour farm.
3. Witnesses can add police heat.
4. Lose wanted and return the tractor to **BARN - MISSION GOAL**.
5. Earn **$300** and permanent ownership of the tractor.

### Garage and vehicles
Persistent vehicles currently include:
- **Rusty Fieldmaster 60** — heavy, durable tractor.
- **Rattleback 82** — quick but fragile old compact.
- **Mulebox 1200** — heavier farm van.

Bring an unowned parked vehicle to the player garage, lose all active police/warden attention, pay the registration fee and save it into the four-slot garage roster. Save format v2 restores each owned vehicle independently by persistent ID.

### Police vs game warden
- Vehicle theft and witnessed ordinary crime feed **WANTED**.
- Restricted fishing feeds the separate **WARDEN** alert.
- Police can arrest and fine the player.
- The game warden can chase the player, issue a separate citation and confiscate carried fish.
- Both systems can be active at the same time.

### Legal work and economy
- Complete the farm delivery job for legal cash.
- Poach fish for risky income and sell fish at the village buyer.
- Use the workshop for repair/refuel.
- Use earnings to register more vehicles.

## 0.0.8 vehicle-damage test

This milestone adds visible and mechanical damage stages. The easiest test is to repeatedly crash a player-controlled vehicle into solid greybox buildings/fences at speed.

Verify the following:

1. **HUD temperature:** while driving hard, the vehicle HUD shows `TEMP` in Celsius.
2. **Condition power loss:** damaged vehicles accelerate more weakly.
3. **Smoke:** once condition falls below roughly 62%, animated grey primitive-mesh smoke puffs appear above the damaged vehicle.
4. **Breakable parts:** body pieces detach at staged condition thresholds and become physics objects.
5. **Rattleback 82:** can progressively lose bumper/doors/trunk and, at extreme damage, a rear wheel.
6. **Mulebox 1200:** can lose front bumper, sliding cargo door, rear doors and a rear wheel.
7. **Rusty Fieldmaster 60:** can lose fenders, exhaust stack and hood.
8. **Mechanical faults:** badly damaged engines can randomly stall under throttle.
9. **Restart delay:** after an engine stall, wait briefly and apply throttle to attempt an automatic restart.
10. **Overheating:** hard driving with severe damage raises engine temperature; overheating reduces power and critical temperature can force a shutdown and apply more damage.
11. **HUD fault row:** faults such as `ROUGH ENGINE`, `OVERHEATING`, `ENGINE STALL`, `ENGINE OVERHEAT` or detached-part count appear below the vehicle line.
12. **Workshop repair:** a strong/full workshop repair restores condition and reattaches staged body parts.
13. **Save/load:** saved condition reconstructs the matching damage stage after load.

## 0.0.8 traffic test

Ambient traffic now has more life:

1. Six traffic cars are spawned by default.
2. Cars are split between clockwise and counter-clockwise flows.
3. A forward line trace checks for obstacles.
4. When blocked, traffic brakes/reverses slightly and steers away.
5. A blocked car briefly displays **BEEP!** above itself as placeholder horn feedback.
6. A vehicle that remains stuck without a detected obstacle advances its route and receives a small recovery impulse.
7. Traffic cars can also accumulate visible collision damage because they inherit the shared vehicle-damage system.

## Persistence to verify

Saved state currently includes cash, fish, player transform, first mission completion, day/time and each owned vehicle's persistent ID, transform, condition and fuel. Because detached body state is derived from condition, a loaded damaged vehicle should reconstruct the correct breakable-part stage.

## Current prototype limitations

- Vehicle motion still uses the source-only physics fallback rather than tuned Chaos wheel/suspension movement.
- Smoke and horn are source-only visual placeholders; final particle/audio assets are not present yet.
- Detached wheels are currently a visual/physics damage stage; tire-specific grip and wheel-movement simulation still need dedicated work.
- Traffic uses route-point physics steering rather than a production road graph/nav system.
- Vehicles remain primitive-mesh prototypes rather than final art assets.
- The four garage bays do not yet have a slot-selection/recall UI.
- Fishing remains interaction-driven without a rod minigame.
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
