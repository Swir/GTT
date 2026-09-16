# GTT 0.0.92 Playtest — Spike Damage Persistence & Workshop Recovery

Run against a packaged Win64 build from the same commit SHA with `-GTTDemoSmokeScenario -log`.

The complete 0.0.91 core route remains mandatory: world/HUD/traffic/NPC/mission/combat, Native fleet control, wanted-4 escalation, active pursuit, roadblock interception, a physical Native Chaos spike-strip crossing, measurable handling degradation, continued post-spike escape, and save evidence must all pass first.

## Persistence round-trip

After the 26-step core route completes, the dedicated damage-recovery evidence subsystem selects the same damaged owned Rattleback 82 or Mulebox 1200 and executes a real persistence round-trip:

1. Capture damaged tire integrity, condition and current Native handling limits.
2. Deactivate Native takeover so the compatibility mirror receives the exact damaged state.
3. Call the real `AGTTGameMode::SaveProgress()` path and read the primary `GTT_Prototype_01` SaveGame back.
4. Require the matching `OwnedVehicles` record to contain the damaged tire/condition values.
5. Deliberately repair the live legacy mirror before load. This prevents a stale in-memory value from faking persistence.
6. Call the real `AGTTGameMode::LoadProgress()` path.
7. Require the legacy mirror to return to the serialized damaged values.
8. Reactivate Native Chaos takeover and require the Native migration snapshot to import the same reloaded damage.
9. Runtime log must contain `DEMO_SCENARIO_DAMAGE_PERSISTENCE ... result=PASS`.

## Paid workshop recovery

The scenario then exercises the existing workshop/economy integration instead of calling a repair shortcut:

1. Stage the same damaged Native road vehicle inside an existing service terminal radius.
2. Route the terminal through `EGTTServiceType::Workshop`.
3. Ensure the player has a deterministic smoke-test service reserve if required.
4. Call the real `AGTTServiceTerminal::Interact_Implementation()` path.
5. Require cash to decrease, `ApplyNativeWorkshopService()` to restore condition/fuel/tires/body state, and the Native vehicle to report no remaining service need.
6. After Native wheel runtime settles, require repaired tire integrity plus non-regressed control authority and a measurable improvement in wheel risk, throttle limit, or steering limit.
7. Runtime log must contain both `DEMO_SCENARIO_WORKSHOP_RECOVERY ... result=PASS` and final `DEMO_SCENARIO_DAMAGE_RECOVERY result=PASS`.

## Evidence gate

`evaluate_demo_scenario.ps1` must emit schema `gtt.demo-scenario.v9`, route `spike-save-load-workshop-recovery`, and exactly 28 required evidence entries: the established 26 core steps plus `DAMAGE_PERSISTENCE` and `WORKSHOP_RECOVERY`.

The Win64 workflow intentionally keeps the packaged EXE alive for 90 seconds (`LaunchTimeoutSeconds=105`) so the save/load/workshop round-trip has time to execute; a 20-second process-alive check is no longer accepted as gameplay evidence for this milestone.

This still is **not** public-demo approval. The exact commit must additionally pass a real UE 5.8 Win64 package, the 90-second packaged-EXE runtime route, technical candidate gate, green relevant Actions, rendered visual acceptance, and all demo-critical blocker checks.
