# GTT 0.1.64 — Trailer Brake Runaway Safety Playtest

## Scope

Validate the driver-facing thermal states and the bounded runaway-mitigation extension to the 0.1.63 trailer-brake thermal model. This source milestone does **not** replace packaged Win64 qualification and does not close Native Chaos, authored-trailer or release gates by itself.

## Required setup

- Rusty Fieldmaster 60 on the dedicated native Chaos movement path.
- Attached farm trailer with at least 50% tow load for runaway-mitigation scenarios.
- Repeatable 8–12 degree downhill segment where the combination can exceed 24 km/h safely.
- Blueprint/debug readout for `GetTrailerBrakeHeat01`, `GetTrailerBrakeAuthority`, `GetTrailerBrakeThermalState`, `IsTrailerRunawayMitigationActive`, `GetTrailerRunawaySafetyBrake`, `GetHillHaulBrake`, speed, grade and tow load.

## Acceptance scenarios

1. **Normal → Hot:** raise trailer heat through 0.50. State enters `Hot`; cooling back to 0.49 must not flicker to Normal.
2. **Hot → Normal hysteresis:** cool below 0.42. State returns to `Normal`.
3. **Hot → Fading:** cross 0.62. State becomes `Fading`; cooling to 0.59 must remain Fading until heat drops below 0.56.
4. **Fading → Critical:** cross 0.88. State becomes `Critical`; cooling to 0.82 must remain Critical until heat drops below 0.78.
5. **Narrow runaway activation:** with Critical heat, >=50% load, <=-8 degree travel grade, >=24 km/h and throttle released, mitigation activates and reports a positive safety-brake value.
6. **Bounded authority:** at worst-case heat/grade/speed/load, added safety brake remains <=0.20 before final brake clamping.
7. **Driver override:** apply deliberate throttle. Automatic downhill tow braking and runaway mitigation must release; the system must not fight the driver.
8. **Threshold rejection:** separately test load below 0.50, grade shallower than 8 degrees and speed below 24 km/h. None may activate runaway mitigation.
9. **Thermal regression:** cold 0.1.63 trailer-assist authority remains unchanged; fade remains bounded to its existing 55% floor and cools normally.
10. **Hill-hold/base-brake regression:** loaded near-stop hill hold and existing base brake remain outside thermal fade and runaway logic.
11. **Heavy-haul regression:** full-load firm-road throttle and steering ceilings remain the established 0.70 / 0.88.
12. **Drivetrain ownership:** no per-tick Fieldmaster path may call `SetTargetGear`; shared forward/reverse authority remains canonical.

## Evidence to capture

For a future packaged candidate, capture Normal/Hot/Fading/Critical transitions plus one mitigation activation and one deliberate-throttle release. Preserve exact candidate SHA/version/configuration through the existing Win64 attestation path. Source CI is not release evidence.

## Expected release status after source acceptance

- Roadmap remains **125 / 130 (96.2%)** unless one of the five existing runtime/build gates is independently proven.
- Demo remains **NOT READY** without a verified UE 5.8 Win64 package, packaged EXE smoke, Native Chaos/trailer runtime evidence and human visual acceptance of the same candidate.
