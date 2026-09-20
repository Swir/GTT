# GTT 0.1.61 — Win64 Candidate Qualification & Authored Trailer Import

This milestone closes a release-pipeline gap without pretending that a Windows candidate has already passed. The canonical Win64 evidence lane must now import and validate the project-owned authored trailer inside Unreal Engine 5.8 **before** packaging, bind that import to the exact commit/version, and carry the evidence through the reviewed-release gate. It also provides fail-closed local/runner commands so the same technical candidate can be qualified on a Windows 11 + UE 5.8 workstation, then sealed with exact-candidate file hashes before human visual review.

## Source / CI acceptance

Run:

```bash
python Scripts/verify_v0_1_61_win64_candidate_pipeline.py
python Scripts/verify_win64_candidate_acceptance_runner.py
python Scripts/verify_v0_1_61_candidate_attestation.py
python Scripts/verify_win64_evidence_pipeline.py
```

Expected results include:

`GTT 0.1.61 Win64 candidate pipeline sanity: PASS ...`

`GTT 0.1.61 Win64 candidate acceptance runner sanity: PASS ...`

`GTT 0.1.61 candidate attestation sanity: PASS ...`

The source verifiers must prove all of the following:

- `Config/DefaultGame.ini` declares `ProjectVersion=0.1.61`.
- Win64 package evidence requires an explicit version and rejects a value that differs from `ProjectVersion`.
- Direct `package_windows.ps1` use resolves the canonical `ProjectVersion` when `-Version` is omitted and fails closed on a mismatched explicit version.
- The canonical package job still requires `[self-hosted, windows, x64, unreal-5.8]`.
- UE 5.8 preflight runs before authored-trailer import.
- `Scripts/import_gtt_farm_trailer_unreal.ps1` runs before `package_windows.ps1`.
- The final `SK_GTT_FarmTrailer.uasset` must physically exist before packaging starts.
- `AUTHORED_TRAILER_IMPORT.json` is bound to the exact commit SHA and version and is copied into the package evidence bundle.
- Runtime smoke, Native Chaos/drivetrain/trailer evaluators, rendered visual evidence and final artifact hashing remain after the import gate.
- `Scripts/run_win64_candidate_acceptance.ps1` uses the same exact-SHA/version boundaries, requires a clean tracked tree, runs the full technical/rendered acceptance route, and writes `WIN64_ACCEPTANCE_SUMMARY.json`.
- `Scripts/run_win64_attested_candidate_acceptance.ps1` runs the base acceptance first, validates that summary, then seals the final package through `Scripts/write_win64_candidate_attestation.ps1`.
- `WIN64_CANDIDATE_ATTESTATION.json` binds the exact SHA/version/configuration to the packaged `GTT.exe`, critical technical/runtime/rendered evidence and exactly five screenshots using SHA-256.
- `FINAL_SHA256SUMS.txt` is generated **after** attestation and all runtime/rendered evidence, so it supersedes the earlier package-time hash snapshot for final-candidate integrity.
- The attested path rebuilds the ZIP and `.sha256` sidecar only after final hashes are sealed.
- The local/runner acceptance summary and attestation keep `human_visual_review=REQUIRED` and `demo_release_authorized=false`; neither can replace the separate reviewed-release gate.
- The dedicated attested workflow requires `[self-hosted, windows, x64, unreal-5.8]`, never auto-releases, and uploads only the exact candidate ZIP/hash, summary, attestation, final hash manifest, gate, visual evidence and five screenshots.
- The reviewed release workflow requires the exact candidate run/SHA, the import manifest, visual evidence and explicit visual approval.
- Stale hard-coded package/release version defaults and the stale fixed gameplay-step count are rejected.

## One-command Windows qualification

On a Windows x64 machine with Unreal Engine 5.8, Visual Studio C++ tools, Windows SDK, Git LFS and sufficient free disk, the exact checked-out candidate can run the full technical route without first registering that machine as a GitHub Actions runner:

```powershell
pwsh ./Scripts/run_win64_candidate_acceptance.ps1 `
  -EngineRoot "C:\Program Files\Epic Games\UE_5.8" `
  -Configuration Shipping
```

