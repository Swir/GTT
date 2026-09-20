# GTT 0.1.65 — Hill-Haul Packaged Runtime Evidence Playtest

## Purpose

This milestone turns the existing 0.1.62 hill-haul, 0.1.63 trailer-brake thermal model and 0.1.64 runaway mitigation into a fail-closed packaged evidence contract. It does not claim that a qualifying Windows run has happened.

The exact candidate must produce `FIELDMASTER_HILL_HAUL_RUNTIME.json` from the same packaged `GTT_RUNTIME.log` already used by the Native Chaos acceptance chain.

## Canonical execution path

For a real qualifying build use the `Win64 package evidence` workflow on a `[self-hosted, windows, x64, unreal-5.8]` runner. That workflow delegates to `Scripts/run_win64_attested_candidate_acceptance.ps1`; it must not maintain a second hand-written package/runtime sequence or rebuild the ZIP after attestation.

A successful technical artifact must contain the exact-candidate `WIN64_ACCEPTANCE_SUMMARY.json`, `WIN64_CANDIDATE_ATTESTATION.json`, `FIELDMASTER_HILL_HAUL_RUNTIME.json`, `NATIVE_AUTHORITY_RUNTIME.json`, `FINAL_SHA256SUMS.txt`, the packaged ZIP/hash sidecar and the five rendered review screenshots. The reviewed-release workflow independently re-hashes the sealed manifest before any human visual acceptance is recorded.

## Required environment

- Windows x64
- Unreal Engine 5.8
- clean exact Git candidate SHA
- packaged GTT Win64 build from that same SHA
- successful packaged runtime smoke
- Native Fieldmaster takeover with `NATIVE_AUTHORITY_RUNTIME.json` = PASS
- physical trailer attached to the Native Fieldmaster

## Required drive evidence

During the deterministic/qualification drive, keep a meaningful loaded trailer attached and exercise both low-speed hill hold and a sustained downhill segment.

The runtime log must capture at least:

1. two `NATIVE_FIELDMASTER_RUNTIME_TELEMETRY` samples with `trailer=ATTACHED` and `tow_load >= 0.15`;
2. one sample where `hill_hold=YES` or `downhill_tow_brake=YES`;
3. one thermal-behavior sample where brake heat is non-zero, the thermal state is not NORMAL, fade is active or cooling is active;
4. all recorded numeric values inside the production bounds;
5. if runaway mitigation activates, it may do so only with CRITICAL trailer-brake state, at least 0.50 tow load, an at-least 8-degree descent, at least 24 km/h and a positive bounded tractor-side safety brake.

A candidate is allowed to have zero runaway samples. The safety path is conditional; the evaluator verifies its invariants whenever it appears rather than forcing the driver to overheat the trailer deliberately.

## Expected evidence

`Scripts/evaluate_fieldmaster_hill_haul_runtime.ps1` must write:

`FIELDMASTER_HILL_HAUL_RUNTIME.json`

The manifest must be bound to the exact candidate SHA/version and report:

- PASS;
- clean `NATIVE_CHAOS` authority;
- loaded trailer sample count;
- hill-assist sample count;
- thermal sample count;
- runaway sample count;
- maximum observed heat/grade/speed and minimum observed trailer-brake authority.

The attested exact candidate wrapper then adds `fieldmaster_hill_haul_runtime=PASS` to the acceptance summary and the final candidate attestation hashes `FIELDMASTER_HILL_HAUL_RUNTIME.json`.

## Failure cases

The exact candidate must fail qualification when:

- build SHA differs from the requested exact candidate;
- runtime smoke or Native Chaos authority is not PASS;
- fewer than two loaded-trailer telemetry samples exist;
- no hill hold/downhill-assist sample exists;
- no trailer-brake thermal behavior is observed;
- telemetry is missing required fields or exceeds bounded production ranges;
- fade is reported below the production threshold tolerance;
- runaway mitigation appears outside the required critical/load/grade/speed envelope;
- the package lane does not use the sealed attested runner;
- the sealed integrity manifest does not reproduce every listed SHA-256 digest;
- the reviewed-release workflow cannot prove the same exact SHA/version/attestation before human approval.

## Release boundary

This milestone materially removes ambiguity from the Win64 qualification path, but it does not close any roadmap checkbox by itself. The roadmap remains 125/130 (96.2%).

A real Unreal Engine 5.8 Win64 candidate still has to package, survive the packaged EXE smoke, produce all same-SHA runtime manifests, pass authored-trailer/Native Chaos acceptance and then pass separate human visual review before any Demo Release can be authorized.
