# GTT Prototype Playtest

This document describes the current source-driven prototype loop for **GTT 0.0.10**.

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
Complete **Borrowed Tractor**, earn the Rusty Fieldmaster, build a garage, take staged legal cargo contracts, fish/poach for risky income, repair/tune vehicles and deal with police or the separate game-warden alert.

## 0.0.10 police pursuit escalation test
1. Commit a vehicle theft or witnessed crime until wanted reaches level 1–2.
2. Verify pedestrian police respond as before and HUD shows `POLICE RESPONSE`.
3. Raise wanted to level 3. HUD should show `VEHICLE ESCALATION` and at least one `PURSUIT CARS` slot after the director evaluates response.
4. A **County Patrol Interceptor** should spawn away from the player and physically accelerate/steer toward the controlled pawn.
5. At wanted 4–5 additional pursuit vehicles should be allowed, up to three at maximum response.
6. Pursuit cars should brake when close instead of endlessly full-throttling through the player.
7. If a pursuit car closes within arrest range while moving slowly enough, it can invoke the existing arrest/fine flow.
8. Clear wanted/arrest the player and verify excess pursuit cars despawn through the director.

## 0.0.10 staged farm cargo job test
1. Start at **PLAYER FARM** and interact with **FARM CARGO CONTRACT**.
2. The HUD objective should direct you to **FEED DEPOT / CARGO PICKUP**.
3. Reach the depot and interact without a vehicle: cargo loading must refuse.
4. Bring a vehicle and interact again. Cargo loads, a 165-second timer starts and the objective switches to **HILL FARM**.
5. Drive across the map to **HILL FARM / CARGO DELIVERY**.
6. Damage the vehicle significantly while carrying cargo. HUD cargo integrity should decline while the damaged vehicle remains in use.
7. Deliver with high cargo integrity for most of the base reward.
8. Deliver quickly enough to receive the `FAST BONUS`.
9. Let the timer expire in another run and verify the job fails without paying a reward.
10. Destroy cargo integrity by transporting it in a severely damaged vehicle and verify the contract fails.
11. Complete a delivery and verify progress is saved through the existing SaveGame flow.

## 0.0.9 systems to regression-test
- Sequential garage recall of owned vehicles.
- Persistent SaveGame v3 engine/tire tuning and tire integrity.
- Tuning terminal, tire repair and `FLAT TIRE` state.
- East forest poaching and separate ranger/game-warden alert.

## Existing systems to regression-test
- Borrowed Tractor mission completion and $300 reward.
- Police wanted, witnesses, pedestrian chase and arrest/fines.
- Separate ranger/game-warden chase and fish confiscation.
- Fishing and fish buyer.
- Day/night and citizen schedules.
- Six-car bidirectional traffic, obstacle probes, BEEP placeholder and stuck recovery.
- Vehicle damage smoke, breakable parts, overheating and random engine stalls.
- Workshop repair/refuel and body-part restoration.

## Persistence
Save v3 stores cash, fish, player transform, mission completion, day/time and every owned vehicle's ID, transform, condition, fuel, engine tune level, tire tune level and tire integrity. Older v2 and tractor-only saves retain migration paths. Active farm-contract timer/cargo state is intentionally session-only in 0.0.10; completing a contract saves the resulting economy state.

## Current prototype limitations
- Player, traffic and police vehicles still use the source-only physics fallback rather than tuned Chaos wheel/suspension movement.
- Pursuit cars use direct physics steering rather than a production road graph/interception planner.
- Police roadblocks and coordinated intercept tactics are not implemented yet.
- Garage recall is sequential; there is not yet a graphical slot-selection menu.
- Forest hunting remains an interaction prototype rather than a tracking/hunting minigame.
- Cargo is gameplay state rather than a visible strapped pallet asset.
- Vehicles/world remain primitive-mesh prototypes rather than final art.
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
