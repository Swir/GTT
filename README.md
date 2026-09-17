<!-- SWIR-README-STANDARD:v1 -->

<div align="center">

# ⚡🚜 GRAND THEFT TRACTOR

### An original rural open-world comedy sandbox built in Unreal Engine 5.8

**Tractors & old vehicles • living countryside • jobs & crime • police + game wardens • systemic vehicle damage**

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-5.8-02050A?style=for-the-badge&logo=unrealengine&logoColor=62E5FF)
![C++](https://img.shields.io/badge/C%2B%2B-Gameplay_Core-02050A?style=for-the-badge&logo=cplusplus&logoColor=62E5FF)
![Windows](https://img.shields.io/badge/Target-Windows-02050A?style=for-the-badge&logo=windows11&logoColor=62E5FF)
![Chaos Vehicles](https://img.shields.io/badge/Physics-Chaos_Vehicles-02050A?style=for-the-badge&logoColor=62E5FF)

[![Project sanity](https://github.com/Swir/GTT/actions/workflows/project-sanity.yml/badge.svg)](https://github.com/Swir/GTT/actions/workflows/project-sanity.yml)
![Status](https://img.shields.io/badge/status-pre--alpha-0088FF?style=flat-square)
![Roadmap](https://img.shields.io/badge/roadmap-125%2F130%20%7C%2096.2%25-0088FF?style=flat-square)

</div>

<img width="100%" src="https://raw.githubusercontent.com/Swir/Swir/main/assets/power-divider-v4.svg" alt="SWIR electric divider" />

## Project status

GTT is in **pre-alpha active development**. The repository contains a large playable-systems foundation, source-level CI and Windows packaging/evidence automation, but the first public demo is intentionally gated behind real Unreal Engine 5.8 Win64 packaging, packaged-EXE runtime smoke tests and rendered visual acceptance.

**No public demo release is available yet.** Source CI passing does not mean a Windows demo EXE has been verified.

Current development milestone: **0.1.22 — Ranger road stops, roadside search and flee escalation**.

## What is GTT?

Grand Theft Tractor is an original countryside sandbox about causing trouble, taking legal work, maintaining a rough vehicle fleet and dealing with consequences in a living rural world. The project combines tractors, an old car, a farm van, traffic, NPC schedules, farming/logistics work, fishing and poaching, police pursuit, game-warden enforcement, combat, day/night systems, economy, save/load and a growing story campaign.

The tone is comedic and chaotic, but the gameplay systems are designed to connect: vehicle condition changes jobs, terrain affects grip, illegal activity feeds law response, cargo and dispatch systems affect earnings, and police/warden systems share the same authoritative wanted/economy state instead of acting as isolated demos.

## Highlights

| Feature | What it does |
|---|---|
| 🚜 Multi-vehicle sandbox | Tractor, old car and farm van roles with garage ownership, recall, fuel, condition and tuning. |
| 🛞 Chaos vehicle migration | Native Chaos drivetrain/wheel/suspension work is integrated behind explicit runtime acceptance gates. |
| 💥 Vehicle damage | Tire wear, breakable panels, overheating, mechanical faults, collision damage and recovery/service loops. |
| 🚓 Police escalation | Wanted heat, pursuit vehicles, roadblocks, spike strips, interception and arrest consequences. |
| 🌲 Game-warden enforcement | Wildlife alerts, ranger pursuit, night reinforcement, police handoff, citations, seizure and vehicle road stops. |
| 🌾 Legal rural work | Farm cargo, mowing, timber hauling, recovery and heavier trailer/logistics jobs tied to economy and vehicle condition. |
| 📦 Living logistics | ROAD/CARGO dispatch, depot stock, urgency, reservations, relationship favors, backlog and route-planning consequences. |
| 🧑‍🌾 Living village | Civilian NPCs, schedules, traffic, day/night cycle, social venues and countryside activity. |
| 🔫 Combat & factions | Rural arsenal, hostile archetypes, repeatable faction encounters and persistent campaign consequences. |
| 💾 Persistent sandbox | Save/load for core progression, fleet state, tuning, economy and campaign systems. |
| 📻 Original radio framework | Four fictional stations with track rotation and project-owned/cleared audio workflow. |
| 🎮 Input support | Keyboard/mouse plus controller mappings for movement, vehicles, interaction, combat, radio and save/load. |

## Quick start — from source

There is no verified public demo build yet, so the current reliable path is the Unreal project source.

1. Clone the repository.
2. Install **Unreal Engine 5.8**.
3. Install Visual Studio 2022 with **Game development with C++** and a compatible Windows SDK.
4. Generate project files from `GTT.uproject` if your Unreal installation requires it.
5. Build the `GTTEditor` target.
6. Open `GTT.uproject` in Unreal Editor.

```bash
git clone https://github.com/Swir/GTT.git
cd GTT
```

The repository intentionally excludes generated Unreal folders and Unreal Engine source code.

## Requirements / compatibility

| Requirement | Current project target |
|---|---|
| Engine | Unreal Engine 5.8 |
| Desktop target | Windows / Win64 |
| Compiler toolchain | Visual Studio 2022 C++ toolchain |
| Unreal plugins | Enhanced Input, Chaos Vehicles |
| Source control | Git; Git LFS is expected for large binary assets when needed |

A full Win64 Unreal compile/package/runtime pass still requires a qualifying Windows runner with Unreal Engine 5.8. Linux source-contract CI is useful for regressions but is **not** treated as packaged-game verification.

## Controls

Current default mappings from `Config/DefaultInput.ini`:

| Action | Keyboard / mouse | Controller |
|---|---|---|
| Move | `WASD` | Left stick |
| Look | Mouse | Right stick |
| Jump | `Space` | Bottom face button |
| Interact | `E` | Left face button |
| Vehicle throttle / reverse | `W` / `S` | Right / left trigger axis |
| Vehicle steering | `A` / `D` | Left stick |
| Exit vehicle | `F` | Right face button |
| Attack | Left mouse button | Right trigger |
| Next weapon | `Q` | Right shoulder |
| Drop weapon | `G` | D-pad down |
| Radio next | `R` | D-pad right |
| Quick save | `F5` | Special left |
| Quick load | `F9` | Special right |

## Gameplay architecture

GTT uses a C++ gameplay core with Blueprint-friendly APIs. Major systems are separated into focused modules/folders under `Source/GTT`, including vehicles, police, wanted state, ranger enforcement, NPCs, traffic, missions, economy, save systems, combat, UI and world simulation.

Vehicle development currently uses a guarded migration model: proven gameplay state remains available while native Chaos components gain dedicated runtime evidence and acceptance checks. This prevents a source-only configuration change from being mistaken for verified packaged physics behavior.

## Verification

The repository contains a large set of Python source-contract sanity checks under `Scripts/`, plus dedicated GitHub Actions workflows for major milestones. Release-oriented automation also records Win64 preflight/build/runtime evidence when a qualifying Unreal Windows runner is available.

Important distinction:

- **Source-contract CI:** verifies repository structure, wiring, invariants and regression contracts.
- **Unreal runtime acceptance:** must come from an actual UE 5.8 Win64 build/package run.
- **Demo acceptance:** additionally requires packaged-EXE smoke testing and visual approval of the exact candidate.

## Roadmap

The authoritative roadmap is [`Docs/ROADMAP.md`](Docs/ROADMAP.md).

Current truth: **125 / 130 tasks complete (96.2%)**. The remaining items are deliberately limited to real Native Chaos, authored-trailer and Win64 runtime/build acceptance blockers; they are not closed by source CI alone.

## Releases / downloads

**No public demo release is available yet.**

When the demo gate is genuinely satisfied, verified Windows artifacts and release notes will appear under [GitHub Releases](https://github.com/Swir/GTT/releases). Until then, the repository should be treated as active source development rather than a finished downloadable game.

## Repository structure

```text
GTT/
├── .github/workflows/       # source CI + Win64 evidence/release workflows
├── Config/                  # Unreal project/input configuration
├── Content/                 # Unreal project assets
├── Docs/                    # roadmap, playtests and design/acceptance notes
├── Scripts/                 # milestone and release sanity verifiers
├── Source/GTT/              # runtime C++ gameplay module
├── CHANGELOG.d/             # milestone changelog fragments
├── GTT.uproject
└── README.md
```

## Originality, assets and limitations

GTT is an original project. It does not use protected maps, models, logos, music, code or other proprietary assets from unrelated commercial games. Third-party assets may only be included when their license permits redistribution and their notices are preserved.

The current repository also contains prototype/source-built presentation and systems that still require final authored art, real packaged-runtime testing and visual acceptance before a public demo is appropriate.

## 🔎 Search Keywords

`original sandbox game` • `tractor game` • `rural open world game` • `Unreal Engine tractor game` • `Unreal Engine 5.8 game` • `Windows vehicle sandbox` • `Chaos Vehicles game` • `farming action sandbox` • `countryside driving game` • `police chase sandbox` • `game warden gameplay` • `vehicle damage simulation` • `rural logistics game` • `C++ Unreal game`

<img width="100%" src="https://raw.githubusercontent.com/Swir/Swir/main/assets/power-divider-v4.svg" alt="SWIR electric divider" />

<div align="center">

### `DRIVE • WORK • BREAK RULES • SURVIVE`

⭐ **If this project interests you, consider leaving a star.**

[**← SWIR profile**](https://github.com/Swir) · [**All projects →**](https://github.com/Swir?tab=repositories)

</div>
