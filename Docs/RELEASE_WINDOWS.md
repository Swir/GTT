# GTT Windows Release Pipeline

GTT 0.0.27 hardens the repository path from source to a distributable Windows archive. The repository still does **not** claim a verified packaged build unless the workflow actually runs on a machine with Unreal Engine 5.8 installed.

## Local release build

Requirements:

- Windows 10/11 x64
- Unreal Engine 5.8 at `C:\Program Files\Epic Games\UE_5.8` or a custom `-EngineRoot`
- Visual Studio 2022 C++ game-development toolchain required by Unreal
- Git LFS assets present

Example Shipping package:

```powershell
./Scripts/package_windows.ps1 -Configuration Shipping -Version 0.0.27
```

The script runs Unreal Automation Tool `BuildCookRun` for Win64 with build, cook, stage, pak/IoStore, prerequisites and archive enabled. It then validates the package, writes metadata and creates a compressed release archive.

## Produced release metadata

A successful run creates:

- `BUILD_INFO.json` — version, configuration, engine label, Git SHA and UTC build time.
- `PACKAGE_VALIDATION.json` — detected executable, cooked container count and package size.
- `SHA256SUMS.txt` — SHA-256 for every file inside the uncompressed package.
- `<release>.zip` — compressed distributable package.
- `<release>.zip.sha256` — checksum for the ZIP itself.

The validation stage rejects packages that do not contain exactly one `GTT.exe`, do not contain cooked `.pak`/`.utoc`/`.ucas` data, are suspiciously small, or contain PDB/LIB/EXP linker/debug artifacts in Shipping mode.

## GitHub Actions release workflow

`.github/workflows/release-windows.yml` is an explicit `workflow_dispatch` pipeline. It requires a project-controlled runner with all of these labels:

```text
self-hosted, Windows, X64, unreal-5.8
```

The job checks out Git LFS data, runs the complete repository sanity suite, verifies `RunUAT.bat`, packages and validates Win64, then uploads the ZIP/checksums/manifests as a GitHub Actions artifact.

This workflow is intentionally **not** assigned to `windows-latest`: GitHub-hosted Windows runners do not provide the licensed Unreal Engine 5.8 installation/content required by this project. Until an eligible self-hosted runner is connected and a complete build is executed, the roadmap item `Full Win64 CI/build runner` remains open.

## Release acceptance gate

Before calling a build verified:

1. Project sanity must be green for the exact commit.
2. The Windows release workflow must finish successfully for the exact commit/version.
3. `PACKAGE_VALIDATION.json` and `SHA256SUMS.txt` must exist.
4. The ZIP checksum must match after download.
5. A human smoke test must launch the packaged `GTT.exe`, start a sandbox session, drive a vehicle, save/load, trigger wanted/ranger gameplay and quit cleanly.

The repository-side pipeline automates steps 1–4 once an Unreal-equipped runner is available. Step 5 remains an explicit runtime acceptance test.
