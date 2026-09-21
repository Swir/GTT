# GTT Win64 UE 5.8 runner qualification

This is a **fast blocker-isolation gate** for the current **0.1.69** Demo candidate. It does not add gameplay scope, does not raise ROADMAP completion and does not replace the full Win64 candidate acceptance. `Config/DefaultGame.ini` is the candidate-version source of truth; the qualification, package and exact-candidate workflows must stay synchronized with that value.

## Why this exists

The remaining GTT roadmap work is dominated by real Unreal Engine 5.8 / Win64 runtime and authored-asset evidence. Source CI cannot prove those gates. Before spending up to two hours on the full package/runtime/visual pipeline, a qualifying machine can prove that the runner itself is usable.

The qualification checks:

- Windows host and a real UE **5.8** installation;
- `RunUAT.bat`, `UnrealEditor-Cmd.exe` and UnrealBuildTool presence;
- Visual Studio C++ toolchain and Windows SDK;
- project EngineAssociation, Chaos Vehicles plugin and GTT runtime module;
- exact Git SHA and exact `ProjectVersion`;
- clean tracked worktree;
- Git LFS integrity (`git lfs fsck`);
- free disk and workspace write access;
- the canonical `preflight_win64_unreal.ps1`;
- an actual time-bounded `UnrealEditor-Cmd` project bootstrap under `-NullRHI`.

A successful run emits `WIN64_RUNNER_QUALIFICATION.json` using schema `gtt.win64-runner-qualification.v1`.

## Fast qualification workflow

Use branch **`qualification/0.1.69-runner-probe`** for the exact-head push probe, or dispatch **GTT Win64 UE 5.8 runner qualification** with version `0.1.69`. The hosted probe must pass before work is handed to a self-hosted runner carrying all four required labels:

- `self-hosted`
- `windows`
- `x64`
- `unreal-5.8`

All versioned qualification probes now share **one global concurrency lane**. New probes created with this workflow automatically supersede older queued or in-progress probes from other versioned branches.

Runs created before that migration used ref-scoped concurrency and cannot retroactively inherit the new group. The hosted probe therefore performs a narrowly scoped one-time/defensive cleanup of **legacy queued** qualification runs before dispatching the real runner: it may cancel only older run IDs for this exact workflow path, re-queries the queue, and fails closed if any older qualification wait remains. The cleanup result and cancelled run IDs are written into `WIN64_RUNNER_DISPATCH_PROBE.json`. The workflow has `actions: write` solely for that cancellation step and never runs on pull-request events.

Use the same UE installation root that will be used by the final candidate workflow. The self-hosted stage checks out LFS content, binds the report to `${{ github.sha }}` and uploads the JSON, nested preflight JSON and editor-probe log even when qualification fails.

A failure is intended to be actionable before the expensive package/runtime pass. Fix the failing machine/toolchain/content condition, then rerun qualification. A queued self-hosted stage is an external runner-availability blocker, not proof of qualification.

## Full candidate hand-off

The existing **GTT Win64 attested candidate acceptance** workflow runs the same qualification first. Only a passing qualification proceeds to packaging, packaged EXE smoke, native Chaos/runtime checks, authored trailer runtime checks, rendered evidence, attestation and sealed ZIP verification.

The full candidate remains bound to one exact SHA/version/configuration. Qualification evidence is uploaded separately and included with the final technical candidate. The **Win64 package evidence** workflow uses the same current `ProjectVersion` default and rejects any explicit version that does not match project metadata.

## Release boundary

Runner qualification **does not close** any of the five remaining ROADMAP gates. It proves that a self-hosted machine can load the exact project with UE 5.8 and has the required build environment; it does not prove a successful Win64 package, packaged runtime behavior, final authored trailer behavior or presentation quality.

`human_visual_review` therefore remains `REQUIRED`, and `demo_release_authorized` remains `false`. The first Demo Release is still blocked until the exact packaged candidate passes the complete technical workflow and the separate human visual review.
