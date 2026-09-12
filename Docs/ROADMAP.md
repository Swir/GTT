# GTT Roadmap

The roadmap is ordered around playable slices. Every phase should leave something testable.

<!-- SWIR-ROADMAP-STANDARD:v1 -->
<!-- ROADMAP-PROGRESS:START -->
<p align="center">
  <a href="https://github.com/Swir/GTT/actions/workflows/project-sanity.yml"><img alt="CI" src="https://github.com/Swir/GTT/actions/workflows/project-sanity.yml/badge.svg"></a>
  <img alt="Roadmap progress" src="https://img.shields.io/badge/ROADMAP-92.3%25-2ea043?style=for-the-badge">
  <img alt="Completed" src="https://img.shields.io/badge/DONE-120%2F130-1f6feb?style=for-the-badge">
  <img alt="Status" src="https://img.shields.io/badge/STATUS-IN%20PROGRESS-7c3aed?style=for-the-badge">
</p>

## 📊 Overall progress

```text
██████████████████░░ 92.3%
```

| ✅ Completed | ⏳ Remaining | 📦 Total | 🎯 Progress |
|---:|---:|---:|---:|
| **120** | **10** | **130** | **92.3%** |

> **Progress rule:** count only real roadmap tasks: `[x] / ([x] + [ ])`. When scope or task state changes, update the badges, numbers, percentage and the 20-segment bar in this block. Never estimate progress from version numbers or commit count.
<!-- ROADMAP-PROGRESS:END -->

## 0.1 — Tractor Theft Prototype
- [x] Unreal C++ project/module skeleton
- [x] Third-person character + interaction
- [x] Vehicle enter/exit, condition and physics driving fallback
- [x] Vehicle theft -> wanted heat
- [x] Police chase AI and HUD
- [x] Borrowed Tractor mission loop
- [x] Runtime greybox village
- [x] Windows packaging helper
- [x] Source-driven four-contact drivetrain/suspension foundation
- [ ] Dedicated native Chaos wheeled tractor movement
- [ ] Full Unreal compile + packaged Win64 smoke test

## 0.2 — Living Village
- [x] Civilian NPCs and witnesses
- [x] Traffic on village roads
- [x] Shop/workshop economy
- [x] Fuel, cash and mission rewards
- [x] Arrest fines
- [x] Multi-vehicle garage + save/load
- [x] Explicit numbered garage slot recall
- [x] Day/night + NPC schedules
- [x] Basic radio framework
- [x] Four fictional stations + track rotation
- [x] Civilian combat reactions: retaliate / chase / flee / knockout / recover
- [x] Four differentiated hostile combat archetypes: Scrapper / Runner / Bruiser / Enforcer

## 0.3 — Rural Crime & Jobs
- [x] Fishing + fish inventory/sales
- [x] Separate ranger/game-warden response
- [x] Forest / poaching gameplay
- [x] Staged farm cargo job with pickup checkpoint
- [x] Timed delivery and cargo-integrity reward scaling
- [x] Legal timber-haul contract with condition-sensitive payout
- [x] Tractor-only field-mowing route with ordered checkpoints
- [x] Timed roadside recovery contract tied to economy
- [x] Rural improvised weapon pickup loop
- [x] Assault / firearm-discharge hooks into wanted system
- [x] Persistent Rural Arsenal loadout and shotgun ammunition
- [x] Heavy articulated trailer contract with condition-sensitive payout
- [x] Repeatable hostile-territory ambush encounters tied to combat economy

## 0.4 — Village Nights
- [x] Village party window 18:30–02:30
- [x] Temporary nightlife NPC crowd
- [x] Interactive comedy random events
- [x] Bent Axle tavern exterior
- [x] Night Shift Favor side mission
- [x] Main-story nighttime contact integrated with village clock
- [x] Village fight/combat encounter framework
- [x] Bent Axle repeatable nighttime brawl activity
- [x] Tavern/community-hall interiors
- [x] NPC dialogue/social encounters
- [ ] Original/royalty-cleared music and radio audio assets

## 0.5 — Vehicle Chaos
- [x] Tractor + old car + farm van
- [x] Different handling/fuel/durability profiles
- [x] Fuel and condition power loss
- [x] Breakable body parts
- [x] Damage smoke and mechanical faults
- [x] Engine temperature / overheating
- [x] Tire integrity and grip-loss simulation
- [x] Repair/refuel service
- [x] Persistent performance tuning
- [x] Mud/off-road drag and tire wear
- [x] Physics-constraint tow line with snap/re-hook behavior
- [x] Four-contact spring/damper suspension simulation
- [x] Multi-gear drivetrain with vehicle-specific gearing and speed envelopes
- [x] Vehicle-specific off-road grip response tied into mud volumes
- [x] Live gear/contact/suspension/grip telemetry in HUD
- [x] Articulated rigid-body farm trailer with breakable physics hitch
- [x] Loaded/unloaded trailer mass and cargo stability simulation
- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup
- [ ] Authored skeletal trailer wheel assets and final hitch sockets