`-Version` is optional and resolves from `Config/DefaultGame.ini`; a supplied mismatch fails. The command fails if the tracked working tree is dirty, binds runtime evidence to the exact 40-character Git SHA, imports the authored trailer, packages Win64, runs the long deterministic packaged smoke/evaluators, promotes the technical gate through schema 17, captures and validates five rendered screenshots, and creates the ZIP/hash plus `WIN64_ACCEPTANCE_SUMMARY.json`.

For the **sealed candidate** used for transfer/upload and subsequent human review, use the attested wrapper instead:

```powershell
pwsh ./Scripts/run_win64_attested_candidate_acceptance.ps1 `
  -EngineRoot "C:\Program Files\Epic Games\UE_5.8" `
  -Configuration Shipping
```

That wrapper performs the same complete acceptance route, then writes `WIN64_CANDIDATE_ATTESTATION.json`, generates `FINAL_SHA256SUMS.txt` after all evidence exists, and rebuilds the candidate ZIP plus its SHA-256 sidecar. A PASS from either route is **technical qualification for human visual review**, not publication permission; the attested route is the preferred final candidate-transfer path.

## GitHub self-hosted attested qualification

When a qualifying Windows/UE 5.8 runner is registered with labels `self-hosted`, `windows`, `x64`, `unreal-5.8`, dispatch:

`GTT 0.1.61 Win64 attested candidate acceptance`

The workflow deliberately has no automatic fallback to an Ubuntu runner and no release-publishing step. If no qualifying runner exists, the job must remain queued rather than produce a false green result. If it runs, the checkout SHA, project version, configuration, acceptance summary and attestation must all match before artifact upload.

## Real Unreal Engine 5.8 / Win64 acceptance

On the qualifying Windows machine or self-hosted runner, a candidate is **not** accepted unless the same exact commit proves:

1. UE 5.8 runner/workstation preflight PASS.
2. Authored trailer Interchange import PASS.
3. Final skeletal trailer asset exists and importer validation confirms required bones, runtime sockets and PhysicsAsset.
4. Win64 package succeeds from the same workspace and exact commit.
5. Packaged EXE remains alive for the configured 472-second deterministic smoke window.
6. Native Chaos, drivetrain and authored-trailer runtime evidence PASS for the exact SHA.
7. Farm Cargo/workshop continuity evidence and schema-17 technical gate PASS.
8. Rendered five-scene visual evidence PASS without NullRHI.
9. `WIN64_ACCEPTANCE_SUMMARY.json` is exact-candidate bound and keeps human review required.
10. `WIN64_CANDIDATE_ATTESTATION.json` hashes the packaged EXE and critical final evidence for that same SHA/version/configuration.
11. `FINAL_SHA256SUMS.txt`, rebuilt ZIP and ZIP SHA-256 sidecar are created only after the final evidence/attestation stage.

Only after screenshots from that exact candidate are reviewed may the separate reviewed-release workflow be run with `visual_review_passed=true`.

## Failure tests

The candidate lane must fail closed when:

- version input is empty where required or differs from `ProjectVersion`;
- a direct package helper version differs from `ProjectVersion`;
- the local acceptance tree contains tracked modifications;
- the exact Git SHA cannot be resolved;
- UE 5.8 is unavailable;
- authored trailer import fails;
- the final trailer `.uasset` is missing after import;
- import evidence SHA/version does not match the package candidate;
- any packaged runtime/evidence evaluator fails;
- the technical gate is not PASS schema 17;
- the rendered visual evidence set is incomplete or not PASS;
- packaged `GTT.exe` is missing/ambiguous;
- summary/build/runtime/visual evidence SHA, version or configuration disagrees with the candidate;
- any critical evidence file is missing before attestation;
- final evidence changes after a digest is sealed;
- the release workflow is pointed at a non-main run, different SHA, different version or unreviewed visuals.

## Roadmap truth

Roadmap completion stays **125 / 130 (96.2%)**. This milestone materially reduces the remaining Windows acceptance blocker by making the exact candidate executable and cryptographically bindable to its final evidence on a qualifying runner, but it does not itself prove that a qualifying UE 5.8 Windows machine executed successfully, does not close Native Chaos/trailer/Win64 checkboxes, and does not authorize a Demo Release.
