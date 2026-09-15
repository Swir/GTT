# GTT 0.0.86 Playtest — Wanted-4 Roadblock & Interception Proof

Run only against a packaged Win64 build from the same commit SHA with `-GTTDemoSmokeScenario -log`.

Expected route: WORLD, HUD, TRAFFIC, NPC, MISSION, COMBAT; Native readiness, motion and CONTROL for Fieldmaster, Rattleback 82 and Mulebox 1200; WANTED_COMPONENT; 130-heat escalation to wanted >=4; POLICE_RESPONSE; PURSUIT_ACTIVE; PURSUIT_CLOSING; ROADBLOCK_ACTIVE; INTERCEPTION_ACTIVE; SAVE.

The wanted-4 stage must produce a real active roadblock through `AGTTPoliceDirector::GetActiveRoadblockCount()`. The log must include `DEMO_SCENARIO_ROADBLOCK active=PASS count=...`.

Road-node interception must be active at the same escalation level and expose the selected node label through `GetLastInterceptionNodeLabel()`. The log must include `DEMO_SCENARIO_INTERCEPTION active=PASS node=...`. A configured roadblock class or non-empty road graph alone is not sufficient.

All 0.0.85 vehicle-control and pursuit-closing evidence remains mandatory. `DEMO_SCENARIO_COMPLETE result=PASS steps=23` and `DEMO_SCENARIO.json` schema `gtt.demo-scenario.v5` are required. This does not replace rendered visual acceptance or fleet-wide `NATIVE_CHAOS_SMOKE_READY` evidence.
