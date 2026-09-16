# GTT 0.0.90 Playtest — Deterministic Native Roadblock Crossing

Run against a packaged Win64 build from the same commit SHA with `-GTTDemoSmokeScenario -log`.

The established world, HUD, traffic, NPC, mission, combat, Native fleet, wanted-4, police pursuit, roadblock, interception and save evidence must still pass.

After `ROADBLOCK_ACTIVE`, the deterministic route must select a ready Rattleback 82 or Mulebox 1200 and an active `AGTTRoadblock`. The vehicle is staged on the approach axis behind the real spike-strip component, aligned toward the strip, and driven forward through the real `UChaosWheeledVehicleMovementComponent` rather than invoking tire damage directly.

Required runtime chain:
1. `DEMO_SCENARIO_ROADBLOCK_CROSSING ... phase=STAGED` identifies vehicle, roadblock and baseline tire/wheel-risk values.
2. The roadblock emits `ROADBLOCK_SPIKE_CONSEQUENCE ... path=NATIVE_CHAOS` from the real component-hit path.
3. The scenario emits `DEMO_SCENARIO_ROADBLOCK_CROSSING ... result=PASS` only after the roadblock reports that exact vehicle as physically spiked.
4. `DEMO_SCENARIO_HANDLING_CONSEQUENCE ... result=PASS` requires tire integrity below baseline and runtime wheel-risk above baseline.
5. `DEMO_SCENARIO_COMPLETE result=PASS steps=25` is required.

`evaluate_demo_scenario.ps1` must write schema `gtt.demo-scenario.v7` and retain physical crossing plus before/after wheel-risk evidence. Missing contact, unchanged tires or unchanged wheel-risk is a hard FAIL.

This milestone does not authorize a public demo by itself. A real UE 5.8 Win64 package, packaged EXE runtime smoke, complete technical gate and rendered visual acceptance remain mandatory.
