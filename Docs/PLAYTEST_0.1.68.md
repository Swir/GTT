# GTT 0.1.68 — sealed Win64 archive integrity

This milestone hardens the final hand-off between the exact-candidate Unreal acceptance pipeline and the artifact a tester would actually download. The project still requires a real qualifying UE 5.8 Win64 run; 0.1.68 does not manufacture or substitute that evidence.

## What changed

`Scripts/verify_win64_candidate_archive.ps1` now re-opens the final candidate ZIP after `write_win64_candidate_attestation.ps1` creates it. The verifier checks the ZIP SHA-256 sidecar, rejects unsafe archive paths, extracts the artifact to a fresh temporary directory, and requires the extracted file set to match `FINAL_SHA256SUMS.txt` exactly.

Every manifest entry is re-hashed from the extracted archive. The verifier then checks the exact candidate SHA/version/configuration, Win64 + Unreal Engine 5.8 identity, the attestation evidence-file count, each attested evidence file's hash and byte count, and the single packaged `GTT.exe` path/hash. It keeps `human_visual_review=REQUIRED` and `demo_release_authorized=false`.

A successful round-trip writes `GTT-<version>-Windows-<configuration>.zip.verify.json` beside the sealed ZIP. That verification record is deliberately external to the ZIP so verifying the candidate does not mutate the candidate it just verified.

## Deterministic contract tests

`Scripts/test_win64_candidate_archive_verifier.ps1` exercises the actual PowerShell verifier on `windows-latest` with:
1. a positive sealed fixture,
2. a wrong ZIP sidecar hash,
3. an extra unmanifested archive file,
4. a forged human-review / Demo Release authorization boundary.

The three negative fixtures must fail closed. `Scripts/verify_v0_1_68_win64_archive_integrity.py` additionally pins integration ordering, workflow upload contents, exact-candidate identity, SWIR progress truth and the five still-open runtime/art gates.

## Required real Windows playtest

1. Dispatch the qualifying **GTT Win64 attested candidate acceptance** workflow on a self-hosted Windows x64 runner labelled `unreal-5.8`.
2. Use version `0.1.68`, Shipping configuration, and the exact checkout SHA being qualified.
3. Require base packaging/runtime/native/trailer/HUD evidence to pass before the attestor creates the final manifest and ZIP.
4. Require `*.zip.verify.json` to be `PASS` for the same SHA/version/configuration and for its `archive_sha256` to match the uploaded ZIP.
5. Perform the separate human visual review on the exact candidate screenshots. No source or archive verifier may convert that human gate into PASS automatically.

## Gate boundary

0.1.68 does not close any of the five remaining ROADMAP gates. A source-level archive verifier is release engineering hardening, not proof that UE 5.8 compiled the project, that the packaged EXE passed runtime smoke, that Native Chaos/authored trailer acceptance passed on real content, or that a qualifying Win64 runner has produced evidence. The roadmap therefore remains 125/130 (96.2%) until those real gates are satisfied.