## 0.6 — Roads, Rangers & Vehicle Life
- [x] Civilian road-driving AI prototype
- [x] Traffic obstacle avoidance / horn / stuck recovery
- [x] Police pursuit-vehicle escalation
- [x] Police roadblocks and spike strips
- [x] Road-node-aware interception tactics
- [x] Shared countryside road graph for traffic, police and main-story route hints
- [x] Rural commuter traffic branches beyond the original village loop
- [x] Explicit garage slot selection and recall service cost
- [x] Main-story ranger escalation and clearance stage
- [x] Main-story wanted escalation reused for Arc 3 evidence escape
- [x] Road graph extended to Scrap Yard, Old Quarry and Marsh Camp hostile territories
- [x] Deeper vehicle ownership costs, insurance/impound fees and fines
- [x] Lane metadata, speed limits and authored junction priorities

## 0.7 — Jobs & Countryside Expansion
- [x] East-side forest expansion
- [x] North Wood Yard legal-work expansion
- [x] Cargo farm job
- [x] Mowing field-work job
- [x] Legal timber transport
- [x] Illegal poaching loop feeding ranger system
- [x] Physical towing/recovery job
- [x] Main Story Arc 2 uses forest, warden outpost and Hill Farm
- [x] Farm/timber/recovery routes consume shared vehicle damage, tire and terrain dynamics
- [x] West-side Red Barn / County Drop campaign expansion
- [x] Dedicated articulated-trailer heavy timber work contract
- [x] Heavy-haul reward tied to cargo, trailer and tractor condition
- [x] Three new hostile countryside territories with automatic proximity encounters
- [x] Deeper poaching inventory / fence economy
- [x] More shops/services
- [x] Larger connected countryside beyond current road graph

## 0.8 — Vehicle Damage & Tuning
- [x] Detachable doors/fenders/body panels
- [x] Visual damage smoke and engine-failure states
- [x] Detachable wheel damage stage
- [x] Tire-specific grip loss / collision degradation
- [x] Persistent engine/tire upgrade levels
- [x] Police spike strips use shared tire-durability model
- [x] Timber payout reacts to body/tire condition
- [x] Mud zones consume tire integrity under load
- [x] Engine/tire tuning feeds the shared drivetrain power/grip model
- [x] Heavy-haul payout reacts to tow-vehicle condition
- [ ] Tractor visual upgrades
- [ ] Old-car visual/performance variants
- [ ] Replacement body-panel economy

## 0.9 — Content & Polish
- [x] Main Story Arc 1: County Ledger / Backroad Deal / Final Farm Meet
- [x] Main Story Arc 2: Timber Ghosts / Warden / Forest Cache / Hill Farm evidence
- [x] Main Story Arc 3: Red Barn Reckoning / combat ambush / police escape / county evidence haul / fleet finale
- [x] Main Story Arc 4: North Pass Run / faction proof / contraband fence / police escape / ridge exchange
- [x] Persistent main-story stage across quit/relaunch with Arc 1 -> Arc 2 compatibility
- [x] Persistent Arc 3 campaign stage
- [x] Shared mission road-route guidance
- [x] First side-story chain
- [x] Police escalation variety: foot / pursuit cars / roadblocks
- [x] Player health + defeat/economy consequence loop
- [x] Eight-item Rural Arsenal foundation
- [x] Persist combat inventory/ammo across quit/relaunch
- [x] First campaign combat encounter with multiple hostile NPCs
- [x] Persistent rural faction notoriety and per-faction victory counters
- [x] Escalating repeatable faction encounters with larger groups and rising payouts
- [ ] Authored combat animations / weapon models / hit reactions
- [x] Consolidate combat/story/faction slots into primary sandbox SaveGame
- [x] Performance passes
- [x] Accessibility/settings
- [x] Controller support
- [x] Packaging/release automation
- [ ] Full Win64 CI/build runner

## 1.0 — First Complete Release
A stable Windows build with a complete core story loop, countryside sandbox, vehicles, wanted + ranger systems, combat, hostile factions, side activities, saving, settings, audio and optimized packaged build.
