# GTT 0.0.25 — World Performance Playtest

This milestone adds a shared distance-based simulation budget for civilians and traffic so the countryside can grow without every ambient actor doing full-rate work at all times. Combat and mission-critical behavior must remain fully responsive.

## World Performance smoke test

1. Start on Player Farm and remain idle for 30 seconds.
2. Drive the full village loop, then North Pass and the hostile-territory roads.
3. Confirm nearby civilians still wander according to their day/night schedule and transition naturally when the player approaches.
4. Confirm distant civilians can become simulation-dormant without disappearing, duplicating or losing their schedule state.
5. Return to an earlier district and verify civilians resume movement rather than remaining permanently frozen.

## Traffic budget

1. Follow a village traffic car closely for at least one full loop. Nearby traffic must retain obstacle avoidance, horn feedback and stuck recovery.
2. Move several kilometres away from the village. Far traffic should continue progressing on the shared road graph with reduced AI tick frequency.
3. Return to the village and verify traffic resumes full nearby obstacle probes without teleporting or creating duplicate vehicles.
4. Block a nearby traffic car with the player vehicle and confirm the expensive obstacle trace is active in the near/critical simulation tiers.

## Combat priority regression

1. Enter a Rust Dogs, Stone Crows or Mud Jackals hostile territory.
2. Trigger an encounter and immediately move around the edge of the encounter radius.
3. Confirm hostile NPCs remain full-rate and responsive while combat is active: chasing, attacking, knockback and knockout timers must not be throttled by distance budgeting.
4. Trigger a Bent Axle brawl and repeat the same responsiveness check.
5. Hit a civilian and confirm retaliation/flee behavior immediately wakes the actor into the critical tier.

## Long-distance regression

- Drive Player Farm -> North Wood -> Hill Farm -> North Pass -> Ridge Exchange -> village without restarting.
- Watch for traffic stalls, actors permanently sleeping, schedule jumps, hitching when crossing budget radii or combat actors being incorrectly dormant.
- Verify wanted/police, ranger, missions, heavy haul and save/load still work; this milestone must optimize simulation frequency without altering persistent gameplay state.

## Performance observation

Use Unreal's normal `stat game`, `stat unit` and `stat fps` commands during a local UE-equipped playtest. Compare a dense village scene while stationary, then repeat after driving far enough that village actors enter Far/Dormant tiers. Record Game Thread timing and visible actor behavior. The repository sanity suite validates the budget wiring structurally, not actual frame-time improvement on hardware.

## CI / build boundary

`Scripts/verify_world_performance.py` verifies simulation tiers, civilian/traffic integration, combat wake-up behavior, trace gating and exact SWIR roadmap arithmetic. Repository CI still does **not** provide a full Unreal Engine 5.8 Win64 compile/package/smoke environment, so this milestone does not claim a verified packaged EXE.
