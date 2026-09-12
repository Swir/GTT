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
- [x] Explicit numbered garage slot recall
- [x] Day/night + NPC schedules
- [x] Basic radio framework
- [x] Four fictional stations + track rotation

## 0.3 — Rural Crime & Jobs
- [x] Fishing + fish inventory/sales
- [x] Separate ranger/game-warden response
- [x] Forest / poaching gameplay
- [x] Staged farm cargo job with pickup checkpoint
- [x] Timed delivery and cargo-integrity reward scaling
- [x] Legal timber-haul contract with condition-sensitive payout
- [x] Tractor-only field-mowing route with ordered checkpoints
- [x] Timed roadside recovery contract tied to economy

## 0.4 — Village Nights
- [x] Village party window 18:30–02:30
- [x] Temporary nightlife NPC crowd
- [x] Interactive comedy random events
- [x] Bent Axle tavern exterior
- [x] Night Shift Favor side mission
- [x] Main-story nighttime contact integrated with village clock
- [ ] Tavern/community-hall interiors
- [ ] NPC dialogue/social encounters
- [ ] Village fight/combat encounter framework
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
- [ ] Dedicated Chaos drivetrain/suspension
- [ ] Authored hitch sockets / articulated trailers

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
- [ ] Deeper vehicle ownership costs, insurance/impound fees and fines
- [ ] Lane metadata, speed limits and authored junction priorities

## 0.7 — Jobs & Countryside Expansion
- [x] East-side forest expansion
- [x] North Wood Yard legal-work expansion
- [x] Cargo farm job
- [x] Mowing field-work job
- [x] Legal timber transport
- [x] Illegal poaching loop feeding ranger system
- [x] Physical towing/recovery job
- [x] Main Story Arc 2 uses forest, warden outpost and Hill Farm
- [ ] Deeper poaching inventory / fence economy
- [ ] Dedicated trailer/hitch work contracts
- [ ] More shops/services
- [ ] Larger connected countryside beyond current road graph

## 0.8 — Vehicle Damage & Tuning
- [x] Detachable doors/fenders/body panels
- [x] Visual damage smoke and engine-failure states
- [x] Detachable wheel damage stage
- [x] Tire-specific grip loss / collision degradation
- [x] Persistent engine/tire upgrade levels
- [x] Police spike strips use shared tire-durability model
- [x] Timber payout reacts to body/tire condition
- [x] Mud zones consume tire integrity under load
- [ ] Tractor visual upgrades
- [ ] Old-car visual/performance variants
- [ ] Replacement body-panel economy

## 0.9 — Content & Polish
- [x] Main Story Arc 1: County Ledger / Backroad Deal / Final Farm Meet
- [x] Main Story Arc 2: Timber Ghosts / Warden / Forest Cache / Hill Farm evidence
- [x] Persistent main-story stage across quit/relaunch with v1 -> Arc 2 progression compatibility
- [x] Shared mission road-route guidance
- [x] First side-story chain
- [x] Police escalation variety: foot / pursuit cars / roadblocks
- [ ] Main Story Arc 3 and campaign finale
- [ ] More NPC archetypes
- [ ] Performance passes
- [ ] Accessibility/settings
- [ ] Controller support
- [ ] Packaging/release automation
- [ ] Full Win64 CI/build runner

## 1.0 — First Complete Release
A stable Windows build with a complete core story loop, countryside sandbox, vehicles, wanted + ranger systems, side activities, saving, settings, audio and optimized packaged build.
