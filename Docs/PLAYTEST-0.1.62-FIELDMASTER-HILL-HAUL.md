# GTT 0.1.62 — Fieldmaster Hill-Haul Control Playtest

## Scope

This milestone improves the native Rusty Fieldmaster 60 driving loop on grades and low-grip terrain without claiming packaged-runtime acceptance. The dedicated Chaos movement component now composes terrain traction with the existing condition, tire and trailer-load authority, adds loaded hill hold at very low speed, and adds bounded throttle-off tow braking while descending.

The 0.1.59 heavy-haul limits remain hard ceilings on firm ground: a fully loaded attached trailer keeps maximum throttle authority at 70% and steering authority at 88%. The 0.1.62 terrain layer may reduce throttle further, but never raises those limits.

## Source-level acceptance

Run:

```text
python Scripts/verify_v0_1_62_fieldmaster_hill_haul_control.py
```

The verifier must confirm all of the following:

- firm ground + healthy tires + no trailer preserves 100% throttle authority;
- firm ground + full trailer preserves the 0.1.59 70% throttle / 88% steering limits;
- low terrain grip reduces propulsion instead of only steering authority;
- loaded hill hold activates only with released throttle, meaningful tow load, a >=4 degree grade and <=2.5 km/h speed;
- loaded downhill tow braking activates only with released throttle, a descending >=4 degree grade and >=10 km/h speed;
- deliberate throttle input releases automatic hill/downhill braking immediately;
- a trailer load below 0.15 does not activate hill-haul assistance;
- the dedicated movement component still does not issue `SetTargetGear`, preserving the shared Native drivetrain authority.

## PIE / editor playtest

Use the native Fieldmaster takeover path with a real attached farm trailer. Perform the cases below in both third-person and the normal HUD state.

1. **Firm road baseline** — empty tractor, healthy tires, flat road. Full throttle must feel unchanged from 0.1.61 and no hill-haul brake should remain active.
2. **Full trailer baseline** — fully load the trailer on flat firm road. Confirm the existing heavy-haul acceleration/steering penalty remains present and no extra brake appears on level ground.
3. **Mud climb** — enter a mud/low-grip surface while towing. The tractor should keep crawl torque but should not accept the same effective throttle as clean pavement. Steering should continue to use the existing terrain/tire authority.
4. **Loaded hill hold** — stop with the loaded trailer on a clear 6–12 degree grade and release throttle. The tractor/trailer combination should resist rollback without requiring a full service-brake spike. Applying throttle must release the hold.
5. **Loaded descent** — descend a 6–12 degree grade above 10 km/h with throttle released. Tow braking should grow with speed/grade and remain bounded; applying throttle must release it.
6. **Reverse grade direction** — reverse on a grade and verify grade sign follows actual travel direction rather than tractor nose direction.
7. **Damage/low fuel regression** — verify the existing invalid/no-fuel/critical-condition stop path still wins over hill-haul logic.
8. **Direction interlock regression** — request forward/reverse transitions and confirm the shared Native drivetrain subsystem still owns target-gear changes.

## Runtime evidence to capture later

A qualifying Windows + Unreal Engine 5.8 packaged run should capture, from the same candidate SHA:

- hill-hold start/release on a loaded trailer;
- downhill tow-brake start/release above threshold;
- flat-road full-load throttle authority remaining <=0.70;
- low-grip throttle authority below the equivalent firm-road case;
- no new automatic-gearbox/interlock regression;
- stable hitch/trailer behaviour with the authored trailer runtime bridge active.

These observations are useful evidence but do **not** close the remaining roadmap Native Chaos/Win64 checkboxes until the existing packaged acceptance gates pass.

## Demo release impact

0.1.62 improves the feel and safety of the farm/haul loop, but it does not authorize a demo release by itself. Demo release still requires the existing verified UE 5.8 Win64 package, packaged EXE smoke test, Native Chaos/trailer runtime acceptance, rendered visual evidence, human visual acceptance and green relevant CI on the same candidate.
