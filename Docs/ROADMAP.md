# GTT Roadmap

The roadmap is ordered around playable slices. Every phase should leave something testable.

## 0.1 — Tractor Theft Prototype
- [x] Unreal C++ project/module skeleton
- [x] Third-person character + interaction
- [x] Vehicle enter/exit, condition and physics driving fallback
- [x] Vehicle theft -> wanted heat
- [x] Police chase AI and HUD
- [x] Borrowed Tractor mission loop
- [x] Runtime greybox village
- [x] Windows packaging helper
- [ ] Dedicated Chaos wheeled tractor movement
- [ ] Full Unreal compile + packaged Win64 smoke test

## 0.2 — Living Village
- [x] Civilian NPCs and witnesses
- [x] Traffic on village roads
- [x] Shop/workshop economy
- [x] Fuel, cash and mission rewards
- [x] Arrest fines
- [x] Multi-vehicle garage + save/load
- [x] Garage sequential vehicle recall
- [x] Explicit numbered garage slot recall
- [x] Day/night + NPC schedules
- [x] Basic radio framework
- [x] Four fictional stations + track rotation

## 0.3 — Rural Crime & Jobs
- [x] Fishing + fish inventory/sales
- [x] Separate ranger/game-warden response
- [x] First forest / poaching gameplay
- [x] Staged farm cargo job with pickup checkpoint
- [x] Timed delivery and cargo-integrity reward scaling
- [x] Legal timber-haul contract with condition-sensitive payout
- [x] Tractor-only field-mowing route with ordered checkpoints
- [x] Multiple legal mission archetypes
- [x] Timed roadside recovery contract tied to economy

## 0.4 — Village Nights
- [x] Village party window 18:30–02:30
- [x] Temporary nightlife NPC crowd
- [x] Interactive comedy random events
- [x] Community hall nightlife gameplay foundation
- [x] Tavern exterior / Bent Axle location
- [x] Radio expansion foundation
- [x] First nightlife side-mission chain: Night Shift Favor
- [x] Main-story nighttime contact integrated with village clock
- [ ] Tavern/community-hall interiors
- [ ] NPC dialogue/social encounters
- [ ] Village fight/combat encounter framework
- [ ] Original/royalty-cleared music and radio audio assets

## 0.5 — Vehicle Chaos
- [x] Tractor + first old car + first farm van
- [x] Different handling/fuel/durability profiles
- [x] Fuel and condition power loss
- [x] Breakable body parts
- [x] Damage smoke and random mechanical faults
- [x] Engine temperature / overheating
- [x] Tire integrity and grip-loss simulation
- [x] Repair/refuel service
- [x] Persistent performance tuning
- [x] Mud/off-road handling foundation with physical drag and tire wear zones
- [x] Physics-constraint tow line with cable-load and snap behavior
- [ ] Dedicated Chaos drivetrain/suspension
- [ ] Authored hitch sockets / articulated trailers

## 0.6 — Roads, Rangers & Vehicle Life
- [x] Traffic route network
- [x] Civilian road-driving AI prototype
- [x] Game-warden/ranger AI
- [x] Traffic obstacle avoidance / horn / stuck recovery
- [x] Police pursuit-vehicle escalation at high wanted
- [x] Police roadblock escalation at wanted 4–5
- [x] Spike-strip tire damage prototype
- [x] Garage vehicle recall
- [x] Road-node-aware interception tactics
- [x] Explicit garage slot-selection UI
- [x] First recurring garage ownership/service cost (recall fee)
- [x] Story progression that deliberately feeds into wanted/police escape gameplay
- [ ] Deeper vehicle ownership costs, insurance/impound fees and fines
- [ ] Shared authored road graph used by traffic, police and missions

## 0.7 — Jobs & Countryside Expansion
- [x] First east-side forest expansion
- [x] North Wood Yard legal-work expansion
- [x] Cargo farm job
- [x] First mowing field-work job
- [x] Legal timber transport
- [x] Illegal poaching loop feeding the ranger system
- [x] First towing/recovery job with a physical tow constraint
- [ ] Deeper poaching inventory / fence economy
- [ ] Dedicated trailer/hitch work contracts
- [ ] More shops/services
- [x] First side-mission chain
- [x] First multi-location main-story arc through countryside services
- [ ] Larger connected road network and countryside

## 0.8 — Vehicle Damage & Tuning
- [x] Detachable doors/fenders/body panels
- [x] Visual damage smoke and engine-failure states
- [x] First detachable wheel damage stage
- [x] Tire-specific grip loss / collision degradation
- [x] Upgrade/tuning garage foundation
- [x] Persistent engine/tire upgrade levels
- [x] Police spike strips use the same tire-durability model
- [x] Timber payout reacts to body/tire condition
- [x] Mud zones consume tire integrity under load
- [ ] Tractor visual upgrades
- [ ] Old-car visual/performance upgrade variants
- [ ] Replacement body-panel economy

## 0.9 — Content & Polish
- [x] Main Story Arc 1: County Ledger / Backroad Deal / Final Farm Meet
- [x] Persistent main-story stage across quit/relaunch
- [x] First side-story chain
- [ ] Main Story Arc 2 and later campaign chapters
- [ ] More NPC archetypes
- [x] Police escalation variety: foot / pursuit cars / roadblocks
- [ ] Performance passes
- [ ] Accessibility/settings
- [ ] Controller support
- [ ] Packaging/release automation
- [ ] Full Win64 CI/build runner

## 1.0 — First Complete Release
A stable Windows build with a complete core story loop, countryside sandbox, vehicles, wanted + ranger systems, side activities, saving, settings, audio and optimized packaged build.
