# GTT 0.0.94 Playtest — Persistent Structural Limp-Home Dynamics

## Goal
Verify that serious Native road-vehicle body damage materially changes driving, survives the real save/load path, and is removed only by the real paid workshop path. This milestone is not satisfied by log text alone: the packaged route must emit the measured consequence values and the evaluator must reject weak/no-op damage.

## Manual gameplay route
1. Own and activate the Native Chaos Rattleback 82 or Mulebox 1200.
2. Drive normally and confirm the vehicle does not pull sideways and does not lose speed from the structural consequence layer while body health is pristine.
3. Create a serious front crash plus a one-sided crash. The vehicle should remain usable, but become a believable limp-home vehicle rather than instantly becoming an immobile prop.
4. Continue driving. Confirm that speed bleeds more aggressively, the vehicle pulls toward the damaged side, and severe radiator/front damage produces an obvious limp-home reduction on top of the existing condition/tire handling penalties.
5. Save progress, reload progress, re-enter/continue with the same Native vehicle, and confirm the degraded behavior remains consistent with the saved body/cooling/panel state.
6. Use the real Workshop service and confirm cash is charged. The repaired vehicle should return to neutral steering and normal structural drag/power behavior.
7. Repeat with the other Native road vehicle to check that the system is fleet-wide and does not depend on a single pawn class instance.

## Deterministic packaged route
Launch through `smoke_test_windows.ps1` with `-GTTDemoSmokeScenario`. After the existing 0.0.93 structural persistence/recovery proof completes, the 0.0.94 subsystem stages fresh front/right damage and must produce all three new PASS records:

- `DEMO_SCENARIO_STRUCTURAL_HANDLING ... result=PASS` — requires nontrivial structural severity, physical drag, asymmetric pull, reduced power/steering retention and active limp-home state.
- `DEMO_SCENARIO_STRUCTURAL_RELOAD_HANDLING ... result=PASS` — requires SaveProgress/LoadProgress to reconstruct the same consequence values within tolerance after an anti-stale pristine mutation.
- `DEMO_SCENARIO_STRUCTURAL_DRIVE_RECOVERY ... result=PASS` — requires a paid Workshop interaction to restore near-zero severity/drag/pull, full retention and `limp_after=NO`.

The subsystem must then emit `DEMO_SCENARIO_STRUCTURAL_DRIVE result=PASS`.

## Evidence gate
`Scripts/evaluate_demo_scenario.ps1` must create `DEMO_SCENARIO.json` with schema `gtt.demo-scenario.v11` and exactly 33 required steps. The new steps are `STRUCTURAL_HANDLING`, `STRUCTURAL_RELOAD_HANDLING`, and `STRUCTURAL_DRIVE_RECOVERY`; failure of any one makes the packaged demo scenario fail.

The Win64 evidence workflow must run the packaged EXE for at least 125 seconds with a 145-second launch timeout, then run the scenario evaluator before the general packaged gameplay smoke and technical candidate gate.

## Regression checks
- Spike-strip tire damage and post-spike escape remain required.
- 0.0.92 tire/condition SaveProgress/LoadProgress and paid workshop recovery remain required.
- 0.0.93 exact body-zone/cooling/detached-panel persistence and structural surcharge remain required.
- The new subsystem must read `FGTTRoadBodyDamageSnapshot`; it must not create a second structural save representation.
- Pristine vehicles must not receive structural drag or pull.
- The SWIR Roadmap Style Lock v1 dashboard must remain 125/130 = 96.2% unless a real roadmap checkbox becomes complete.

## Demo-release decision
Do **not** publish a demo from source checks. A release still requires a real UE 5.8 Win64 package, successful packaged-EXE runtime smoke, green relevant Actions, the full 33-step scenario, and rendered visual acceptance proving the world/vehicles/characters/UI/lighting are presentation-ready. Prototype detached-panel cubes are not final art and remain a visual-acceptance concern.
