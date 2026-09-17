# GTT 0.1.15 Playtest — Native Chaos Runtime Telemetry & Drivetrain Acceptance

This milestone does not claim a Win64 runtime success from source checks. It makes a real Unreal Engine 5.8 packaged run capable of proving whether the Rusty Fieldmaster 60 is actually using the dedicated Native Chaos drivetrain, wheels and suspension under load.

## A. Source/evidence contract

1. Run `python Scripts/verify_fieldmaster_runtime_telemetry.py` and confirm it passes.
2. Run `python Scripts/verify_win64_evidence_pipeline.py` and confirm the candidate path requires `FIELDMASTER_CHAOS_TELEMETRY.json`.
3. Confirm `Docs/ROADMAP.md` still reports the mathematically correct 125/130 = 96.2% and keeps all five runtime/content blockers open until real evidence exists.

## B. UE 5.8 / Win64 runner

4. On the project-controlled Windows x64 runner, execute `Scripts/preflight_win64_unreal.ps1` and require `WIN64_PREFLIGHT.json` PASS for UE 5.8, Chaos Vehicles, UBT, MSVC, Windows SDK and Git LFS.
5. Run the `Win64 package evidence` workflow for version 0.1.15. `BUILD_ATTEMPT.json` must report PASS and UAT exit code 0.
6. Confirm the packaged archive contains exactly one `GTT.exe`, cooked `.pak/.utoc/.ucas` data and a PASS `PACKAGE_VALIDATION.json`.
7. Launch the packaged EXE through `smoke_test_windows.ps1`; `RUNTIME_SMOKE.json` must report PASS and the exact candidate SHA.

## C. Dedicated Fieldmaster drivetrain telemetry

8. Confirm the runtime log contains repeated `FIELDMASTER_CHAOS_TELEMETRY result=OBSERVED` lines emitted while the deterministic scenario drives the tractor.
9. Confirm the Fieldmaster scenario uses `ApplyFieldmasterDriveCommand` rather than the generic road-vehicle input helper.
10. Confirm at least one sample has active movement, non-zero current gear, positive engine RPM, requested/effective throttle and measurable forward speed.
11. Confirm at least one sample reports exactly four valid Chaos wheels, at least two wheel contacts and four normalized suspension samples in the 0..1 range.
12. Confirm live wheel spring force is positive while grounded and total Chaos drive torque becomes non-zero under throttle.
13. Confirm `FIELDMASTER_CHAOS_TELEMETRY.json` uses schema `gtt.fieldmaster-chaos-telemetry.v1`, reports PASS, records at least three observations and is bound to the exact `BUILD_INFO.json` Git SHA.
14. Confirm the manifest summarizes observed gears, maximum speed, maximum engine RPM, maximum wheel drive torque and maximum spring force without NaN/Infinity values.

## D. Existing packaged gameplay regressions

15. Confirm `DEMO_SCENARIO.json` still passes schema `gtt.demo-scenario.v11` with all 33 required evidence steps, including Fieldmaster/Rattleback/Mulebox motion, wanted escalation, pursuit, physical spike crossing, post-spike escape, damage persistence, workshop recovery and structural limp-home.
16. Confirm `GAMEPLAY_SMOKE.json` remains PASS and the runtime log contains no fatal/assert/unhandled-exception markers.
17. Confirm `DEMO_TECHNICAL_GATE.json` remains schema 4 but now contains `fieldmaster_native_chaos_telemetry: PASS` and the telemetry sample count.

## E. Runtime acceptance decision

18. Only after steps 4–17 pass on the exact candidate may the dedicated Fieldmaster movement/drivetrain/wheel/suspension runtime blockers be reviewed for closure. A source-only CI pass is insufficient.
19. Do not close `Authored skeletal trailer wheel assets and final hitch sockets` from this milestone; it requires its own authored asset/runtime proof.
20. Do not create a Demo Release yet unless rendered visual acceptance is also performed on the exact packaged candidate: world presentation, vehicles, characters/weapons, HUD/UI, lighting/atmosphere, NPC/traffic density, mission/combat/vehicle slice and absence of obvious placeholder clutter must all be acceptable.

## Expected evidence bundle

A technically valid candidate must retain `WIN64_PREFLIGHT.json`, `BUILD_ATTEMPT.json`, `BUILD_INFO.json`, `PACKAGE_VALIDATION.json`, `RUNTIME_SMOKE.json`, `FIELDMASTER_CHAOS_TELEMETRY.json`, `DEMO_SCENARIO.json`, `GAMEPLAY_SMOKE.json`, `GTT_RUNTIME.log` and `DEMO_TECHNICAL_GATE.json`. Visual acceptance remains a separate final Demo Release gate.
