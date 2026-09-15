# GTT 0.0.83 Playtest — Vehicle & Crime Packaged Route

## Automated packaged route
Run a packaged Win64 build with `-GTTDemoSmokeScenario -log` through `Scripts/smoke_test_windows.ps1`.

A valid runtime must emit PASS for all 13 steps: WORLD, HUD, TRAFFIC, NPC, MISSION, COMBAT, FIELDMASTER, RATTLEBACK, MULEBOX, WANTED_COMPONENT, WANTED_ESCALATION, POLICE_RESPONSE and SAVE.

Vehicle gates require each Native Chaos pawn to report ready and its legacy takeover to be active. The smoke-only crime action injects 55 heat through the real wanted component; confirm `WANTED_ESCALATION` follows and `POLICE_RESPONSE` is observed from the live police director. Normal launches without `-GTTDemoSmokeScenario` must not inject heat.

`Scripts/evaluate_demo_scenario.ps1` must create `DEMO_SCENARIO.json` with schema `gtt.demo-scenario.v2`, result PASS, 13 passed steps and `crime_action_passed=true`. The manifest SHA must match `RUNTIME_SMOKE.json` and the expected build commit.

## Manual visual acceptance still required
After technical PASS, manually inspect the packaged build for coherent vehicle/world presentation, readable HUD, lighting, NPC/traffic presentation, mission readability and absence of placeholder clutter. This milestone does not substitute for that review and does not authorize a public demo release by itself.
