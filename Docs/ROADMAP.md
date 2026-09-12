# GTT Roadmap

The roadmap is deliberately ordered around **playable slices**, not feature count. Every phase should leave something that can be launched and tested.

## 0.1 — Tractor Theft Prototype

**Goal:** walk around a tiny rural test area, steal a tractor, trigger a police response and return it home after losing wanted level.

- [x] Unreal C++ project/module skeleton
- [x] Third-person character foundation
- [x] Interaction interface
- [x] Enter/exit vehicle framework
- [x] Vehicle condition/damage foundation
- [x] Prototype physics driving fallback
- [x] Vehicle theft -> wanted heat integration
- [x] Wanted heat and 0–5 wanted levels
- [x] Police response director foundation
- [x] Police AI chase behaviour
- [x] Native HUD with wanted + vehicle diagnostics
- [x] Mission runtime component foundation
- [x] Borrowed Tractor mission start + theft stage transition
- [x] Dedicated prototype tractor class and visible primitive-mesh body
- [x] Mission safe-zone / completion trigger
- [x] Runtime-generated greybox village and road loop
- [x] Runtime lighting and landmark labels
- [x] First complete **Borrowed Tractor** gameplay loop in source
- [x] Windows packaging helper script
- [ ] Dedicated Chaos wheeled tractor movement implementation
- [ ] Full Unreal compile + packaged Win64 smoke test on an Unreal-equipped runner/PC

## 0.2 — Living Village

- [x] Civilian NPC placeholders and basic wandering
- [x] Witness system for vehicle theft and crimes
- [ ] Traffic on village roads
- [x] Shop fish buyer and workshop services
- [x] Vehicle fuel consumption and refuelling
- [x] Basic cash/economy
- [x] Mission cash rewards
- [x] Fines and arrest cash penalties
- [x] Player garage / vehicle ownership
- [x] Multi-vehicle garage capacity and HUD occupancy
- [x] Save/load with owned-vehicle persistence
- [x] Save migration from the old one-tractor format
- [x] Day/night cycle
- [x] NPC schedules tied to time of day
- [ ] Basic radio framework

## 0.3 — Rural Crime & Jobs

- [x] Fishing at the prototype lake
- [x] Illegal fishing zone adding wanted heat
- [x] Fish inventory by count and weight
- [x] Fish sale loop for cash
- [ ] Ranger / game-warden response distinct from police
- [ ] Forest / poaching gameplay
- [x] First legal farm-job loop
- [ ] Farm-job checkpoints, cargo and timed variants
- [ ] More mission archetypes

## 0.4 — Village Nights

- [ ] Village parties and events
- [ ] Tavern/community-hall interiors
- [ ] NPC social encounters
- [ ] Music/radio expansion
- [ ] Comedy random events

## 0.5 — Vehicle Chaos

- [ ] Multiple tractor classes
- [x] First old car: `Rattleback 82`
- [x] First farm van: `Mulebox 1200`
- [x] Different mass/condition/fuel/handling profiles per vehicle class
- [x] Fuel system
- [x] Condition affects available power
- [ ] Breakable body parts
- [ ] Degradation smoke and random mechanical faults
- [x] Basic paid repair/refuel service
- [ ] Full repair/tuning system
- [ ] Mud and off-road handling
- [ ] Dedicated Chaos wheeled vehicle drivetrain/suspension

## 0.6 — Roads, Rangers & Vehicle Life

- [ ] Traffic route network
- [ ] Civilian road-driving AI
- [ ] Parked/traffic vehicle respawn rules
- [ ] Game-warden/ranger AI for illegal fishing and forest crime
- [ ] Police vehicle pursuit escalation
- [ ] Garage slot selection / vehicle recall
- [ ] More vehicle ownership costs and fines

## 0.7 — Jobs & Countryside Expansion

- [ ] Larger connected road network and countryside
- [ ] Plowing/mowing/cargo farm jobs
- [ ] Forest work and legal timber transport
- [ ] Illegal poaching loop
- [ ] More shops/services
- [ ] Side-mission chain

## 0.8 — Vehicle Damage & Tuning

- [ ] Detachable doors/fenders/body panels
- [ ] Visual smoke and engine-failure states
- [ ] Tires/wheel damage
- [ ] Upgrade/tuning garage
- [ ] Tractor visual upgrades
- [ ] Old-car performance upgrades

## 0.9 — Content & Polish

- [ ] Story mission chain
- [ ] More NPC archetypes
- [ ] Police escalation variety
- [ ] Performance passes
- [ ] Accessibility/settings
- [ ] Controller support
- [ ] Packaging/release automation
- [ ] Full Win64 CI/build runner

## 1.0 — First Complete Release

A stable Windows build with a complete core story loop, countryside sandbox, vehicles, wanted system, police, side activities, saving, settings, audio and optimized packaged build.
