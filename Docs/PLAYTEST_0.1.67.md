# GTT 0.1.67 — Native Fieldmaster HUD packaged-runtime evidence

This milestone binds the driver-facing Native Fieldmaster safety HUD contract introduced in 0.1.66 to the same exact packaged Win64 candidate evidence used by the sealed acceptance pipeline.

## What is verified

`Scripts/evaluate_fieldmaster_hud_runtime.ps1` reads the exact candidate `BUILD_INFO.json`, packaged smoke result, Native Chaos authority result, 0.1.65 hill-haul evidence and `GTT_RUNTIME.log`. It derives the HUD safety state from authoritative `NATIVE_FIELDMASTER_RUNTIME_TELEMETRY` using the same precedence as `BuildNativeFieldmasterAlert`: runaway, critical, fade, hot, cooling, descent assist, hill hold, then no alert.

A PASS writes `FIELDMASTER_HUD_RUNTIME.json` with the exact candidate version/SHA and counts for telemetry, visible alerts, assist alerts, thermal alerts, cooling, critical and runaway states. Invalid ranges, split authority, identity mismatch or impossible telemetry combinations fail closed.

## Exact-candidate seal

`run_win64_attested_candidate_acceptance.ps1` now runs the HUD evaluator after hill-haul evaluation and before candidate sealing. `write_win64_candidate_attestation.ps1` requires `FIELDMASTER_HUD_RUNTIME.json`, checks exact SHA/version identity and minimum visible-alert coverage, adds the evidence to the SHA-256 critical-file set and records HUD evidence counters in `WIN64_CANDIDATE_ATTESTATION.json`.

This evidence proves the authoritative packaged telemetry states that feed the HUD contract were exercised. It does **not** claim that the final HUD presentation looks good, and it does not replace the separate human visual review of the exact candidate screenshots.

## Required real Windows playtest

1. Run the sealed candidate acceptance on a qualifying Windows x64 runner with Unreal Engine 5.8.
2. Possess the Native Fieldmaster and attach/load the trailer.
3. Exercise a hill-hold or descent-assist state and at least one trailer thermal/cooling warning state during the same packaged runtime session.
4. Confirm `FIELDMASTER_HUD_RUNTIME.json` is `PASS`, carries the exact candidate SHA/version and has `visible_alert_samples >= 1`.
5. Inspect the five exact-candidate screenshots separately for visual readability before any Demo Release decision.

## Gate boundary

0.1.67 does not close any of the five remaining ROADMAP runtime/art gates by source work alone. Full Unreal compile/package, packaged EXE smoke, dedicated Native Chaos acceptance, authored trailer acceptance and the real Win64 runner remain evidence-gated. Demo Release remains unauthorized until all technical gates and the separate human visual review pass on the same candidate.
