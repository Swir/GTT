# GTT Prototype Playtest

This document describes the current source-driven prototype loop for **GTT 0.0.12**.

## Requirements
- Unreal Engine 5.8
- Visual Studio 2022 with **Game development with C++**
- Windows 10/11 x64

## Launch
1. Clone the repository.
2. Generate Visual Studio project files for `GTT.uproject`.
3. Build the `GTTEditor` target.
4. Open `GTT.uproject` and press **Play**.

The project boots from Unreal's built-in Entry map and creates the greybox countryside at runtime.

## Controls
- `WASD` — walk / drive
- Mouse — camera
- `Space` — jump
- `E` — interact / enter / jobs / services
- `F` — exit vehicle
- `R` — cycle radio
- `F5` — quick-save
- `F9` — quick-load

## 0.0.12 feed-cargo regression
1. Start **FEED CARGO CONTRACT** at Player Farm.
2. Drive any working vehicle to Feed Depot, park beside the pickup and exit.
3. Interact with **LOAD FEED PALLETS**. The nearby parked vehicle must satisfy the vehicle requirement.
4. Drive to Hill Farm, park inside the delivery yard, exit and interact with **DELIVER FEED CARGO**.
5. Verify timer, cargo integrity, fast bonus and payout still work.

This specifically regression-tests the old possession-flow problem where interacting on foot could not see the parked cargo vehicle.

## 0.0.12 legal timber haul
1. Clear police wanted and ranger alert.
2. Travel to **NORTH WOOD YARD** and take **TIMBER CONTRACT**.
3. Park a working vehicle beside **LOAD LOGS** and interact.
4. Drive the loaded vehicle across the map to the workshop.
5. Damage the vehicle and/or tires during one run and verify timber integrity/payout decreases.
6. Park beside **TIMBER UNLOAD** and interact.
7. Repeat quickly with a healthy vehicle and verify the fast-delivery bonus can be earned.

## 0.0.12 tractor field mowing
1. Bring the Rusty Fieldmaster tractor near the Hill Farm field office.
2. Interact with **MOWING CONTRACT**. Starting without a nearby tractor must be rejected.
3. Enter the tractor and drive through **FIELD GATE 1** through **FIELD GATE 5** in order.
4. Check that each overlap advances the HUD objective automatically without leaving the tractor.
5. Driving through a later gate out of order must not advance progress.
6. Finish all five passes before the timer expires and verify the base reward plus optional efficient-route bonus.

## 0.0.12 Night Shift Favor side mission
1. Visit **THE BENT AXLE TAVERN** between 18:30 and 02:30 with zero wanted level.
2. Interact with **NIGHT SHIFT FAVOR**.
3. Follow the HUD objective to the workshop and collect **EMERGENCY PARTS**.
4. Travel to **STRANDED NEIGHBOR / EAST ROAD** and interact with **HELP NEIGHBOR**.
5. Return to The Bent Axle and interact again.
6. Verify the side mission pays `$450` and saves resulting progress/economy.
7. Try starting outside nightlife hours and while wanted; both starts must be rejected.

## 0.0.11 systems to regression-test
- Four-station fictional radio and `R` cycling.
- Nightlife crowd and random village encounters.
- Police pursuit cars at wanted 3+.
- Roadblocks and spike-strip tire damage at wanted 4–5.

## Existing systems to regression-test
- Borrowed Tractor mission and tractor ownership.
- Multi-vehicle garage, save/load and persistent tuning.
- Police arrest/fines and separate ranger/game-warden response.
- Fishing, poaching, fish buyer and legal feed cargo.
- Day/night, citizen schedules and village traffic.
- Vehicle body damage, breakable parts, engine heat/stalls and tire degradation.
- Workshop repair/refuel and tuning.

## Current limitations
- Player, traffic and pursuit vehicles still use the source-only physics fallback; dedicated Chaos wheel/suspension drivetrain tuning is not yet implemented.
- Rural cargo is gameplay state rather than visible strapped log/pallet assets.
- Mowing gates model route completion; visible cut-grass deformation is not yet implemented.
- Night Shift Favor is session mission-state in 0.0.12; its resulting cash is saved, but the side-mission stage itself is not yet in SaveGame.
- Police roadblocks are not yet positioned by an authored road-node interception planner.
- Vehicles/world remain primitive-mesh prototypes rather than final art.
- Repository CI is structural sanity checking, not a full Unreal Win64 compile/package smoke test.

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
