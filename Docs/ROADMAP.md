# GTT Roadmap

The roadmap is ordered around playable slices. Every phase should leave something testable.

<!-- SWIR-ROADMAP-STANDARD:v1 -->
<!-- ROADMAP-PROGRESS:START -->
<p align="center">
  <a href="https://github.com/Swir/GTT/actions/workflows/project-sanity.yml"><img alt="CI" src="https://github.com/Swir/GTT/actions/workflows/project-sanity.yml/badge.svg"></a>
  <img alt="Roadmap progress" src="https://img.shields.io/badge/ROADMAP-96.2%25-2ea043?style=for-the-badge">
  <img alt="Completed" src="https://img.shields.io/badge/DONE-125%2F130-1f6feb?style=for-the-badge">
  <img alt="Status" src="https://img.shields.io/badge/STATUS-IN%20PROGRESS-7c3aed?style=for-the-badge">
</p>

## 📊 Overall progress

<p align="center">
  <img width="100%" src="../assets/readme/progress-mini.svg" alt="GTT roadmap progress — 125 of 130 tasks complete, 96.2 percent" />
</p>

| ✅ Completed | ⏳ Remaining | 📦 Total | 🎯 Progress |
|---:|---:|---:|---:|
| **125** | **5** | **130** | **96.2%** |

> **Progress rule:** count only real roadmap tasks: `[x] / ([x] + [ ])`. When scope or task state changes, update the badges, numbers, percentage and deterministic `progress-mini.svg` from the same checklist truth. Never estimate progress from version numbers or commit count, and do not restore a text/Unicode progress meter.
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
- [x] Original/royalty-cleared music and radio audio assets

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

> **0.1.18 runtime evidence closure:** the Win64 technical gate now consumes the deterministic drivetrain manifest and a separate authored-trailer runtime manifest. These source-side acceptance contracts intentionally do **not** close either Native Chaos or trailer asset checkbox until a real UE 5.8 Win64 package produces matching PASS evidence and the rendered result is visually accepted.

> **0.1.34 roadside recovery choice:** native road vehicles can choose a paid temporary emergency patch or tow when recovery is recommended. The patch restores only limp-home floors, preserves body damage and persistent identity, while active Farm Cargo keeps the exact loaded vehicle, primary-save checkpoints and a running delivery timer. This source milestone does **not** close any of the five Native Chaos, trailer or Win64 acceptance blockers.

> **0.1.35 packaged recovery evidence:** the Farm Cargo packaged-evidence route now requires the production emergency patch on the exact loaded native Mulebox, verifies body/identity/timer/cargo-integrity continuity through the real cooldown, then deliberately re-breaks that same vehicle for paid tow, decoy rejection and authoritative final delivery. The evidence manifest and demo technical gate are strengthened, but the roadmap remains 125/130 until a real UE 5.8 Win64 package supplies the required runtime and visual proof.

> **0.1.36 roadside dispatch authority:** voluntary patch/tow now locks its request-time quote and exact `PersistentVehicleId`, supports explicit no-charge cancellation before arrival and exposes pending mode/quote/ETA/target to UI. Police impound remains a separate non-cancellable consequence. This source milestone does not close runtime acceptance blockers.

> **0.1.37 dispatch HUD contract:** native vehicle and Farm Cargo HUD presentation reads the authoritative locked quote, live ETA and target vehicle from the roadside subsystem after service acceptance instead of recalculating mutable estimates. This UX wiring does not change the five runtime/art blockers.

> **0.1.38 packaged dispatch evidence:** a later deterministic packaged route exercises locked patch/tow quotes, live ETA, no-charge cancellation, explicit re-request, exact charge, exact cargo-vehicle continuity, decoy rejection, final payout/reputation and primary save. `FARM_CARGO_DISPATCH_RUNTIME.json` is required by the Win64 candidate workflow, but the roadmap remains 125/130 until a real UE 5.8 Win64 package produces the evidence and the visual/runtime gates pass.

> **0.1.39 dispatch SaveGame persistence:** voluntary in-flight PATCH/TOW service now has a transactional sidecar that preserves exact target ID, locked quote and remaining ETA through save/load while keeping economy authority in the production completion path. It fails closed on stale/replayed charge evidence, Wanted/conflicting state or cargo-ID mismatch. This source work does not close any runtime/art blocker.

