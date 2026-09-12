# GTT 0.0.27 — Release Pipeline Playtest

This milestone hardens the path from source to a verifiable Windows release artifact. It does not claim a packaged EXE was runtime-tested unless the Unreal-equipped Windows workflow actually runs.

## Repository sanity

1. Run `python Scripts/verify_release_pipeline.py`.
2. Confirm it reports exactly `118/130 (90.8%)` and an 18/20 SWIR progress bar.
3. Run the complete `Project sanity` workflow and confirm every existing milestone regression still passes.

## Local Unreal packaging

On a Windows machine with UE 5.8 and the required C++ toolchain:

```powershell
./Scripts/package_windows.ps1 -Configuration Shipping -Version 0.0.27
```

Confirm Unreal Automation Tool completes build, cook, stage, pak/IoStore, prerequisites and archive without error.

## Package validation

Confirm the release directory contains:

- exactly one `GTT.exe`;
- cooked `.pak`, `.utoc` or `.ucas` data;
- `BUILD_INFO.json`;
- `PACKAGE_VALIDATION.json`;
- `SHA256SUMS.txt`.

For Shipping builds confirm no `.pdb`, `.lib` or `.exp` files remain in the distributable package.

## Integrity / artifact test

1. Confirm `<release>.zip` and `<release>.zip.sha256` exist.
2. Recompute SHA256 after copying/downloading the ZIP and compare it with the sidecar checksum.
3. Extract the ZIP to a clean directory and confirm the internal `SHA256SUMS.txt` hashes still match.

## Runtime smoke test boundary

When an Unreal-equipped runner or local build machine is available, launch the packaged EXE and verify:

1. title/startup reaches gameplay without editor dependencies;
2. Player Farm loads and HUD appears;
3. enter/drive/exit Fieldmaster, Rattleback and Mulebox;
4. quick save/load survives a restart;
5. trigger wanted police and ranger gameplay;
6. complete one legal job and one hostile encounter;
7. radio/settings/controller paths remain functional;
8. quit cleanly and relaunch once.

Until that runtime pass is actually executed, report repository CI as verified but the full Win64 compile/package/smoke result as **not verified**.
