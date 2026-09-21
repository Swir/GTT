# GTT Win64 UE 5.8 runner qualification

This is a **fast blocker-isolation gate** for the existing 0.1.68 Demo candidate. It does not add gameplay scope, does not raise ROADMAP completion and does not replace the full Win64 candidate acceptance.

## Why this exists

The remaining GTT roadmap work is dominated by real Unreal Engine 5.8 / Win64 runtime and authored-asset evidence. Source CI cannot prove those gates. Before spending up to two hours on the full package/runtime/visual pipeline, a qualifying machine can now prove that the runner itself is usable.

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

Dispatch **GTT Win64 UE 5.8 runner qualification** on a self-hosted runner carrying all four required labels:

- `self-hosted`
- `windows`
- `x64`
- `unreal-5.8`

Use the same UE installation root that will be used by the final candidate workflow. The workflow checks out LFS content, binds the report to `${{ github.sha }}` and uploads the JSON, nested preflight JSON and editor-probe log even when qualification fails.

A failure is intended to be actionable before the expensive package/runtime pass. Fix the failing machine/toolchain/content condition, then rerun qualification.

## Full candidate hand-off

The existing **GTT Win64 attested candidate acceptance** workflow now runs the same qualification first. Only a passing qualification proceeds to packaging, packaged EXE smoke, native Chaos/runtime checks, authored trailer runtime checks, rendered evidence, attestation and sealed ZIP verification.

The full candidate remains bound to one exact SHA/version/configuration. Qualification evidence is uploaded separately and included with the final technical candidate.

## Release boundary

Runner qualification **does not close** any of the five remaining ROADMAP gates. It proves that a self-hosted machine can load the exact project with UE 5.8 and has the required build environment; it does not prove a successful Win64 package, packaged runtime behavior, final authored trailer behavior or presentation quality.

`human_visual_review` therefore remains `REQUIRED`, and `demo_release_authorized` remains `false`. The first Demo Release is still blocked until the exact packaged candidate passes the complete technical workflow and the separate human visual review.
