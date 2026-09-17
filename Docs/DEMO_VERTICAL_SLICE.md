# GTT Demo Vertical Slice Contract

The first public GTT demo should prove a connected rural sandbox loop rather than a collection of disconnected systems. The current reference slice is the legal **Farm Cargo** route because it already crosses the fleet, economy, logistics, traffic, NPC, vehicle-condition, police/ranger, day/night and persistence layers.

## Player-facing slice

1. Use the unified contract board to inspect and prepare a suitable cargo vehicle.
2. Accept Farm Cargo only when legal-work lockouts and the depot schedule/stock allow it.
3. Follow the world-space beacon to Feed Depot and physically load the cargo vehicle.
4. Keep that **same physical vehicle** for the delivery chain; the load is bound to the vehicle that actually picked it up.
5. Drive through the living village while cargo mass, vehicle condition, traffic, NPCs, day/night and law response remain active.
6. Reach Hill Farm, bring the bound cargo vehicle into the buyer zone, stop it at no more than 3.0 km/h and perform the legal handoff. Higher logistics tiers continue the same physical load to North Wood Yard.
7. Finish the chain and receive the existing economy payout, logistics reputation/history update and save persistence.

## Authority rules

The route beacon is presentation only. It must never own cash, stock, reputation, Wanted heat, wildlife alert, save data or contract state. Those remain in the existing contract board, farm director, logistics subsystem, economy component, Wanted/ranger systems and game-mode save path.

`UGTTFarmCargoAuthoritySubsystem` has one narrow responsibility: bind the physical vehicle that successfully loaded the contract at Feed Depot and verify that exact vehicle at buyer handoffs. It may resolve persistent vehicle identity, distance and speed, but it must not own payout, cargo inventory, market settlement, reputation, law response or a parallel SaveGame.

This prevents an opportunistic second vehicle from being parked beside a terminal to complete cargo it never collected. The original `AGTTFarmJobDirector` remains authoritative for stage, timer, cargo integrity, reward, market settlement, logistics history and persistence. The handoff guard does not create another delivery timer, another cargo-condition value or a parallel reward path.

## Roadmap progress vs release readiness

The roadmap checklist is currently **125 / 130 tasks complete (96.2%)** and is visualized by deterministic SVGs generated from `Docs/ROADMAP.md`. That percentage measures the roadmap checklist only. It is not a demo-readiness percentage and it does not authorize a release.

Release readiness remains **NOT READY** while Native Chaos/authored-trailer/Win64 runtime and visual gates remain open.

## Demo evidence still required

Source sanity can prove wiring and regression contracts but cannot prove Unreal runtime quality. A release candidate must still provide:

- a verified Unreal Engine 5.8 Win64 compile/cook/package;
- packaged-EXE runtime smoke evidence from the exact candidate;
- Native Chaos drivetrain/wheel evidence and authored-trailer acceptance required by the roadmap;
- successful execution of the connected Farm Cargo slice in the packaged build, including same-vehicle pickup-to-handoff authority;
- rendered visual acceptance showing coherent world, vehicles, NPCs, UI/HUD, lighting and route guidance without placeholder clutter;
- green relevant GitHub Actions and no demo-critical blockers.

Until every release gate above is satisfied, the project remains source development and **no public demo release** should be created.
