# GTT Prototype Playtest

This document describes the current source-driven prototype loop for **GTT 0.0.9**.

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
- `E` — interact / enter / fish / poach / garage / tuning / jobs
- `F` — exit vehicle
- `F5` — quick-save
- `F9` — quick-load

## Core loop
Complete **Borrowed Tractor**, earn the Rusty Fieldmaster, build a four-vehicle garage, do legal farm work, fish/poach for risky income, repair/tune vehicles and deal with police or the separate game-warden alert.

## 0.0.9 garage recall test
1. Own at least two persistent vehicles.
2. Move them away from the garage terminal so no vehicle is inside the terminal search radius.
3. Interact with **GARAGE: REGISTER / RECALL NEXT**.
4. The next owned unoccupied vehicle should teleport into the recall bay and stop with zero linear/angular velocity.
5. Move it away and use the terminal again; the fleet cursor should advance to the next owned vehicle.
6. Occupied vehicles must refuse recall.

## 0.0.9 tuning and tire test
1. Park an owned vehicle beside **TUNING / TIRES** near the workshop.
2. Use the terminal repeatedly.
3. Engine tune advances from L0 to L3. Price rises by level; each level adds power, slightly reduces fuel burn/temperature and reduces low-condition stall chance.
4. Once engine tuning is maxed, tire tuning advances from L0 to L3 and improves grip/impact resistance.
5. Crash hard enough to reduce tire integrity. HUD should show **TIRE HEALTH** falling.
6. Low tire health reduces acceleration and steering authority. At extremely low integrity the fault row should show `FLAT TIRE`.
7. If tire integrity is below 80%, the tuning terminal prioritizes a tire service before further upgrades.
8. Save with `F5`, change/move the vehicle, then load with `F9`. Save v3 should restore engine level, tire level and tire integrity for each owned vehicle.
9. Loading an older v2 garage save should still work; missing tuning fields default to stock engine/tires and full tire integrity.

## 0.0.9 forest / poaching test
1. Travel east past the private lake to **WARDEN FOREST / NO HUNTING**.
2. Interact with **ILLEGAL FOREST POACHING**.
3. Every attempt adds wildlife heat to the separate `WARDEN` alert.
4. Successful attempts can return a forest hare, wild boar or red deer and an immediate black-market cash reward.
5. Failed attempts still create ranger risk.
6. Repeat attempts are cooldown-limited.
7. Verify the ranger can still pursue/cite independently from police wanted.

## Existing systems to regression-test
- Borrowed Tractor mission completion and $300 reward.
- Police wanted, witness reports, chase and arrest/fines.
- Separate ranger/game-warden chase and fish confiscation.
- Fishing and fish buyer.
- Farm delivery job.
- Day/night and citizen schedules.
- Six-car bidirectional traffic, obstacle probes, BEEP placeholder and stuck recovery.
- Vehicle damage smoke, breakable parts, overheating and random engine stalls.
- Workshop repair/refuel and body-part restoration.

## Persistence
Save v3 stores cash, fish, player transform, mission completion, day/time and every owned vehicle's ID, transform, condition, fuel, engine tune level, tire tune level and tire integrity. Older v2 and tractor-only saves retain migration paths.

## Current prototype limitations
- Vehicle movement still uses the source-only physics fallback rather than tuned Chaos wheel/suspension movement.
- Garage recall is sequential; there is not yet a graphical slot-selection menu.
- Forest hunting is an interaction prototype rather than a full tracking/hunting minigame.
- Poaching currently pays immediately rather than carrying a separate game inventory to a fence.
- Smoke and horn feedback are source-only placeholders.
- Vehicles and world remain primitive-mesh prototypes rather than final art.
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