> **0.1.40 packaged dispatch-persistence evidence:** the Win64 candidate contract now requires a later deterministic route that reads the real sidecar, performs primary SaveGame roundtrips, proves restored tow and patch quote/ETA/exact-ID continuity, proves no-charge Wanted rejection and restored patch single-charge behavior, then finishes the same Farm Cargo contract through wrong-vehicle rejection, Hill Farm and North Wood Yard. `FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME.json` and demo technical gate schema 12 remain future runtime evidence until the exact candidate executes on a qualifying UE 5.8 Win64 runner.

> **0.1.41 garage/workshop recovery integration:** ordinary roadside tow remains damage-preserving and drops the exact native road vehicle at the workshop. Garage bays now treat authoritative `TOW`/`IMMOBILE` fleet states as a hard WORKSHOP HOLD, rejecting cheap recall before movement or payment; the workshop clears that hold only by applying the existing paid native repair/refuel service and saving the resulting vehicle state. `LIMP`/ordinary `SERVICE` remain advisory. This connected gameplay milestone does not close any Native Chaos, trailer or Win64 acceptance blocker.

> **0.1.42 packaged workshop recovery evidence:** the future Win64 candidate now has a deterministic Farm Cargo route that must prove ordinary tow → hard WORKSHOP HOLD → blocked garage recall → paid workshop service → hold release while preserving exact cargo identity/economy continuity. `FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json` promotes the demo technical gate to schema 13 only after packaged PASS evidence; source CI alone still closes no runtime/art checkbox.

> **0.1.43 workshop hours and after-hours recovery:** the existing day/night clock now controls regular workshop availability from 06:30 through 20:00. Ordinary repair/refuel waits while closed, but a genuine `TOW`/`IMMOBILE` WORKSHOP HOLD retains an emergency recovery lane at a deterministic +35% surcharge using the same authoritative repair, economy and save path. Garage office status surfaces the schedule and emergency rule. This connected economy milestone leaves the five Native Chaos, trailer and Win64 acceptance blockers unchanged.

> **0.1.44 packaged workshop-hours evidence:** the future Win64 candidate now has a later deterministic window that must prove the real 06:30–20:00 boundaries, no-charge/no-mutation ordinary service rejection after closing, production tow-created WORKSHOP HOLD, exact +35% emergency checkout math, one exact emergency debit, hold release and stable `PersistentVehicleId`. `WORKSHOP_HOURS_RUNTIME.json` can promote the existing schema-13 Farm Cargo workshop-recovery technical gate to schema 14 only after same-SHA packaged PASS evidence. Source CI still closes no Native Chaos, trailer or Win64/runtime/visual checkbox.

> **0.1.45 persistent workshop repair queue:** after-hours garage interaction can reserve one nearby owned damaged/mobile native road vehicle for the next 06:30 opening. The reservation pins the exact `PersistentVehicleId`, request-time locked quote and ready time in `GTT_WorkshopQueue_01` without pre-charging; execution at/after opening requires that same owned vehicle at a real workshop, uses the existing repair/economy/save path, keeps Farm Cargo exact-ID authority and leaves hard TOW/IMMOBILE WORKSHOP HOLD on the separate +35% emergency lane. This connected gameplay/save milestone does not close any Native Chaos, trailer or Win64/runtime/visual checkbox.

> **0.1.46 packaged workshop-queue evidence:** the future Win64 candidate now has a deterministic route for after-hours queue booking with exact persistent ID, request-time locked quote and zero pre-charge; it loads the real queue sidecar from disk, proves a substitute owned vehicle cannot consume the reservation at opening, then completes service only for the exact queued vehicle with one locked-quote debit, sidecar cleanup and preserved Farm Cargo authority. `WORKSHOP_QUEUE_RUNTIME.json` can promote the schema-14 technical gate to schema 15 only after same-SHA packaged PASS evidence. Source CI still closes no Native Chaos, trailer or Win64/runtime/visual checkbox.

