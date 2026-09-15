# Playtest 0.0.81 — Packaged Gameplay Smoke Gate

## Purpose
Validate that a future UE 5.8 Win64 package is not promoted merely because `GTT.exe` stays alive. The candidate must produce auditable gameplay/runtime evidence from the same package and commit.

## Required automated sequence
1. Package Win64 with `package_windows.ps1`.
2. Launch the packaged `GTT.exe` unattended for at least 20 seconds and retain `GTT_RUNTIME.log` plus `RUNTIME_SMOKE.json`.
3. Run `evaluate_packaged_gameplay_smoke.ps1` against that exact log.
4. Confirm the log contains `NATIVE_CHAOS_SMOKE_READY` evidence for Rusty Fieldmaster 60, Rattleback 82 and Mulebox 1200.
5. Confirm the log has no fatal error, unhandled exception, LowLevelFatalError or assertion signature.
6. Confirm `GAMEPLAY_SMOKE.json` is `PASS` and carries the same Git SHA as the package/runtime evidence.
7. Only then evaluate the demo technical candidate.

## Diagnostic coverage
`GAMEPLAY_SMOKE.json` also records whether the packaged log exposes evidence associated with world boot, traffic, civilians/NPCs, missions and police/wanted gameplay. These fields are diagnostic in 0.0.81. They must not be reported as deterministic gameplay completion until a later milestone adds explicit runtime scenario markers that actively drive those systems.

## Demo rule
A technical PASS still does not authorize a public demo. A rendered visual review must separately verify coherent world/vehicle/character presentation, readable HUD, lighting/atmosphere and the absence of obvious placeholder clutter. No release should be created without both technical and visual acceptance.
