<!-- SWIR-README-STANDARD:v2 -->

<div align="center">

<img width="100%" src="assets/readme/hero.svg" alt="Grand Theft Tractor — original rural open-world comedy sandbox" />

<br>

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-5.8-02050A?style=for-the-badge&logo=unrealengine&logoColor=62E5FF)
![C++](https://img.shields.io/badge/C%2B%2B-Gameplay_Core-02050A?style=for-the-badge&logo=cplusplus&logoColor=62E5FF)
![Windows](https://img.shields.io/badge/Target-Windows-02050A?style=for-the-badge&logo=windows11&logoColor=62E5FF)
![Chaos Vehicles](https://img.shields.io/badge/Physics-Chaos_Vehicles-02050A?style=for-the-badge&logoColor=62E5FF)

[![Project sanity](https://github.com/Swir/GTT/actions/workflows/project-sanity.yml/badge.svg)](https://github.com/Swir/GTT/actions/workflows/project-sanity.yml)
![Status](https://img.shields.io/badge/status-pre--alpha-0088FF?style=flat-square)
![Roadmap](https://img.shields.io/badge/roadmap-125%2F130%20%7C%2096.2%25-0088FF?style=flat-square)

[**Status**](#project-status) · [**Highlights**](#highlights) · [**Quick Start**](#quick-start--from-source) · [**Roadmap**](#roadmap) · [**Releases**](#releases--downloads)

</div>

<img width="100%" src="https://raw.githubusercontent.com/Swir/Swir/main/assets/power-divider-v4.svg" alt="SWIR electric divider" />

## Project status

GTT is in **pre-alpha active development**. The repository contains a large playable-systems foundation, source-level CI and Windows packaging/evidence automation, but the first public demo is intentionally gated behind real Unreal Engine 5.8 Win64 packaging, packaged-EXE runtime smoke tests and rendered visual acceptance.

**No public demo release is available yet.** Source CI passing does not mean a Windows demo EXE has been verified.

<img width="100%" src="assets/readme/progress-card.svg" alt="GTT roadmap checklist progress — 125 of 130 tasks complete, 96.2 percent; release readiness remains not ready" />

Roadmap checklist: **125 / 130 tasks complete (96.2%)**. Release readiness: **NOT READY** — the remaining gates require real Win64/runtime/visual evidence and are not inferred from source CI.

Current development milestone: **0.1.38 — packaged roadside dispatch + Farm Cargo continuity evidence**.

## What is GTT?

Grand Theft Tractor is an original countryside sandbox about causing trouble, taking legal work, maintaining a rough vehicle fleet and dealing with consequences in a living rural world. The project combines tractors, an old car, a farm van, traffic, NPC schedules, farming/logistics work, fishing and poaching, police pursuit, game-warden enforcement, combat, day/night systems, economy, save/load and a growing story campaign.

The tone is comedic and chaotic, but the gameplay systems are designed to connect: vehicle condition changes jobs, terrain affects grip, illegal activity feeds law response, cargo and dispatch systems affect earnings, and police/warden systems share the same authoritative wanted/economy state instead of acting as isolated demos.

## Highlights

| Feature | What it does |
|---|---|
| 🚜 Multi-vehicle sandbox | Tractor, old car and farm van roles with garage ownership, recall, fuel, condition and tuning; same-model legacy instances use collision-safe persistent IDs before ownership. |
| 🛞 Chaos vehicle migration | Native Chaos drivetrain/wheel/suspension work is integrated behind explicit runtime acceptance gates. |
| 💥 Vehicle damage | Tire wear, breakable panels, overheating, mechanical faults, collision damage and recovery/service loops. Eligible native road vehicles can authorize a paid temporary patch or tow with a request-time locked quote, exact target identity and same-key cancellation before arrival; active Farm Cargo keeps exact-vehicle authority. |
| 🚓 Police escalation | Wanted heat, pursuit vehicles, roadblocks, spike strips, interception and arrest consequences. |
| 🌲 Game-warden enforcement | Wildlife alerts, ranger pursuit, night reinforcement, police handoff, citations, seizure, lane-aware road stops, physical shoulder pull-over guidance, compact COMPLY/SEARCH/FLEE HUD, patrol-scene lighting and nearby civilian reactions. |
| 🌾 Legal rural work | Farm cargo, mowing, timber hauling, recovery and heavier trailer/logistics jobs tied to economy and vehicle condition. Farm Cargo locks the actual loaded vehicle to the contract so another vehicle cannot complete its handoff. |
| 📦 Living logistics | ROAD/CARGO dispatch, depot stock, urgency, reservations, relationship favors, backlog and route-planning consequences. |
| 🧑‍🌾 Living village | Civilian NPCs, schedules, traffic, day/night cycle, social venues and countryside activity. |
| 🔫 Combat & factions | Rural arsenal, hostile archetypes, repeatable faction encounters and persistent campaign consequences. |
| 💾 Persistent sandbox | Save/load for core progression, fleet state, tuning, economy, campaign systems and active Farm Cargo route/vehicle identity recovery, including exact-ID actor rebinding and recovery checkpoints. |
| 📻 Original radio framework | Four fictional stations with track rotation and project-owned/cleared audio workflow. |
| 🎮 Input support | Keyboard/mouse plus controller mappings for movement, vehicles, interaction, combat, radio, roadside recovery and save/load. |

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

Current default mappings plus world-level recovery controls:

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
| Emergency roadside patch / cancel pending patch | `Y` | D-pad left |
| Roadside tow / cancel pending tow | `T` | D-pad up |
| Quick save | `F5` | Special left |
| Quick load | `F9` | Special right |

## Gameplay architecture

GTT uses a C++ gameplay core with Blueprint-friendly APIs. Major systems are separated into focused modules/folders under `Source/GTT`, including vehicles, police, wanted state, ranger enforcement, NPCs, traffic, missions, economy, save systems, combat, UI and world simulation.

Vehicle development currently uses a guarded migration model: proven gameplay state remains available while native Chaos components gain dedicated runtime evidence and acceptance checks. This prevents a source-only configuration change from being mistaken for verified packaged physics behavior.

Farm Cargo uses the same pattern for physical delivery authority: the job director owns contract/economy state, while `UGTTFarmCargoAuthoritySubsystem` binds the exact loaded vehicle and verifies that same physical vehicle is present and stopped at each buyer handoff. Since 0.1.30, the active route and stable persistent vehicle ID are saved in the existing primary snapshot; after load or actor recreation, authority may rebind only an actor carrying that exact ID. Presentation markers do not own payout, reputation, Wanted or save state.

Milestone 0.1.29 added an opt-in packaged-runtime exercise that drives the existing Farm Cargo path through real terminals and authorities: contract acceptance, exact-vehicle pickup binding, deliberate wrong-vehicle rejection, Hill Farm relay, North Wood Yard completion, payout/reputation observation and save verification. Milestone 0.1.30 hardened the ordinary player save path around that same loop: ReachPickup, loaded delivery and Hill Farm relay checkpoints persist without re-reserving stock, the exact cargo vehicle can recover after save/load or garage actor recreation, and a legacy writer can no longer downgrade the primary snapshot to schema v3.

Milestone 0.1.31 hardened the identity foundation used by that recovery path. `UGTTVehicleIdentitySubsystem` prevents later unowned same-model legacy vehicles from silently reusing an already-observed persistent ID, while refusing to rename IDs that are already owned/persisted. Its packaged evidence route saves a loaded contract, destroys/recreates the exact cargo Mulebox actor, reloads the real primary save, rejects a different same-model van after reload, repeats save/load at the Hill Farm relay, completes North Wood Yard and reloads the completed snapshot to prove the contract does not resurrect or pay twice.

Milestone 0.1.32 connected active Farm Cargo to native breakdown, roadside tow and police impound consequences without creating a second contract authority. The exact loaded `PersistentVehicleId` remains authoritative through recovery, the delivery clock keeps running, pre/post recovery checkpoints use the primary save, and another vehicle cannot inherit the load.

Milestone 0.1.33 adds a later packaged evidence window that must exercise that production path with the native Mulebox: real pickup, disabled tires, player-authorized paid tow, primary-save checkpointing, exact-ID continuity, non-paused cargo timer, no repair of ordinary tow damage, post-tow wrong-vehicle rejection, Hill Farm/North Wood Yard completion, payout/reputation and save. The harness is inert during normal play and restores its temporary evidence baseline after the proof.

Milestone 0.1.34 turns roadside recovery into a clearer gameplay choice. Eligible native road vehicles can pay for a temporary limp-home patch or choose a tow; severe body/structural failures remain tow-only, wanted heat blocks ordinary player service, and automatic police impound remains limited to genuinely stranded vehicles. Farm Cargo checkpoints before/after a patch, verifies the same persistent cargo vehicle ID and never pauses the delivery clock or transfers the load to a substitute vehicle.

Milestone 0.1.35 strengthens the packaged Farm Cargo evidence route so it must exercise that emergency patch inside the authoritative contract. The same native Mulebox is damaged into a patch-eligible `TowRecommended` state, pays the production patch quote, keeps its exact cargo identity and body damage, continues the route clock through service and the real recovery cooldown, then is deliberately broken down again for the existing paid tow path. A decoy vehicle must still be rejected before the exact Mulebox finishes Hill Farm and North Wood Yard through the real terminals. This remains an evidence contract until an actual packaged Win64 candidate emits the required PASS manifest.

Milestone 0.1.36 makes voluntary roadside help a request-time contract instead of a mutable delayed action. Patch and tow pin the exact `PersistentVehicleId` and accepted quote when authorized, reject a changed target before charge or vehicle mutation, expose the pending service/quote/countdown/target to Blueprint-friendly UI, allow same-key cancellation before arrival with no charge, and require explicit cancellation before switching service type. Wanted heat can invalidate voluntary dispatch, while automatic police impound remains a separate non-cancellable consequence for genuinely stranded vehicles.

Milestone 0.1.37 wires that authoritative dispatch contract into the native vehicle and Farm Cargo HUD. Once a service is accepted, presentation reads the locked quote, live ETA and pinned target ID from `UGTTRoadsideRecoverySubsystem` instead of recalculating mutable estimates; active cargo also reports whether the dispatch target still matches the exact loaded vehicle.

Milestone 0.1.38 adds a later packaged evidence route for those same production contracts. It accepts real Farm Cargo, binds the exact native Mulebox, observes a locked patch quote and decreasing ETA, cancels with no charge, repeats the same proof for tow, re-requests patch and verifies the final charge equals the locked quote, then rejects a decoy vehicle before completing Hill Farm and North Wood Yard through normal terminals. The generated runtime manifest is required by the Win64 evidence workflow but is not claimed until the packaged executable actually emits PASS evidence.

## Verification

The repository contains a large set of Python source-contract sanity checks under `Scripts/`, plus dedicated GitHub Actions workflows for major milestones. Release-oriented automation also records Win64 preflight/build/runtime evidence when a qualifying Unreal Windows runner is available.

A successful future packaged Farm Cargo candidate must produce `FARM_CARGO_RUNTIME.json` (`gtt.farm-cargo-runtime.v1`), `FARM_CARGO_RECOVERY_RUNTIME.json` (`gtt.farm-cargo-recovery-runtime.v1`), `FARM_CARGO_BREAKDOWN_RUNTIME.json` (`gtt.farm-cargo-breakdown-runtime.v2`) and `FARM_CARGO_DISPATCH_RUNTIME.json` (`gtt.farm-cargo-dispatch-runtime.v1`). These progressively prove exact-vehicle contract continuity, mid-route save/load + recreated-actor rebinding, emergency patch + exact-ID/body/timer continuity followed by deliberate re-breakdown/paid tow, and locked-quote/live-ETA/cancellation/re-request dispatch authority. **None of these manifests is claimed until the packaged Unreal executable actually runs and emits the required PASS evidence.**

The roadmap graphics are deterministic outputs of `Scripts/generate_progress_svg.py`; `--check` verifies checklist mathematics, XML, bounded fill geometry, README/Roadmap embeds, SVG-only presentation and separation between roadmap completion and release readiness.

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
├── assets/readme/           # project hero + deterministic progress SVG assets
├── Config/                  # Unreal project/input configuration
├── Content/                 # Unreal project assets
├── Docs/                    # roadmap, playtests and design/acceptance notes
├── Scripts/                 # milestone, progress and release sanity verifiers
├── Source/GTT/              # runtime C++ gameplay module
├── CHANGELOG.d/             # milestone changelog fragments
├── GTT.uproject
└── README.md
```

## Originality, assets and limitations

GTT is an original project. It does not use protected maps, models, logos, music, code or other proprietary assets from unrelated commercial games. Third-party assets may only be included when their license permits redistribution and their notices are preserved.

The current repository also contains prototype/source-built presentation and systems that still require final authored art, real packaged-runtime testing and visual acceptance before a public demo is appropriate.

## 🔎 Search Keywords

`original sandbox game` • `tractor game` • `rural open world game` • `Unreal Engine tractor game` • `Unreal Engine 5.8 game` • `Windows vehicle sandbox` • `Chaos Vehicles game` • `farming action sandbox` • `countryside driving game` • `police chase sandbox` • `game warden gameplay` • `vehicle damage simulation` • `rural logistics game` • `Farm Cargo save load` • `vehicle breakdown recovery` • `roadside emergency repair` • `C++ Unreal game`

<img width="100%" src="https://raw.githubusercontent.com/Swir/Swir/main/assets/power-divider-v4.svg" alt="SWIR electric divider" />

<div align="center">

### `DRIVE • WORK • BREAK RULES • SURVIVE`

⭐ **If this project interests you, consider leaving a star.**

[**← SWIR profile**](https://github.com/Swir) · [**All projects →**](https://github.com/Swir?tab=repositories)

</div>