> **0.1.47 multi-vehicle workshop appointments:** the after-hours workshop desk now supports up to four independent exact-vehicle reservations instead of one global slot. Each appointment preserves its own request-time locked quote and `PersistentVehicleId`, receives a deterministic 45-minute service-capacity slot, persists through the additive `GTT_WorkshopQueue_01` appointment list and can be cancelled independently without charge. An underfunded ready vehicle remains queued but does not block later due affordable appointments; successful service charges only that exact locked quote once, while TOW/IMMOBILE WORKSHOP HOLD stays on the separate emergency lane. This connected fleet/economy/save milestone leaves the five Native Chaos, trailer and Win64 runtime/visual blockers unchanged.

> **0.1.48 packaged multi-vehicle capacity evidence:** the future Win64 candidate now has a deterministic two-appointment route that persists the exact queue to disk, proves 45-minute slot spacing, exact-ID cancellation/rebooking, non-blocking underfunded checkout, one locked-quote debit for the later affordable vehicle and Farm Cargo authority continuity. The demo technical gate can advance only on real same-SHA packaged PASS evidence; source CI still closes no runtime/art checkbox.

> **0.1.49 timed workshop service lifecycle:** a due appointment no longer repairs instantly. The exact booked vehicle must physically check in at the workshop and progress through `READY → IN_SERVICE → AWAITING_PAYMENT → checkout`; deterministic work takes 30–90 in-world minutes from the vehicle workload, leaving the bay pauses safely without charge, and direct walk-up service cannot bypass the queue's locked quote or persisted timer. Hard TOW/IMMOBILE WORKSHOP HOLD remains the higher-priority emergency lane. This connected gameplay/save/economy milestone leaves the five runtime/art blockers unchanged.

> **0.1.50 workshop job board and safe appointment control:** a physical board beside the garage now exposes all authoritative workshop appointments with exact vehicle identity, locked quote, lifecycle state and ETA. Waiting/ready jobs gain a six-second, two-interaction exact-ID cancellation guard; checked-in and payment states remain queue-owned, and the board never charges cash or mutates repairs. The 48-case playtest/source contract validates presentation authority and cancellation safety without claiming Win64 runtime evidence. The roadmap remains 125/130 and all five Native Chaos, trailer and Win64 runtime/visual blockers stay open.

> **0.1.51 workshop priority, pickup and fleet return:** waiting STANDARD appointments can now be deliberately promoted before check-in through a separate exact-ID two-step priority desk. URGENT locks a deterministic +20% checkout quote, advances ahead of waiting STANDARD work where possible and shortens the existing workload-derived service timer to x0.80 without pre-charge. Successful timed checkout persists the exact paid/repaired vehicle as `READY_FOR_PICKUP`; the physical job board releases that exact vehicle back to the fleet, while garage dispatch shows PICKUP HOLD until collection. Additive SaveGame fields preserve priority/payment/pickup state, hard TOW/IMMOBILE WORKSHOP HOLD remains separate, and historical packaged workshop evidence auto-releases only through an evidence-only bridge using the production pickup API. This source milestone leaves all five Native Chaos, trailer and Win64 runtime/visual blockers unchanged.

> **0.1.52 packaged workshop priority/pickup evidence:** the future exact Win64 candidate now has a deterministic later window that must book STANDARD after hours, promote the same exact `PersistentVehicleId` to URGENT, prove a persisted +20% locked quote with no pre-charge, prove x0.80 timed service, one exact locked-quote debit, a disk-backed `READY_FOR_PICKUP` checkpoint, wrong-ID pickup rejection, exact-ID fleet release with no second charge and unchanged Farm Cargo authority. `WORKSHOP_PRIORITY_PICKUP_RUNTIME.json` can promote an already-PASS schema-16 workshop-capacity technical gate to schema 17 only for the same candidate SHA. Source CI verifies the contract wiring but does not close any of the five Native Chaos, trailer or Win64 runtime/visual blockers.

> **0.1.53 dynamic traffic incidents and roadside assistance:** meaningful real condition loss on an ambient traffic car now promotes into the existing crash-response state, nearby traffic receives a bounded non-recursive slowdown, and a disabled civilian car offers a 6-second/500 cm legal roadside-assistance interaction. Completion applies a 45% field repair, leaves a temporary limp recovery state and awards a severity-scaled $65–$110 exactly once for that disabled incident; leaving the scene cancels without charge. The 48-case playtest and dedicated source-contract workflow strengthen connected village/traffic/economy coverage, but no roadmap checkbox closes because UE 5.8 Win64 runtime, Native Chaos and authored trailer evidence remain unchanged.

