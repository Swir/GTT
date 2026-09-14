# GTT 0.0.63 Playtest — Native Chaos Wheel-State Runtime Control

## Goal

Validate that the Native Fieldmaster no longer relies only on authored bone/trace proxies for contact and traction decisions. When Chaos exposes four valid wheel states, the live stability controller must consume `FWheelStatus` directly; trace probes remain a safe fallback when the runtime state is incomplete.

This milestone does **not** by itself close the Native Chaos roadmap items. A real UE 5.8 Win64 compile/package and rendered driving session are still required.

## Required runtime setup

1. Build GTT in Unreal Engine 5.8 with Chaos Vehicles enabled.
2. Use the authored Native Fieldmaster setup and activate the existing legacy-to-native takeover.
3. Drive on dry road, uneven shoulder, mud and a moderate hill both unloaded and with Heavy Timber Haul attached.
4. Capture the Unreal log during all cases.

## Acceptance evidence

The log must emit `NATIVE_CHAOS_WHEEL_STATE_EVIDENCE` while the Native Fieldmaster is active. The evidence is sourced from `UChaosWheeledVehicleMovementComponent::GetWheelState()` and must show:

- `valid=4/4` for the canonical four-wheel setup,
- Chaos `contacts=x/4`,
- four `NormalizedSuspensionLength` values,
- four spring-force values,
- four drive-torque and brake-torque values,
- slipping/skidding wheel counts,
- maximum runtime slip magnitude and slip angle,
- the derived `chaos_slip_risk` used by GTT traction control.

`NATIVE_STABILITY_EVIDENCE` must additionally report `contact_source=CHAOS`. If all four wheel states are not valid, the system must report `contact_source=TRACE_FALLBACK` and continue using the existing four ground probes rather than inventing Chaos evidence.

## Driving matrix

### Dry road baseline

- Cruise above 15 km/h with smooth throttle and gentle steering.
- Expect 4/4 valid wheel states, normally 4/4 contact, low `chaos_slip_risk` and no unnecessary throttle cut.
- Drive/brake torque should change when throttle and brake commands change.

### Uneven shoulder / cross-axle

- Put one side or one corner over a ditch/shoulder where suspension travel changes visibly.
- Expect the corresponding runtime suspension values and/or contact count to change.
- Sustained loss of contact must feed the existing low-contact stability timer from Chaos state when valid.

### Mud / traction loss

- Enter an authored `GTTMudZone` under moderate throttle.
- Expect tire/mud penalties from existing gameplay plus Chaos slip/skid evidence when the simulation reports it.
- Sustained runtime slip must raise traction risk and progressively reduce requested throttle before the severe stability intervention threshold.

### Heavy Timber Haul

- Attach the farm trailer, add cargo and repeat dry/uneven/mud tests.
- Existing `tow_load`, sway and load-transfer risk must remain active while Chaos wheel-state evidence supplies contact/slip truth.
- Heavy-haul control must not create a second vehicle state or bypass condition/fuel/tire/save systems.

### Runtime-state fallback

- If a development setup temporarily returns fewer than four valid `FWheelStatus` entries, confirm `TRACE_FALLBACK` appears.
- The tractor must remain controllable through the previous probe-based stability path; do not treat missing wheel-state telemetry as a verified Chaos result.

## DEMO gate

0.0.63 is source/runtime instrumentation plus gameplay-control integration only. Do not publish a Demo Release unless the same commit also has a verified UE 5.8 Win64 package, packaged EXE smoke test, rendered visual acceptance, green required CI and no demo-critical blockers.
