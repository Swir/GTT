# GTT 0.0.80 Playtest — Win64 Demo Evidence Gate

## Goal

Turn a genuine UE 5.8 Win64 package into auditable demo-candidate evidence without treating source CI, a process-alive check, or an unreviewed screenshot as a public-demo approval.

## Technical candidate procedure

1. Run `Win64 package evidence` on a self-hosted Windows x64 runner labelled `unreal-5.8`.
2. Package the exact `main` commit in Shipping configuration and retain `BUILD_INFO.json`.
3. Launch the packaged `GTT.exe`; `RUNTIME_SMOKE.json` must report `PASS`.
4. Exercise Fieldmaster, Rattleback 82 and Mulebox 1200 long enough for each to satisfy the 0.0.79 acceptance matrix and emit `NATIVE_CHAOS_SMOKE_READY` in the packaged runtime log.
5. Run `evaluate_demo_candidate.ps1`. It must reject a SHA mismatch, missing runtime smoke, missing runtime log, or missing fleet evidence.
6. Preserve `DEMO_TECHNICAL_GATE.json`, runtime log, package manifests, ZIP and SHA256 in the workflow artifact.

## Visual acceptance

Technical success is not public-demo readiness. Review the same packaged build rendered normally (not `-nullrhi`) and create `DEMO_VISUAL_ACCEPTANCE.json` only after confirming coherent world/vehicle/character presentation, readable HUD, lighting/atmosphere, no wall-of-text placeholder clutter, and a stable slice containing traffic/NPC/mission/combat/vehicle play.

The visual manifest must contain at least `result: PASS`, a non-empty `reviewer`, and `reviewed_utc`. Re-run the candidate evaluator with `-RequireVisual`; only that combined gate is suitable as input to a future Demo Release step.

## Negative tests

- Change `ExpectedGitSha`: gate must fail.
- Remove one `NATIVE_CHAOS_SMOKE_READY` vehicle line: gate must fail.
- Set runtime smoke result to anything other than `PASS`: gate must fail.
- Invoke `-RequireVisual` without the visual manifest: gate must fail.

## Demo status

Do not publish a Release from repository/source sanity alone. Until a real UE 5.8 Win64 runner executes this procedure and the rendered visual review passes, demo status remains blocked.
