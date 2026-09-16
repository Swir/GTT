# GTT 0.0.91 Playtest — Post-Spike Escape Dynamics

Run against a packaged Win64 build from the same commit SHA with `-GTTDemoSmokeScenario -log`.

All established world, HUD, traffic, NPC, mission, combat, Native fleet, wanted-4, pursuit, roadblock, interception, physical crossing and save evidence must remain green.

After the Native Rattleback 82 or Mulebox 1200 physically crosses the active spike strip:
1. `DEMO_SCENARIO_HANDLING_CONSEQUENCE` must report lower tire integrity and higher wheel risk than the staged baseline.
2. The same event must report `throttle_limit_before/after` and `steering_limit_before/after`; at least one control-authority limit must be lower after the strike.
3. The scenario must continue the same damaged Native vehicle with full throttle and alternating steering for at least three seconds.
4. `DEMO_SCENARIO_POST_SPIKE_ESCAPE ... result=PASS` requires live Native wheel motion after that interval and records start/current speed plus the damaged handling limits.
5. `DEMO_SCENARIO_COMPLETE result=PASS steps=26` is mandatory.

`evaluate_demo_scenario.ps1` must emit `gtt.demo-scenario.v8` and hard-fail unchanged control authority, missing post-spike motion, missing physical crossing, missing spike damage, SHA mismatch or any prior required scenario step.

This is still not public-demo approval. The exact commit must additionally pass real UE 5.8 Win64 package, packaged EXE smoke, technical gate, green relevant Actions and rendered visual acceptance.
