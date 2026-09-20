# GTT 0.1.61 — Win64 Candidate Qualification & Authored Trailer Import

This milestone closes a release-pipeline gap without pretending that a Windows candidate has already passed. The canonical Win64 evidence lane must now import and validate the project-owned authored trailer inside Unreal Engine 5.8 **before** packaging, bind that import to the exact commit/version, and carry the evidence through the reviewed-release gate.

## Source / CI acceptance

Run:

```bash
python Scripts/verify_v0_1_61_win64_candidate_pipeline.py
```

Expected result:

`GTT 0.1.61 Win64 candidate pipeline sanity: PASS ...`

The verifier must prove all of the following:

- `Config/DefaultGame.ini` declares `ProjectVersion=0.1.61`.
- Win64 package evidence requires an explicit version and rejects a value that differs from `ProjectVersion`.
- The canonical package job still requires `[self-hosted, windows, x64, unreal-5.8]`.
- UE 5.8 preflight runs before authored-trailer import.
- `Scripts/import_gtt_farm_trailer_unreal.ps1` runs before `package_windows.ps1`.
- The final `SK_GTT_FarmTrailer.uasset` must physically exist before packaging starts.
- `AUTHORED_TRAILER_IMPORT.json` is bound to the exact `GITHUB_SHA` and version and is copied into the package evidence bundle.
- Runtime smoke, Native Chaos/drivetrain/trailer evaluators, rendered visual evidence and final artifact hashing remain after the import gate.
- The reviewed release workflow requires the exact candidate run/SHA, the import manifest, visual evidence and explicit visual approval.
- Stale hard-coded package/release version defaults and the stale fixed gameplay-step count are rejected.

## Real Unreal Engine 5.8 / Win64 acceptance

On the qualifying Windows runner, dispatch **Win64 package evidence** only after this branch is integrated to `main`, using the exact `ProjectVersion`.

A candidate is **not** accepted unless the same run proves:

1. UE 5.8 runner preflight PASS.
2. Authored trailer Interchange import PASS.
3. Final skeletal trailer asset exists and importer validation confirms required bones, runtime sockets and PhysicsAsset.
4. Win64 package succeeds from the same workspace and exact commit.
5. Packaged EXE remains alive for the configured deterministic smoke window.
6. Native Chaos, drivetrain and authored-trailer runtime evidence PASS for the exact SHA.
7. Farm Cargo/workshop continuity evidence and technical gate PASS.
8. Rendered five-scene visual evidence PASS without NullRHI.
9. Final ZIP/hash artifact is produced from that same package.

Only after screenshots from that exact candidate are reviewed may the separate reviewed-release workflow be run with `visual_review_passed=true`.

## Failure tests

The package lane must fail closed when:

- version input is empty or differs from `ProjectVersion`;
- UE 5.8 is unavailable;
- authored trailer import fails;
- the final trailer `.uasset` is missing after import;
- import evidence SHA/version does not match the package candidate;
- any packaged runtime/evidence evaluator fails;
- the rendered visual evidence set is incomplete;
- the release workflow is pointed at a non-main run, different SHA, different version or unreviewed visuals.

## Roadmap truth

Roadmap completion stays **125 / 130 (96.2%)**. This milestone makes the candidate pipeline capable of collecting the missing evidence; it does not itself prove that the self-hosted Unreal runner executed successfully, does not close Native Chaos/trailer/Win64 checkboxes, and does not authorize a Demo Release.
