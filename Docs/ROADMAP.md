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
- [ ] Fines and arrest cash penalties
- [ ] Player garage / vehicle ownership
- [ ] Save/load
- [ ] Day/night cycle
- [ ] NPC schedules tied to time of day
- [ ] Basic radio framework

## 0.3 — Rural Crime & Jobs

- [x] Fishing at the prototype lake
- [x] Illegal fishing zone adding wanted heat
- [x] Fish inventory by count and weight
- [x] Fish sale loop for cash
- [ ] Ranger / game-warden response distinct from police
- [ ] Forest / poaching gameplay
- [ ] Farm work and legal side jobs
- [ ] More mission archetypes

## 0.4 — Village Nights

- [ ] Village parties and events
- [ ] Tavern/community-hall interiors
- [ ] NPC social encounters
- [ ] Music/radio expansion
- [ ] Comedy random events

## 0.5 — Vehicle Chaos

- [ ] Multiple tractor classes
- [ ] Old cars/vans
- [x] Fuel system
- [x] Condition affects available power
- [ ] Breakable body parts
- [ ] Degradation smoke and random mechanical faults
- [x] Basic paid repair/refuel service
- [ ] Full repair/tuning system
- [ ] Mud and off-road handling

## 0.6–0.9 — Content & Polish

- [ ] Larger connected map
- [ ] Story mission chain
- [ ] More NPC archetypes
- [ ] Police escalation variety
- [ ] Performance passes
- [ ] Accessibility/settings
- [ ] Controller support
- [ ] Packaging/release automation

## 1.0 — First Complete Release

A stable Windows build with a complete core story loop, countryside sandbox, vehicles, wanted system, police, side activities, saving, settings, audio and optimized packaged build.
