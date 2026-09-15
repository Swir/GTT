# GTT 0.0.78 Playtest — Fleet-wide Native Chaos Runtime Acceptance

## Scope

Prove in Unreal runtime that Fieldmaster, Rattleback 82 and Mulebox 1200 keep Native control only while their live Chaos movement, Physics Asset, collision, four wheel records and four suspension samples are healthy.

## Preconditions

1. Run an Unreal Editor/Development build with the current authored Native vehicle rigs.
2. Keep Output Log visible and filter for `NATIVE_CHAOS_RUNTIME_`.
3. Exercise all three Native vehicles after their normal legacy-to-Native takeover.

## Scenario A — healthy Fieldmaster

Drive the Fieldmaster on road, bumps and a slope for at least 15 seconds. Confirm repeated `NATIVE_CHAOS_RUNTIME_ACCEPTANCE` lines report `wheels=4/4`, `suspension=4/4`, Physics Asset present and `accepted=YES`. Suspension range should react to terrain.

## Scenario B — healthy road fleet

Repeat with Rattleback 82 and Mulebox 1200. Confirm each persistent vehicle id emits its own accepted evidence while normal body damage, cargo, workshop and drivetrain systems remain active.

## Scenario C — wheel/suspension contract failure

In an editor test copy, intentionally invalidate one authored wheel/suspension setup. Activate takeover and confirm acceptance does not falsely remain healthy. After the 1.5-second grace window, expect `NATIVE_CHAOS_RUNTIME_FALLBACK` and legacy control restoration.

## Scenario D — Physics Asset / movement failure

In an editor test copy, remove the Physics Asset or deactivate Chaos movement after takeover. Confirm the same delayed fallback occurs without leaving the player trapped in a broken Native vehicle.

## Scenario E — transient frame tolerance

Cause a short transient runtime disturbance shorter than the grace window, then restore a healthy contract. Confirm the accumulated unhealthy state clears and takeover remains active.

## Regression

- Direction interlock and command composition from 0.0.77 still own final movement commands.
- Axle traction/suspension evidence still reports normally.
- Fieldmaster heavy-haul, mud and rollover systems remain active.
- Rattleback/Mulebox damage, police incidents, recovery and workshop loops remain active.

## Release honesty

Run `python Scripts/verify_native_runtime_acceptance.py` and full Project sanity. Keep roadmap at **125/130 (96.2%)** until real UE runtime evidence and the remaining Win64/asset gates are genuinely satisfied. This playtest document is a procedure, not proof that the packaged Windows demo has been executed.
