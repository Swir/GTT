# GTT Prototype Playtest

This document describes the current source-driven prototype loop for **GTT 0.0.4**.

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

The project boots from Unreal's built-in Entry map. `AGTTPrototypeWorld` creates the current greybox countryside at runtime, including lighting, roads, buildings, villagers, the tractor, fishing, service terminals and the mission return zone.

## Controls

- `WASD` — walk / drive
- Mouse — camera
- `Space` — jump
- `E` — interact / enter vehicle / fish / use shop or workshop
- `F` — exit vehicle

## Borrowed Tractor mission

1. Start at **PLAYER FARM** with $120.
2. Follow the labels/road toward **NEIGHBOUR FARM**.
3. Find the **RUSTY FIELDMASTER 60**.
4. Aim at it and press `E` to enter it.
5. The theft creates wanted heat. If a nearby villager has line of sight, they add extra heat by reporting the theft.
6. Police units begin pursuing the currently controlled pawn.
7. The tractor uses fuel while its engine is running; heavy throttle burns fuel faster.
8. Escape long enough for wanted heat to decay back to zero.
9. Drive to **BARN - MISSION GOAL**.
10. Enter the marked goal zone while still driving the stolen tractor and with wanted level at zero.
11. The HUD should show `MISSION COMPLETE: BORROWED TRACTOR` and the player receives **$300**.

## Free-roam economy loop

After or during the mission you can test the first repeatable sandbox activity loop:

1. Walk to **PRIVATE LAKE - NO FISHING**.
2. Aim at the **POACH FISH - E** marker and press `E`.
3. Illegal fishing adds wanted heat even when nothing bites.
4. Successful casts add a River Perch, Village Carp or Old Pike to the player's carried catch.
5. HUD shows fish count and total weight.
6. Reach **VILLAGE SHOP / FISH BUYER** and interact with **SELL FISH - E**.
7. The catch is sold by weight and cash is added immediately.
8. Park a damaged or low-fuel vehicle near **WORKSHOP**.
9. Exit the vehicle, aim at **REPAIR + REFUEL $75 - E** and interact.
10. If you have enough money, the nearest vehicle is fully repaired and refuelled.

## Vehicle behaviour to verify

- HUD shows condition, fuel percentage, litres and speed.
- Tractor starts with 18 L in a 55 L tank.
- Fuel decreases while the engine is running.
- Higher throttle burns fuel faster.
- Collision damage lowers condition.
- Lower condition reduces available drive power.
- Zero condition stops the vehicle through the existing breakdown path.
- Zero fuel shuts the engine down.

## NPC / witness behaviour to verify

Eight prototype villagers are spawned around the village. They wander locally. When the tractor is stolen, citizens within witness range perform a line-of-sight check. A successful witness report adds wanted heat and produces a gameplay message. A theft with no valid witness shows an unnoticed-theft message instead.

## Current prototype limitations

- Vehicle movement is still the source-only physics fallback, not the final Chaos wheel/suspension setup.
- World art is intentionally greybox and uses Unreal built-in primitive meshes.
- Police are gameplay placeholders, not final vehicle patrols or tactical AI.
- Citizens use simple local wandering rather than navmesh schedules.
- Fishing is interaction-driven; there is no rod animation or minigame yet.
- Economy is runtime-only until save/load lands.
- The bootstrap map is procedural so there is no authored village `.umap` yet.

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
