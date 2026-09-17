# GTT Windows Release Pipeline

GTT 0.1.15 keeps the 0.1.14 fail-fast Win64 build forensics and adds a runtime proof layer for the dedicated Rusty Fieldmaster 60 Native Chaos drivetrain, wheel contacts and suspension. The repository still does **not** claim a verified packaged build unless the workflow actually runs on a Windows x64 machine with Unreal Engine 5.8 and the packaged executable passes all runtime evidence gates.

## Local release build

Requirements:

- Windows 10/11 x64.
- Unreal Engine 5.8 at `C:\Program Files\Epic Games\UE_5.8` or a custom `-EngineRoot`.
- Visual Studio 2022 C++ game-development toolchain required by Unreal.
- Windows 10/11 SDK.
- Git + Git LFS with project assets present.
- At least 25 GiB free on the evidence/output drive by default.

Run the preflight first when preparing a runner:

```powershell
./Scripts/preflight_win64_unreal.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.8"
```

A passing run writes `Saved\Win64\WIN64_PREFLIGHT.json`. It checks the Windows host, UE root/version, RunUAT, UnrealEditor-Cmd, UnrealBuildTool, project EngineAssociation, Chaos Vehicles plugin, GTT runtime module, Git LFS, MSVC, Windows SDK and disk capacity. A failed preflight also writes JSON evidence before returning non-zero.

Example Shipping package:

```powershell
./Scripts/package_windows.ps1 -Configuration Shipping -Version 0.1.15
```

The package helper re-runs preflight before touching the target archive, then runs Unreal Automation Tool `BuildCookRun` for Win64 with build, cook, stage, pak/IoStore, prerequisites and archive enabled. It captures the UAT attempt even on failure. A success is validated and hashed.

## Produced build and failure evidence

A successful package contains:

- `WIN64_PREFLIGHT.json` — schema-v2 host, UE 5.8, toolchain, project and capacity gate.
- `BUILD_ATTEMPT.json` — schema-v2 UAT start/completion/result, exit code, source Git SHA and error field.
- `BUILD_INFO.json` — version, configuration, engine label, source SHA and UTC build time.
- `PACKAGE_VALIDATION.json` — executable, cooked containers, package size and linked preflight/UAT status.
- `SHA256SUMS.txt` — SHA-256 for every file present at package-validation time.
- `<release>.zip` and `<release>.zip.sha256` when ZIP creation is enabled.

Before the archive exists, external `<archive>.preflight.json` and `<archive>.attempt.json` files preserve the most useful forensic state. The GitHub workflows upload these as failure diagnostics when an eligible runner reaches the job but a required stage fails.

The package validator rejects builds that do not carry PASS schema-v2 preflight and build-attempt evidence, do not contain exactly one `GTT.exe`, do not contain cooked `.pak`/`.utoc`/`.ucas` data, are suspiciously small, or contain PDB/LIB/EXP artifacts in Shipping mode.

## Packaged Native Chaos telemetry

The technical-candidate route launches the packaged executable with `-GTTDemoSmokeScenario`. During that run the deterministic scenario drives the Rusty Fieldmaster 60 through `UGTTFieldmasterChaosMovementComponent::ApplyFieldmasterDriveCommand` and logs live Chaos state rather than bypassing the dedicated tractor authority.

`Scripts/evaluate_fieldmaster_chaos_telemetry.ps1` parses those observations and writes `FIELDMASTER_CHAOS_TELEMETRY.json` using schema `gtt.fieldmaster-chaos-telemetry.v1`. A PASS requires, on the exact candidate SHA:

- multiple live observations from an active, valid dedicated Fieldmaster movement component;
- a driven sample proving non-zero gear, engine RPM, effective throttle and measurable speed;
- four valid Chaos wheels with at least two ground contacts;
- four finite normalized suspension samples in the 0..1 range and positive spring force while grounded;
- non-zero live wheel drive torque;
- bounded/finitely-valued control, health and grip data.

This manifest is runtime evidence only when it was produced by the packaged UE 5.8 Win64 executable. Source-contract CI merely verifies that the evidence path cannot silently disappear.

## GitHub Actions paths

`.github/workflows/win64-package-evidence.yml` is the technical-candidate pipeline. It requires:

```text
self-hosted, windows, x64, unreal-5.8
```

It runs preflight, package, packaged EXE runtime smoke, Fieldmaster Native Chaos telemetry evaluation, deterministic scenario evidence, gameplay smoke, a second package validation and the demo technical gate. A success artifact includes `WIN64_PREFLIGHT.json`, `BUILD_ATTEMPT.json`, `RUNTIME_SMOKE.json`, `FIELDMASTER_CHAOS_TELEMETRY.json`, `DEMO_SCENARIO.json`, `GAMEPLAY_SMOKE.json`, runtime log and `DEMO_TECHNICAL_GATE.json`.

`.github/workflows/release-windows.yml` is the explicit release-package pipeline. It uses the same preflight and package evidence contract. It must not be treated as a substitute for the technical/runtime candidate gate.

`.github/workflows/win64-runtime-acceptance-sanity.yml` is intentionally source-only. For 0.1.15 it verifies the dedicated Fieldmaster telemetry contract plus the Win64 evidence route; it does not compile Unreal.

GitHub-hosted `windows-latest` runners do not provide this project's licensed Unreal Engine 5.8 installation/content. Until an eligible self-hosted runner is connected and actually executes the UE workflows, `Full Win64 CI/build runner` remains open.

## Release acceptance gate

Before calling a Windows build technically verified, all evidence must refer to the exact candidate commit:

1. Project/source sanity green.
2. `WIN64_PREFLIGHT.json` PASS on the real Windows/UE 5.8 runner.
3. `BUILD_ATTEMPT.json` PASS with UAT exit code 0.
4. `PACKAGE_VALIDATION.json` PASS plus valid hashes/archive.
5. `RUNTIME_SMOKE.json` PASS on packaged `GTT.exe`.
6. `FIELDMASTER_CHAOS_TELEMETRY.json` PASS, proving the dedicated tractor drivetrain/wheel/suspension runtime path.
7. Deterministic scenario and packaged gameplay smoke PASS.
8. `DEMO_TECHNICAL_GATE.json` PASS and bound to the candidate SHA.

A **Demo Release** has one additional non-automated requirement: visual acceptance of the exact packaged candidate. World/vehicle/character presentation, HUD/UI, lighting, NPC/traffic density and the playable mission/combat/vehicle slice must be good enough to represent the intended game publicly. Source CI, a preflight PASS, telemetry source checks, or even a compile by itself is not visual acceptance.

See `Docs/PLAYTEST_0.1.15.md` for the current runtime and visual acceptance matrix.
