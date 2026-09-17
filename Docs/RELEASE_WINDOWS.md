# GTT Windows Release Pipeline

GTT uses a two-stage Windows Demo Release process. A public release is never built after visual review: first a qualifying Unreal Engine 5.8 runner produces one exact packaged candidate plus technical and rendered evidence; only that already-produced ZIP can later be approved and published.

The repository does **not** claim a verified packaged build unless the candidate workflow actually runs on a Windows x64 machine with Unreal Engine 5.8 and the packaged executable passes the runtime evidence gates.

## Local release build

Requirements:

- Windows 10/11 x64.
- Unreal Engine 5.8 at `C:\Program Files\Epic Games\UE_5.8` or a custom `-EngineRoot`.
- Visual Studio 2022 C++ game-development toolchain required by Unreal.
- Windows 10/11 SDK.
- Git + Git LFS with project assets present.
- At least 25 GiB free on the evidence/output drive by default.
- A working rendered RHI for the visual-evidence pass; NullRHI is deliberately rejected for screenshots.

Run the preflight first:

```powershell
./Scripts/preflight_win64_unreal.ps1 -EngineRoot "C:\Program Files\Epic Games\UE_5.8"
```

A passing run writes `Saved\Win64\WIN64_PREFLIGHT.json`. Example Shipping package:

```powershell
./Scripts/package_windows.ps1 -Configuration Shipping -Version 0.1.20
```

## Stage 1 — exact-SHA candidate evidence

`.github/workflows/win64-package-evidence.yml` is the only authoritative candidate producer. It requires `self-hosted, windows, x64, unreal-5.8` and runs preflight; UBT/UAT build/cook/package; package validation; the 178-second packaged technical route; the 33-step gameplay route; Native Chaos telemetry; deterministic forward/reverse drivetrain; loaded authored-trailer runtime; and `DEMO_TECHNICAL_GATE.json`.

It then launches the **same packaged executable a second time without NullRHI** using `-RenderOffscreen` at 1280×720. `UGTTDemoVisualEvidenceSubsystem` captures five deterministic frames: world/gameplay, law pressure, Native vehicle, loaded trailer and final HUD overview. `evaluate_demo_visual_evidence.ps1` verifies the PNGs, resolution/size, non-flat luminance, sampled color variation, runtime markers and frame diversity before writing `DEMO_VISUAL_EVIDENCE.json`.

These automated checks detect broken rendering/capture. They are not aesthetic approval; the manifest deliberately records `human_review_required=true`.

A successful artifact contains the original candidate ZIP + SHA256, all technical evidence, `VISUAL_RUNTIME_SMOKE.json`, `DEMO_VISUAL_EVIDENCE.json`, `GTT_VISUAL_RUNTIME.log`, and the five screenshots.

## Stage 2 — review the exact packaged candidate

Before publication, download the Stage 1 artifact and inspect all five screenshots at full size. Reject the candidate if the world, vehicles, characters/weapons, traffic/NPCs, lighting/atmosphere, HUD/UI, mission/combat/vehicle slice, placeholders/debug clutter or general presentation contains a demo-critical problem.

When those exact screenshots are good enough to represent GTT publicly, run `.github/workflows/release-windows.yml` with the exact `version`, successful `candidate_run_id`, full 40-character `expected_sha`, `visual_review_passed=true`, meaningful `visual_review_notes`, and `publish_release=true` only when publication is intended.

The publication workflow runs on `windows-latest` because it **does not rebuild Unreal**. It verifies through the GitHub Actions API that `candidate_run_id` is a successful `Win64 package evidence` run from `main` at exactly `expected_sha`, downloads that already-produced artifact, validates the candidate ZIP SHA256, and expands it only for gate re-evaluation.

`write_demo_visual_acceptance.ps1` writes `DEMO_VISUAL_ACCEPTANCE.json`, binding the authenticated workflow actor, notes, exact Git SHA, rendered-evidence manifest SHA256 and all reviewed screenshot hashes. The full candidate gate is then rerun with `evaluate_demo_candidate.ps1 -RequireVisual`. The publication workflow never invokes preflight, UBT/UAT or `package_windows.ps1`, so it cannot silently replace the reviewed binary.

If `publish_release=true`, the **original candidate ZIP** and `.sha256` are published as a GitHub prerelease together with final visual acceptance/gate evidence and the reviewed screenshots.

## Authoritative evidence bundle

- `WIN64_PREFLIGHT.json`
- `BUILD_ATTEMPT.json`
- `BUILD_INFO.json`
- `PACKAGE_VALIDATION.json`
- `SHA256SUMS.txt`
- `RUNTIME_SMOKE.json`
- `DEMO_SCENARIO.json`
- `GAMEPLAY_SMOKE.json`
- `NATIVE_CHAOS_RUNTIME.json`
- `NATIVE_DRIVETRAIN_SCENARIO.json`
- `NATIVE_TRAILER_RUNTIME.json`
- `GTT_RUNTIME.log`
- `DEMO_TECHNICAL_GATE.json`
- `VISUAL_RUNTIME_SMOKE.json`
- `DEMO_VISUAL_EVIDENCE.json`
- `GTT_VISUAL_RUNTIME.log`
- `DemoVisualEvidence/GTT_visual_*.png`
- candidate `.zip` and `.zip.sha256`

`DEMO_VISUAL_ACCEPTANCE.json` is intentionally created only after a reviewer has inspected the exact candidate screenshots.

## Release acceptance gate

A Demo Release is eligible only when all evidence refers to the same candidate commit and original ZIP: source sanity green; real UE 5.8 Win64 preflight/build/package PASS; packaged runtime/gameplay PASS; Native Chaos/drivetrain/loaded trailer PASS; technical gate PASS; non-NullRHI rendered run PASS; five-scene `DEMO_VISUAL_EVIDENCE.json` PASS; human review of those exact images; `DEMO_VISUAL_ACCEPTANCE.json` bound to the SHA/hashes; and a final `-RequireVisual` gate PASS. No demo-critical blocker may remain.

GitHub-hosted `windows-latest` runners do not provide this project's licensed Unreal Engine 5.8 installation/content. Until an eligible self-hosted runner actually executes Stage 1, `Full Win64 CI/build runner` stays open and no Demo Release should be claimed.

See `Docs/PLAYTEST_0.1.20.md` for the detailed 30-step visual and publication integrity matrix.
