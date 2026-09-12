# GTT Prototype Playtest

This document describes the current source-driven prototype loop.

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

The project boots from Unreal's built-in Entry map. `AGTTPrototypeWorld` then creates the current greybox countryside at runtime, including lighting, roads, buildings, labels, the prototype tractor and the mission return zone.

## Controls

- `WASD` — walk / drive
- Mouse — camera
- `Space` — jump
- `E` — interact / enter vehicle
- `F` — exit vehicle

## Borrowed Tractor mission

1. Start at **PLAYER FARM**.
2. Follow the labels/road toward **NEIGHBOUR FARM**.
3. Find the **RUSTY FIELDMASTER 60**.
4. Aim at it and press `E` to enter it.
5. The theft creates wanted heat and police units begin pursuing the currently controlled pawn.
6. Escape long enough for wanted heat to decay back to zero.
7. Drive to **BARN - MISSION GOAL**.
8. Enter the marked goal zone while still driving the stolen tractor and with wanted level at zero.
9. The HUD should show `MISSION COMPLETE: BORROWED TRACTOR`.

## Current prototype limitations

- Vehicle movement is still the source-only physics fallback, not the final Chaos wheel/suspension setup.
- World art is intentionally greybox and uses Unreal built-in primitive meshes.
- Police are gameplay placeholders, not final vehicle patrols or tactical AI.
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
