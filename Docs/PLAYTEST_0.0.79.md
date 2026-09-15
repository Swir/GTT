# GTT 0.0.79 Playtest — Native Chaos Acceptance Matrix

## Goal

Capture one coherent runtime proof for each Native vehicle showing that authored wheel setup, powertrain, Physics Asset, live wheel states and suspension all remain valid together under real driving.

## Preconditions

1. Use an Unreal Editor/Development build containing the current Native Fieldmaster, Rattleback 82 and Mulebox 1200 rigs.
2. Filter Output Log for `NATIVE_CHAOS_ACCEPTANCE_MATRIX` and `NATIVE_CHAOS_SMOKE_READY`.
3. Activate normal legacy-to-Native takeover; do not manually force hidden Native pawns into play.

## Scenario A — Fieldmaster sustained acceptance

Drive Rusty Fieldmaster 60 for at least 25 seconds across flat road, a bump and a mild slope. Confirm matrix lines show `wheels_cfg=PASS`, `powertrain_cfg=PASS`, `physics_asset=YES`, `movement=ACTIVE`, `wheels=4/4`, `suspension=4/4`, then confirm exactly one smoke-ready transition for the healthy window.

## Scenario B — road fleet sustained acceptance

Repeat the same route with Rattleback 82 and Mulebox 1200. Each vehicle must independently accumulate at least 20 accepted seconds and measurable suspension travel before `NATIVE_CHAOS_SMOKE_READY` appears.

## Scenario C — static-but-valid rejection of smoke proof

Leave a Native vehicle stationary on perfectly flat ground. The matrix may remain accepted, but smoke readiness must not be treated as meaningful until ground-contact and suspension-travel evidence has been observed.

## Scenario D — configuration regression

In an editor-only test copy, invalidate one canonical wheel class or powertrain field. Confirm the matrix reports the corresponding configuration failure and never emits smoke-ready evidence for that unhealthy window.

## Scenario E — runtime regression and recovery

Invalidate a live wheel/suspension or Physics Asset condition long enough to trigger the existing 0.0.78 fallback. Restore the authored rig and reactivate takeover. Confirm the acceptance timer restarts from zero rather than carrying stale proof across the failure.

## Regression

- 0.0.78 fleet-wide runtime fallback remains authoritative for unhealthy takeover.
- 0.0.77 command composition remains the final throttle/brake/steer/gear authority.
- 0.0.76 axle traction, 0.0.74 rollover safety and road-fleet damage/workshop systems remain active.
- No roadmap checkbox is closed solely from source-level Project sanity.

## Demo gate

A `NATIVE_CHAOS_SMOKE_READY` line from Editor/Development is useful runtime evidence, but the first public demo still requires the same evidence from a genuinely packaged Win64 build plus successful packaged EXE launch, gameplay smoke and visual acceptance.
