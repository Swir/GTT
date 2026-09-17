# GTT Demo Vertical Slice Contract

The first public GTT demo should prove a connected rural sandbox loop rather than a collection of disconnected systems. The current reference slice is the legal **Farm Cargo** route because it already crosses the fleet, economy, logistics, traffic, NPC, vehicle-condition, police/ranger, day/night and persistence layers.

## Player-facing slice

1. Use the unified contract board to inspect and prepare a suitable cargo vehicle.
2. Accept Farm Cargo only when legal-work lockouts and the depot schedule/stock allow it.
3. Follow the world-space beacon to Feed Depot and physically load the cargo vehicle.
4. Drive through the living village while cargo mass, vehicle condition, traffic, NPCs, day/night and law response remain active.
5. Reach Hill Farm, stop the cargo vehicle and perform the legal handoff. Higher logistics tiers continue the same load to North Wood Yard.
6. Finish the chain and receive the existing economy payout, logistics reputation/history update and save persistence.

## Authority rules

The route beacon is presentation only. It must never own cash, stock, reputation, Wanted heat, wildlife alert, save data or contract state. Those remain in the existing contract board, farm director, logistics subsystem, economy component, Wanted/ranger systems and game-mode save path.

Likewise, the stopped-vehicle handoff rule only prevents unsafe drive-by completion. It does not create another delivery timer, another cargo state or a parallel reward path.

## Demo evidence still required

Source sanity can prove wiring and regression contracts but cannot prove Unreal runtime quality. A release candidate must still provide:

- a verified Unreal Engine 5.8 Win64 compile/cook/package;
- packaged-EXE runtime smoke evidence from the exact candidate;
- Native Chaos drivetrain/wheel evidence and authored-trailer acceptance required by the roadmap;
- successful execution of the connected slice in the packaged build;
- rendered visual acceptance showing coherent world, vehicles, NPCs, UI/HUD, lighting and the new route guidance without placeholder clutter;
- green relevant GitHub Actions and no demo-critical blockers.

Until every release gate above is satisfied, the project remains source development and no public demo release should be created.