> **0.1.54 civilian incident dispatch & recovery continuity:** severe disabled civilian traffic now opens one bounded authoritative dispatch with a world-space ROADSIDЕ SOS marker, persisted recovery continuity, exact tracked-vehicle rebinding after load and fail-closed expiry. Ranger/game-warden traffic control remains higher priority. This is source-contract gameplay work only and does not close any Native Chaos, trailer or Win64 runtime/visual gate.

> **0.1.55 civilian incident responder handoff:** unresolved severe dispatches now keep a player-first grace period before a physical county ROAD SERVICE responder is sent. On arrival the responder takes scene authority, performs the existing no-payout recovery, persists through save/load, clears stale authority defensively and yields to warden traffic control. This milestone does not change the five runtime/art blockers or demo readiness.

> **0.1.56 responder safety corridor:** an on-scene ROAD SERVICE responder now deploys four visible engine-local safety cones and creates a bounded 1,800 cm traffic-yield corridor with a 320 cm recovery pocket and per-car 4.5 second cooldown. Cooldown history is scoped to the physical responder scene and is reset on replacement/teardown; disabled vehicles, player roadside assistance, responder scene authority and ranger/warden traffic remain excluded. The 52-case source/playtest contract adds no economy, repair, Wanted or release authority, so roadmap completion remains 125/130 until real UE 5.8 Win64/runtime/visual blockers close.

> **0.1.57 scene clearance + physical trailer dynamics:** county ROAD SERVICE now keeps a deterministic post-recovery lane-reopening phase instead of disappearing immediately, while the physical farm trailer gains bounded vertical wheel suspension with load-sensitive spring/damping and a cargo/integrity/stress-sensitive breakable hitch that routes physical failure through the existing authoritative detach path. Source CI and playtest contracts verify the wiring, but authored skeletal trailer wheels/final hitch sockets and real UE 5.8 Win64 packaged/runtime/visual acceptance remain open. Roadmap truth stays 125/130 (96.2%).

> **0.1.58 trailer roadside recovery + suspension:** the physical farm trailer now exposes a timed in-world field-repair interaction with a request-time locked quote, 7–20 second service window and fail-safe cancellation on movement, distance, hitch stress or collision. Successful service debits exactly once, restores wheel/structure authority without erasing damaged cargo, and Heavy Haul applies its existing 22-second penalty only after the physical repair completes. Authored skeletal trailer wheels/final hitch sockets and real UE 5.8 Win64 packaged/runtime/visual acceptance remain open, so roadmap truth stays 125/130 (96.2%).

> **0.1.59 Heavy Haul driving quality + native tow-load coupling:** loaded Heavy Haul now rewards deliberate control rather than arrival alone: 60 qualified smooth seconds with no more than 12 seconds of rough exposure can arm a one-shot $180 handling bonus, while overspeed, excessive hitch load, severe roll/pitch and lost wheels count as rough driving. Loaded speed/attitude feed the same bounded breakable-hitch stress envelope, and the native Fieldmaster consumes the attached physical trailer's normalized tow load to reduce throttle authority by at most 30% and steering authority by at most 12%. This materially advances Native Chaos/trailer integration but does not close authored trailer assets or real UE 5.8 Win64 packaged/runtime/visual acceptance, so roadmap truth remains 125/130 (96.2%).

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

> **0.1.22 enforcement expansion:** the existing ranger/game-warden response now includes vehicle road-stop orders, continuous low-speed surrender/search acceptance, fish + rural-contraband seizure through existing authoritative inventories, and one-shot flee escalation into the shared Wanted/Police system. This is a connected gameplay expansion and does not change the five hardware/runtime acceptance blockers counted above.

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
- [x] Tractor visual upgrades
- [x] Old-car visual/performance variants
- [x] Replacement body-panel economy

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
- [x] Authored combat animations / weapon models / hit reactions
- [x] Consolidate combat/story/faction slots into primary sandbox SaveGame
- [x] Performance passes
- [x] Accessibility/settings
- [x] Controller support
- [x] Packaging/release automation
- [ ] Full Win64 CI/build runner

## 1.0 — First Complete Release
A stable Windows build with a complete core story loop, countryside sandbox, vehicles, wanted + ranger systems, combat, hostile factions, side activities, saving, settings, audio and optimized packaged build.