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
- [x] Day/night + NPC schedules
- [ ] Basic radio framework

## 0.3 — Rural Crime & Jobs
- [x] Fishing + fish inventory/sales
- [x] Separate ranger/game-warden response
- [x] First forest / poaching gameplay
- [x] Staged farm cargo job with pickup checkpoint
- [x] Timed delivery and cargo-integrity reward scaling
- [ ] More mission archetypes

## 0.4 — Village Nights
- [ ] Village parties and events
- [ ] Tavern/community-hall interiors
- [ ] NPC social encounters
- [ ] Music/radio expansion
- [ ] Comedy random events

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
- [ ] Mud/off-road handling
- [ ] Dedicated Chaos drivetrain/suspension

## 0.6 — Roads, Rangers & Vehicle Life
- [x] Traffic route network
- [x] Civilian road-driving AI prototype
- [x] Game-warden/ranger AI
- [x] Traffic obstacle avoidance / horn / stuck recovery
- [x] Police pursuit-vehicle escalation at high wanted
- [ ] Police roadblocks / interception tactics
- [x] Garage vehicle recall
- [ ] Explicit garage slot-selection UI
- [ ] More vehicle ownership costs and fines

## 0.7 — Jobs & Countryside Expansion
- [x] First east-side forest expansion
- [ ] Larger connected road network and countryside
- [x] Cargo farm job
- [ ] Plowing/mowing field-work jobs
- [ ] Forest work and legal timber transport
- [x] Illegal poaching loop feeding the ranger system
- [ ] Deeper poaching inventory / fence economy
- [ ] More shops/services
- [ ] Side-mission chain

## 0.8 — Vehicle Damage & Tuning
- [x] Detachable doors/fenders/body panels
- [x] Visual damage smoke and engine-failure states
- [x] First detachable wheel damage stage
- [x] Tire-specific grip loss / collision degradation
- [x] Upgrade/tuning garage foundation
- [x] Persistent engine/tire upgrade levels
- [ ] Tractor visual upgrades
- [ ] Old-car visual/performance upgrade variants
- [ ] Replacement body-panel economy

## 0.9 — Content & Polish
- [ ] Story mission chain
- [ ] More NPC archetypes
- [x] First police escalation variety
- [ ] Performance passes
- [ ] Accessibility/settings
- [ ] Controller support
- [ ] Packaging/release automation
- [ ] Full Win64 CI/build runner

## 1.0 — First Complete Release
A stable Windows build with a complete core story loop, countryside sandbox, vehicles, wanted + ranger systems, side activities, saving, settings, audio and optimized packaged build.
