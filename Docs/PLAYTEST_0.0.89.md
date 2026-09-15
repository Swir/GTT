# GTT 0.0.89 Playtest — Native Roadblock Runtime Evidence

Run against a packaged Win64 build from the same commit SHA with `-GTTDemoSmokeScenario -log`.

The existing 23-step route must still pass: world/HUD/traffic/NPC/mission/combat, Native readiness/motion/control for Fieldmaster, Rattleback and Mulebox, wanted-4 escalation, police response, pursuit closing, active roadblock, road-node interception and save.

In addition, the runtime log must contain a real `ROADBLOCK_SPIKE_CONSEQUENCE` for `Rattleback82` or `Mulebox1200` with `path=NATIVE_CHAOS`. The reported `tire_after` must be lower than `tire_before` and `tire_delta` must be positive. `evaluate_demo_scenario.ps1` records those measurements in `DEMO_SCENARIO.json` schema `gtt.demo-scenario.v6`.

A candidate that reaches wanted 4 and spawns a roadblock but never produces Native tire damage must fail this milestone. This is intentional: presence of the system is no longer accepted as proof of gameplay consequence.

Passing still does not authorize a public demo by itself. UE 5.8 Win64 packaging, packaged EXE smoke, the complete technical gate and rendered visual acceptance all remain mandatory.
