# GTT 0.1.14 Playtest — Native Chaos Runtime Acceptance & Win64 Build Path

This milestone hardens the boundary between repository/source confidence and a genuinely verified Unreal Engine 5.8 Windows build. Source CI can prove the contract exists; only the real Win64 runner can produce the evidence required to close runtime/demo gates.

## A. Runner preflight

1. On the intended self-hosted Windows x64 runner, execute:
   `./Scripts/preflight_win64_unreal.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.8"`.
2. Confirm the command exits `0` and writes `WIN64_PREFLIGHT.json`.
3. Confirm `result` is `PASS`, `evidence_schema` is `2`, engine version is 5.8.x and project EngineAssociation is 5.8.
4. Confirm required checks pass for `RunUAT.bat`, `UnrealEditor-Cmd.exe`, UnrealBuildTool, ChaosVehiclesPlugin, GTT Runtime module, Git LFS, MSVC, Windows SDK and free disk.
5. Negative test: pass a nonexistent engine root. The script must still write evidence, return a non-zero exit, and mark the missing required checks as failed.
6. Negative test: run on a non-Windows host. The contract must fail rather than pretending that a Win64 package was accepted.

## B. Compile/cook/package evidence

7. Run `package_windows.ps1` for Shipping 0.1.14 on the UE 5.8 runner.
8. Confirm preflight completes before the archive directory is recreated.
9. Confirm a successful UAT `BuildCookRun` produces `BUILD_ATTEMPT.json` with `result=PASS`, `uat_exit_code=0` and the exact Git SHA.
10. Force a controlled UAT failure on a disposable branch/configuration and confirm the external `.attempt.json` remains available with `result=FAIL` and an error message.
11. Confirm successful packages contain `WIN64_PREFLIGHT.json`, `BUILD_ATTEMPT.json`, `BUILD_INFO.json`, `PACKAGE_VALIDATION.json` and `SHA256SUMS.txt`.
12. Delete or corrupt either preflight/build-attempt file in a copied package and verify `validate_windows_package.ps1` rejects it.

## C. Packaged runtime acceptance

13. Run the `Win64 package evidence` workflow for the exact candidate commit. It must package, start the packaged `GTT.exe`, remain alive for the required smoke window and write `RUNTIME_SMOKE.json`.
14. Confirm deterministic scenario and gameplay smoke produce `DEMO_SCENARIO.json` and `GAMEPLAY_SMOKE.json`.
15. Confirm `DEMO_TECHNICAL_GATE.json` references the same source commit and no runtime evidence comes from a different build.
16. Enter the native Fieldmaster and verify forward, reverse, steering, braking, fuel-zero stop, damaged-condition power loss, tire/mud steering response, trailer load, collision damage and workshop recovery. This is runtime evidence for the dedicated native Chaos movement path; source sanity alone is not enough.
17. Save/load and garage recall must not strand a stale throttle/gear command; the legacy mirror may remain only as the declared persistence/fallback boundary.
18. Confirm wanted/ranger interactions, ROAD/CARGO contracts and a short traffic encounter still work after the native-vehicle run.

## D. Visual acceptance and Demo gate

19. Perform visual acceptance on the exact packaged candidate, not PIE/source-only output.
20. Verify world lighting, vehicle presentation, player/NPC presentation, weapon/combat readability, HUD/UI density, traffic/NPC population and mission readability are suitable to show publicly.
21. Capture screenshots/preview only from the exact accepted package.
22. A Demo Release remains blocked unless all of these are simultaneously true: green source CI, PASS `WIN64_PREFLIGHT.json`, PASS `BUILD_ATTEMPT.json`, package validation, PASS `RUNTIME_SMOKE.json`, deterministic/gameplay smoke, PASS technical gate, and completed visual acceptance.
23. If any gate fails, upload/use the failure diagnostics and fix the defect; do not create a release from partial evidence.

## Expected repository-only result

On GitHub-hosted source CI, `Scripts/verify_win64_evidence_pipeline.py` must pass and the roadmap must remain at the truthful 125/130 (96.2%) state. Repository CI does **not** count as a UE 5.8 compile, packaged runtime smoke or visual acceptance.
