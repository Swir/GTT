# GTT 0.0.85 Playtest — Vehicle Control & Pursuit Interaction Proof

Run only against a packaged Win64 build from the same commit SHA with `-GTTDemoSmokeScenario -log`.

Expected route: WORLD, HUD, TRAFFIC, NPC, MISSION, COMBAT; Native readiness, motion and CONTROL for Fieldmaster, Rattleback 82 and Mulebox 1200; WANTED_COMPONENT; 80-heat escalation to wanted >=3; POLICE_RESPONSE; PURSUIT_ACTIVE; PURSUIT_CLOSING; SAVE.

Each Native vehicle is actively driven by the smoke route using throttle, alternating steering and brake inputs. A CONTROL pass is accepted only while live Native Chaos motion evidence is simultaneously valid. The log must contain `DEMO_SCENARIO_CONTROL` evidence for all three vehicle IDs.

Pursuit interaction starts from the first observed active County Patrol Interceptor. The route records its distance to the player and requires the same interceptor to close by at least 250 cm. Spawn alone is not sufficient.

`DEMO_SCENARIO_COMPLETE result=PASS steps=21` and `DEMO_SCENARIO.json` schema `gtt.demo-scenario.v4` are required. Missing steps after 60 seconds fail the route. This does not replace rendered visual acceptance or fleet-wide `NATIVE_CHAOS_SMOKE_READY` evidence.
