# GTT 0.0.69 Playtest — Native Road Crash-Scene AI & Hit-and-Run

## Preconditions
- Use an authored Native Rattleback 82 or Mulebox 1200 that passes existing takeover gates.
- Own the matching garage vehicle and enter through Native takeover.
- Ensure civilian traffic is active and wanted/police systems are enabled.

## A. Victim emergency stop
1. Collide with a civilian traffic car above the existing incident threshold.
2. Confirm `NATIVE_ROAD_TRAFFIC_INCIDENT` is emitted and includes `zone=` plus `nearby_reactors=`.
3. Confirm the victim stops instead of immediately continuing its route.
4. Confirm `TRAFFIC_CRASH_RESPONSE` is emitted with stop/limp/disabled evidence.

**Pass:** the victim behaves like a fresh crash participant rather than ordinary uninterrupted traffic.

## B. Limp recovery vs disabled vehicle
1. Produce a moderate collision that leaves victim condition above the disable threshold.
2. Wait for the emergency stop to expire and confirm the victim resumes at reduced cruise speed for the limp window.
3. Produce a severe collision that pushes victim condition to/below the disable threshold.
4. Confirm the severely damaged victim remains stopped with HAZARD state.

**Pass:** crash severity changes post-impact traffic behavior using the shared damage model.

## C. Nearby traffic scene response
1. Cause a collision while other civilian traffic is within roughly 18 m.
2. Confirm nearby cars slow/stop and bias away from the crash origin.
3. Confirm they are not permanently disabled unless they are directly damaged.

**Pass:** surrounding traffic responds to the scene, reducing immediate pile-ups.

## D. Hit-and-run escalation
1. Cause a qualifying traffic collision and record wanted heat.
2. Stay near the crash for several seconds and confirm no hit-and-run heat is added.
3. Repeat, then drive beyond the hit-and-run escape radius before the incident window expires.
4. Confirm exactly one `NATIVE_ROAD_HIT_AND_RUN` event and one extra wanted heat addition.

**Pass:** fleeing a fresh crash adds a distinct one-time consequence through the existing wanted system.

## E. Regression
- World-object collisions without a traffic victim still do not create traffic crime.
- Rattleback/Mulebox takeover, fuel, tire wear, garage/save mirror and breakdown behavior remain intact.
- Existing traffic route navigation, obstacle avoidance, horn and stuck recovery resume after non-disabling incidents.
- Roadmap remains 125/130 until genuine Unreal/Win64 runtime evidence exists.

## Demo gate
Do not publish a Demo Release from Project sanity alone. Require a real UE 5.8 Win64 package, packaged EXE runtime smoke test, visual acceptance, green relevant CI and no demo-critical blockers.
