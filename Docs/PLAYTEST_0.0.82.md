# Playtest 0.0.82 — Deterministic Packaged Demo Scenario

## Automated packaged route
Launch the packaged executable with `-GTTDemoSmokeScenario -unattended -nullrhi -NoSound -log`. The scenario is intentionally opt-in and must not affect ordinary gameplay sessions.

Within the smoke window the runtime must emit PASS markers for: WORLD, HUD, TRAFFIC, NPC, MISSION, WANTED and SAVE. SAVE is not a presence check: the scenario calls the real `AGTTGameMode::SaveProgress()` and only passes that step when the write succeeds. The run must finish with `DEMO_SCENARIO_COMPLETE result=PASS`.

`evaluate_demo_scenario.ps1` converts those exact runtime markers into `DEMO_SCENARIO.json`; missing steps fail the Win64 workflow. Existing fleet gates still independently require `NATIVE_CHAOS_SMOKE_READY` for Fieldmaster, Rattleback 82 and Mulebox 1200 and fatal-signature scanning.

## Manual visual acceptance remains mandatory
This headless scenario is technical evidence, not a visual review. Before a public demo release, run the same packaged candidate rendered and verify coherent countryside/vehicle/character presentation, readable HUD, lighting/atmosphere, no placeholder wall-of-text clutter, and a representative playable slice. Do not create a release without both technical and visual PASS evidence.
