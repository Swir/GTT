# GTT 0.1.15 Playtest — Native Chaos Runtime Telemetry & Drivetrain Acceptance

## Purpose

0.1.15 closes the evidence gap between a source-level Native Chaos configuration and proof that the packaged Fieldmaster is actually running its four-wheel Chaos drivetrain. The authoritative automated artifact is `NATIVE_CHAOS_RUNTIME.json`; source CI alone must never be treated as a runtime PASS.

## Required environment

- Windows x64 self-hosted runner or equivalent test PC.
- Unreal Engine 5.8 with Chaos Vehicles available.
- Project and LFS assets at the exact commit being tested.
- A Win64 package produced by `Scripts/package_windows.ps1` after `WIN64_PREFLIGHT.json` and `BUILD_ATTEMPT.json` pass.
- Packaged EXE smoke run long enough for the deterministic demo scenario and telemetry sampler to execute.

## Automated acceptance route

1. Run the Win64 evidence workflow for the exact candidate commit.
2. Confirm `WIN64_PREFLIGHT.json` is `PASS` and identifies the real UE 5.8 toolchain.
3. Confirm `BUILD_ATTEMPT.json` is `PASS` with UAT exit code 0.
4. Confirm `RUNTIME_SMOKE.json`, `DEMO_SCENARIO.json` and `GAMEPLAY_SMOKE.json` are `PASS`.
5. Confirm `GTT_RUNTIME.log` contains repeated `NATIVE_FIELDMASTER_RUNTIME_TELEMETRY vehicle=RustyFieldmaster60` lines.
6. Run `Scripts/evaluate_native_chaos_runtime.ps1` against that package and log.
7. Confirm `NATIVE_CHAOS_RUNTIME.json` uses schema `gtt.native-chaos-runtime.v1`, has `result: PASS`, and carries the same Git SHA as `BUILD_INFO.json` and the requested candidate.
8. Confirm the manifest reports at least two telemetry samples and at least two active-movement samples.
9. Confirm `max_valid_wheels` is 4, `max_contacts` is at least 2 and `max_suspension_samples` is 4 with at least one suspension-ready sample.
10. Confirm `max_speed_kmh` is at least 0.35, at least one non-zero control command was sampled and `observed_gears` contains a non-neutral gear.
11. Confirm the log contains `NATIVE_PHYSICS_EVIDENCE ... accepted=YES` plus `NATIVE_WHEEL_SETUP_EVIDENCE`, and contains no `NATIVE_PHYSICS_FALLBACK vehicle=RustyFieldmaster60`.
12. Confirm the deterministic route independently logged `FIELDMASTER_MOTION` and `FIELDMASTER_CONTROL` as PASS.
13. If a farm trailer is attached during the run, verify telemetry reports `trailer=ATTACHED` and a bounded `tow_load`; absence of a trailer is not by itself a drivetrain failure.
14. Confirm `evaluate_demo_candidate.ps1` accepts the native runtime manifest and writes `DEMO_TECHNICAL_GATE.json` schema 5 with `native_chaos_runtime: PASS`.
15. Confirm the final technical-candidate artifact includes `NATIVE_CHAOS_RUNTIME.json` alongside the preflight, build-attempt, runtime-smoke, deterministic-scenario, gameplay-smoke and raw runtime log evidence.

## Manual handling checks on the same packaged build

- Drive the Fieldmaster forward, brake to a stop, reverse, then return to forward. Look for stable gear transitions rather than an instant high-speed direction flip.
- Perform a low-speed full-left/full-right steering sweep on firm ground. Steering must remain controllable and wheel contact must not collapse because of a source-only setup mismatch.
- Cross uneven ground and a mild slope. Suspension movement should be visible and the telemetry should continue to report four valid suspension samples without a Native Physics fallback.
- Repeat on mud or reduced-grip terrain and verify the tractor remains controllable as traction authority/slip values change.
- Attach the farm trailer, load cargo and repeat a hill start. Tow load must affect the existing heavy-haul path without bypassing native wheel/contact acceptance.
- Damage tires/condition using the existing gameplay systems and confirm the tractor still uses the same Native Chaos movement path and bounded safety interventions.
- Save, reload and re-enter the Fieldmaster. Native takeover and telemetry must recover without spawning a duplicate gameplay vehicle or silently falling back to the legacy movement path.

## Failure conditions

The candidate fails 0.1.15 runtime acceptance if any of these occur: wrong Git SHA, source-only evidence with no packaged log, Native Physics fallback, fewer than four valid Chaos wheels, no usable wheel contact, incomplete suspension sampling, no proven motion/control, no non-neutral gear, evaluator failure, or missing `NATIVE_CHAOS_RUNTIME.json` in the technical artifact.

## Demo release status

Passing this playtest is necessary but not sufficient for the first public demo. The demo still also requires a verified Win64 package, packaged EXE smoke, all relevant GitHub Actions, rendered visual acceptance, and no demo-critical blockers. Do not create a Demo Release from source-contract CI alone.
