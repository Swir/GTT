# GTT 0.0.84 Playtest — Native Motion & Pursuit Runtime Proof

Run only against a packaged Win64 build from the same commit SHA with `-GTTDemoSmokeScenario -log`.

Expected route: WORLD, HUD, TRAFFIC, NPC, MISSION, COMBAT; Native readiness plus FIELDMASTER_MOTION, RATTLEBACK_MOTION and MULEBOX_MOTION; WANTED_COMPONENT; smoke-only 80-heat escalation to wanted >=3; real POLICE_RESPONSE and PURSUIT_ACTIVE; then SAVE.

Each vehicle motion PASS requires active Chaos movement, 4/4 valid wheel states, at least two live ground contacts, 4/4 finite normalized suspension samples and non-trivial planar velocity. Police response PASS requires at least one active spawned foot unit; pursuit PASS requires at least one active pursuit vehicle.

`DEMO_SCENARIO_COMPLETE result=PASS steps=17` and `DEMO_SCENARIO.json` schema `gtt.demo-scenario.v3` are required. Any missing step after 45 seconds is a failure. This test does not replace visual review or the fleet-wide `NATIVE_CHAOS_SMOKE_READY` evidence gate.
