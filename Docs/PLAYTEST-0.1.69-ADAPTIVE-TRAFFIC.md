# GTT 0.1.69 — Adaptive Living Traffic Playtest

## Scope

This milestone replaces the traffic director's one-shot startup spawn with a bounded, day/night-aware population loop that reuses the existing road graph and existing `AGTTTrafficCarPawn` driver/incident behavior. It is a gameplay integration change, not a release-readiness shortcut.

Roadmap truth remains **125/130 (96.2%)**. No roadmap gate is closed by this source-level milestone.

## What changed

- Traffic population now reconciles throughout play instead of spawning once at `BeginPlay`.
- The existing `AGTTDayNightCycle` is the population clock authority.
- Default managed population profiles are:
  - NIGHT: 5 vehicles.
  - DAY: 8 vehicles.
  - RUSH: 11 vehicles during 06:00–09:00 and 16:00–19:30.
- Population transitions are bounded to at most three additions/removals per reconciliation pass after initial world fill, preventing a visible burst at a clock boundary.
- Approximately two of every five deterministic spawn-route selections use rural round trips between village/North Wood and Farm North/Hill Farm; remaining selections use the village loop.
- Opposing route directions receive a right-hand lane offset derived from local route tangents instead of sharing the exact center line.
- Cars are tagged `GTT.ManagedTraffic` so population ownership is explicit.
- Night/rush downsizing is safety-aware: occupied cars, disabled incident cars, active roadside-assistance scenes, county roadside-service scenes, and ranger road-stop traffic are never removed by the population manager.
- A managed car must also be farther than 3200 cm from the player before it can be culled; the farthest eligible car is removed first.
- If no player pawn exists, downscaling fails safe and temporarily leaves the population above target rather than destroying an uncertain vehicle.

## Source-level verification

Run from repository root:

```powershell
python Scripts/verify_adaptive_traffic_population.py
```

Expected source-contract result:

- profile math resolves to NIGHT=5, DAY=8, RUSH=11;
- rush/day/night ordering is preserved;
- road-graph village and rural routes remain connected;
- incident/roadside/ranger-stop and occupied-vehicle culling guards remain present;
- player-proximity protection and bounded population adjustment remain present;
- project version is 0.1.69;
- roadmap remains exactly 125 checked / 5 open.

GitHub Actions also runs `.github/workflows/adaptive-traffic-sanity.yml` for changes affecting this system.

## Unreal playtest plan

### 1. Daytime baseline

1. Start the prototype world around 12:00.
2. Observe village and rural roads for at least two population-reconciliation intervals.
3. Confirm the traffic director reports profile `DAY`, target 8, and converges without a burst after the initial fill.
4. Confirm village cars follow separated opposing lanes rather than occupying the identical road center line.
5. Drive from the village toward North Wood and Hill Farm and confirm rural commuter routes are active.

### 2. Morning rush transition

1. Restore world time just before 06:00.
2. Cross into 06:00–09:00.
3. Confirm profile becomes `RUSH` and target becomes 11.
4. Confirm at most three cars are added in one reconciliation pass and traffic grows over subsequent passes.
5. Confirm new cars use the shared road graph and ordinary traffic AI/avoidance rather than a separate rush-hour actor type.

### 3. Evening rush and night reduction

1. Restore time to 18:00 and confirm RUSH target 11.
2. Advance beyond 19:30 and confirm profile returns to DAY before the existing night boundary.
3. Advance beyond the `AGTTDayNightCycle::IsNight()` boundary and confirm profile becomes NIGHT with target 5.
4. Confirm removals occur away from the player and are bounded rather than visibly deleting nearby cars.

### 4. Incident preservation during downscale

1. During RUSH, create a collision severe enough to disable one managed traffic vehicle.
2. Start player roadside assistance on one disabled managed car, or allow the county road-service responder to own the scene.
3. Create/enter a ranger road-stop traffic-control scene with another managed traffic car if available.
4. Advance time into NIGHT.
5. Confirm none of the protected incident/assistance/ranger-stop cars are culled even if the live count temporarily remains above target.
6. After protected states clear and cars move beyond the cull radius, confirm population can converge to the NIGHT target.

### 5. Occupied-car protection

1. Enter a managed traffic vehicle.
2. Trigger a population reduction.
3. Confirm the occupied vehicle is never selected for culling.
4. Exit it near the player and confirm proximity still protects it.
5. Move far away; only then may the director eventually choose it if it is otherwise safe.

## Regression checks

- Police interception and story route hints still use the same `FGTTRoadGraph` data.
- Traffic collision response, hit-and-run consequences, player roadside assistance, county road-service response and ranger road stops continue using the existing `AGTTTrafficCarPawn` state rather than duplicate traffic actors.
- No managed population operation changes player cash, wanted heat, mission state, vehicle ownership or save data.
- World performance remains bounded: population has an explicit maximum of 14 and reconciliation occurs at a 3-second default interval.

## Verification boundary

These repository checks verify source contracts only. They do **not** prove that Unreal Engine 5.8 compiled the C++ change, that Win64 packaging succeeded, that the packaged `GTT.exe` passed a runtime smoke test, or that the resulting traffic density looks acceptable in a rendered build. The qualifying self-hosted UE 5.8 / Win64 runner is still required for those claims.

Therefore **DEMO remains NOT READY** until the existing package/runtime/visual acceptance gates are actually satisfied.
