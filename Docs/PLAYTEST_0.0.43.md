# GTT 0.0.43 — Win64 Runtime Evidence Gate

This milestone turns the existing Windows packaging helpers into a repeatable evidence pipeline for the first real packaged demo candidate. It does not claim that the repository already owns or has executed on an Unreal-capable Windows runner.

## Automated acceptance path

1. Provision a self-hosted Windows x64 GitHub Actions runner with Unreal Engine 5.8 installed and labels `self-hosted`, `windows`, `x64`, `unreal-5.8`.
2. Run **Win64 package evidence** manually and provide the actual UE 5.8 root if it differs from the default.
3. The workflow must complete `BuildCookRun` for Win64, run `validate_windows_package.ps1`, launch the packaged `GTT.exe`, and require it to remain alive for at least 20 seconds.
4. Confirm the uploaded artifact contains the release ZIP plus `BUILD_INFO.json`, `PACKAGE_VALIDATION.json`, `RUNTIME_SMOKE.json` and the ZIP SHA-256 file.
5. Confirm `RUNTIME_SMOKE.json` reports `result: PASS`, the exact commit SHA, runner name and `visual_acceptance: NOT_PERFORMED`.

## Native Fieldmaster checks on the same build

Before either Native Chaos roadmap checkbox is closed, test the authored Rusty Fieldmaster 60 in the packaged build:

- forward/reverse launch and braking,
- steering at low and road speed,
- suspension travel on road edges and mud terrain,
- no double-simulation with the legacy dynamics path,
- fuel/condition/tire/tuning influence still comes from shared gameplay state,
- enter/exit, theft heat, wanted response, garage persistence and save/load,
- trailer attach/detach through the validated `rear_hitch` socket.

## Visual demo gate

The automated smoke uses `-nullrhi` deliberately: it proves the packaged executable survives startup without pretending that visuals were reviewed. A public demo additionally requires a normal rendered launch and human visual acceptance at 1080p (and a quick 720p readability check):

- vehicle/world materials and lighting look coherent,
- HUD remains readable and uncluttered,
- no obvious placeholder/debug walls dominate the scene,
- night driving, vehicle lighting and village presentation look intentional,
- combat/NPC/mission/vehicle slice is presentable enough to represent GTT publicly.

Record screenshots or video separately when that rendered visual pass is performed. Do not convert a headless smoke PASS into visual approval.

## Roadmap honesty

0.0.43 intentionally leaves `Full Unreal compile + packaged Win64 smoke test`, `Full Win64 CI/build runner`, and both Native Chaos runtime tasks open until this workflow has actually run successfully on a real UE 5.8 Windows runner and the relevant runtime checks have been observed.
