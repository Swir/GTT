#!/usr/bin/env python3
"""Static contract checks for the current Win64 UE 5.8 runner/candidate handoff."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing required file: {rel}")
    return path.read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


config = read("Config/DefaultGame.ini")
match = re.search(r"(?m)^ProjectVersion=(.+)$", config)
require(match is not None, "ProjectVersion missing from Config/DefaultGame.ini")
version = match.group(1).strip()
require(re.fullmatch(r"\d+\.\d+\.\d+", version) is not None, f"unexpected ProjectVersion format: {version!r}")
probe_branch = f"qualification/{version}-runner-probe"

qualifier = read("Scripts/qualify_win64_runner.ps1")
fixture = read("Scripts/test_win64_runner_qualification.ps1")
provisioner = read("Scripts/provision_win64_ue58_runner.ps1")
provision_fixture = read("Scripts/test_win64_runner_provisioning.ps1")
manual_workflow = read(".github/workflows/win64-runner-qualification.yml")
candidate_workflow = read(".github/workflows/gtt-v0.1.61-win64-attested-candidate.yml")
package_workflow = read(".github/workflows/win64-package-evidence.yml")
sanity_workflow = read(".github/workflows/win64-runner-qualification-sanity.yml")
docs = read("Docs/WIN64_RUNNER_QUALIFICATION.md")
roadmap = read("Docs/ROADMAP.md")

for token in (
    'schema = "gtt.win64-runner-qualification.v1"',
    '"expected-git-sha"',
    '"expected-project-version"',
    '"clean-tracked-tree"',
    '"git-lfs-fsck"',
    '"workspace-write"',
    '"x64-host-process"',
    '"github-runner-arch-label"',
    '"win64-unreal-preflight"',
    '"editor-nullrhi-project-probe"',
    'human_visual_review = "REQUIRED"',
    'demo_release_authorized = $false',
):
    require(token in qualifier, f"qualifier contract missing {token!r}")

require("preflight_win64_unreal.ps1" in qualifier, "qualifier must reuse canonical UE/Win64 preflight")
require("UnrealEditor-Cmd.exe" in qualifier and "-NullRHI" in qualifier, "qualifier must probe UnrealEditor-Cmd under NullRHI")
require("EditorProbeTimeoutSeconds" in qualifier and "WaitForExit" in qualifier, "editor probe must be time bounded")
require("git lfs fsck" in qualifier, "qualifier must fail closed on unresolved/corrupt LFS state")

for token in (
    '"gtt.win64-runner-provisioning.v1"',
    '@("self-hosted", "windows", "x64", "unreal-5.8")',
    '$CustomLabels = "unreal-5.8"',
    "GTT_GITHUB_RUNNER_TOKEN",
    "--labels $CustomLabels",
    "qualification_required = $true",
    'exact_candidate_binding = "NONE - provisioning/readiness only"',
    "roadmap_gate_closed = $false",
    "native_chaos_runtime_verified = $false",
    "authored_trailer_runtime_verified = $false",
    "packaged_exe_smoke_verified = $false",
    'human_visual_review = "REQUIRED"',
    "demo_release_authorized = $false",
    "Build.version",
    "RunUAT.bat",
    "UnrealEditor-Cmd.exe",
    "UnrealBuildTool",
    "git lfs version",
):
    require(token in provisioner, f"runner provisioning contract missing {token!r}")
require("--no-default-labels" not in provisioner,
        "provisioner must preserve GitHub default self-hosted/windows/x64 labels")
require("registration token" not in provisioner.lower() or "short-lived" in docs.lower(),
        "registration credential boundary must be documented as short-lived")
require("qualification workflow" in provisioner.lower(),
        "provisioner must hand authority back to the real qualification workflow")

for token in (
    'runs-on: [self-hosted, windows, x64, unreal-5.8]',
    "Scripts/qualify_win64_runner.ps1",
    "WIN64_RUNNER_QUALIFICATION.json",
    "actions/upload-artifact@v4",
    "qualification-only",
    "probe-contract",
    "needs: probe-contract",
    probe_branch,
    "WIN64_RUNNER_DISPATCH_PROBE.json",
):
    require(token in manual_workflow, f"manual runner workflow missing {token!r}")
require(f"default: '{version}'" in manual_workflow, "manual runner workflow version default is stale")
require(manual_workflow.count(f"'{version}'") >= 3,
        "manual runner workflow push/dispatch fallbacks are not all bound to current ProjectVersion")
require("cancel-in-progress: true" in manual_workflow,
        "qualification workflow must supersede stale queued runs so the current exact-head probe can execute")
require("cancel-in-progress: false" not in manual_workflow,
        "qualification workflow must not let a stale runner wait block a newer exact-head probe")
require(
    re.search(r"(?m)^\s*group:\s*gtt-win64-runner-qualification\s*$", manual_workflow) is not None,
    "qualification workflow must use one global concurrency lane across versioned probe branches",
)
require(
    "gtt-win64-runner-qualification-${{ github.ref }}" not in manual_workflow,
    "qualification concurrency must not be scoped by github.ref; stale version branches must be superseded",
)
require("actions: write" in manual_workflow,
        "qualification workflow needs narrowly scoped Actions write permission for legacy stale-run cancellation")
require("pull_request:" not in manual_workflow,
        "write-enabled qualification workflow must never execute from pull_request events")
for token in (
    "Cancel legacy stale qualification runs",
    "GTT_CURRENT_RUN_ID",
    'workflow_path = ".github/workflows/win64-runner-qualification.yml"',
    "status=queued&per_page=100",
    "if run_id >= current_run_id:",
    "/actions/runs/{run_id}/cancel",
    "stale queued qualification runs remain after cleanup",
    "GTT_CANCELLED_STALE_QUALIFICATION_RUNS",
    '"legacy_stale_queue_clear": True',
    '"cancelled_stale_qualification_run_ids": cancelled',
):
    require(token in manual_workflow, f"legacy qualification queue cleanup missing {token!r}")
require("release" not in manual_workflow.lower() or "release authorization" in manual_workflow.lower(),
        "runner qualification workflow must not publish a GitHub Release")

qualifier_pos = candidate_workflow.find("qualify_win64_runner.ps1")
acceptance_pos = candidate_workflow.find("run_win64_attested_candidate_acceptance.ps1")
require(qualifier_pos >= 0, "exact-candidate workflow must qualify the runner")
require(acceptance_pos >= 0, "exact-candidate workflow lost the attested acceptance runner")
require(qualifier_pos < acceptance_pos, "runner qualification must execute before expensive exact-candidate acceptance")
require("WIN64_RUNNER_QUALIFICATION.json" in candidate_workflow, "exact-candidate workflow must persist runner qualification evidence")
require(f"default: '{version}'" in candidate_workflow, "exact-candidate workflow version default is stale")

require(f"default: '{version}'" in package_workflow, "Win64 package evidence workflow version default is stale")
for token in (
    'runs-on: [self-hosted, windows, x64, unreal-5.8]',
    "run_win64_attested_candidate_acceptance.ps1",
    "Config\\DefaultGame.ini",
    "ProjectVersion",
    "NATIVE_CHAOS_RUNTIME.json",
    "NATIVE_TRAILER_RUNTIME.json",
    "DEMO_VISUAL_EVIDENCE.json",
):
    require(token in package_workflow, f"Win64 package evidence workflow missing {token!r}")

for token in (
    "test_win64_runner_qualification.ps1",
    "provision_win64_ue58_runner.ps1",
    "test_win64_runner_provisioning.ps1",
    "verify_win64_runner_qualification_contract.py",
    "verify_progress_presentation.py",
    "verify_project.py",
):
    require(token in sanity_workflow, f"runner qualification sanity workflow missing {token!r}")
require("runs-on: windows-latest" in sanity_workflow, "fixture must execute in real PowerShell on windows-latest")
require("Exercise runner provisioning readiness and fail-closed fixtures" in sanity_workflow,
        "sanity workflow must exercise host provisioning fixtures")

for token in (
    "positive qualification",
    "wrong-version",
    "wrong-sha",
    "human_visual_review",
    "demo_release_authorized",
):
    require(token in fixture, f"PowerShell fixture coverage missing {token!r}")

for token in (
    "PlanOnly",
    "missing-editor",
    "no-token",
    "qualification_required",
    "roadmap_gate_closed",
    "human_visual_review",
    "demo_release_authorized",
):
    require(token in provision_fixture, f"runner provisioning fixture coverage missing {token!r}")

require(version in docs, "runner qualification docs must identify the current ProjectVersion candidate")
require("self-hosted" in docs and "unreal-5.8" in docs, "runner docs must state qualifying labels")
require("global concurrency lane" in docs.lower(), "runner docs must explain cross-branch stale-run supersession")
require("legacy queued" in docs.lower(), "runner docs must explain one-time pre-migration stale-run cleanup")
require("provision_win64_ue58_runner.ps1" in docs, "runner docs must include the host provisioning helper")
require("GTT_GITHUB_RUNNER_TOKEN" in docs, "runner docs must document the short-lived registration-token handoff")
require("provisioning does not qualify" in docs.lower(), "runner docs must preserve provisioning/qualification truth boundary")
require("does not close" in docs.lower(), "runner docs must preserve roadmap gate boundary")
require("human visual" in docs.lower(), "runner docs must preserve human visual review boundary")

checked = len(re.findall(r"(?m)^\s*-\s+\[x\]\s+", roadmap, flags=re.IGNORECASE))
open_items = len(re.findall(r"(?m)^\s*-\s+\[ \]\s+", roadmap))
total = checked + open_items
expected_percent = round((checked * 100.0 / total), 1) if total else 0.0
require(checked == 125 and open_items == 5, f"roadmap truth changed unexpectedly: checked={checked}, open={open_items}")

progress_row = re.search(
    r"(?m)^\|\s*\*\*(\d+)\*\*\s*\|\s*\*\*(\d+)\*\*\s*\|\s*\*\*(\d+)\*\*\s*\|\s*\*\*([0-9]+(?:\.[0-9]+)?)%\*\*\s*\|\s*$",
    roadmap,
)
require(progress_row is not None, "canonical roadmap progress table row missing")
table_completed, table_remaining, table_total = map(int, progress_row.group(1, 2, 3))
table_percent = float(progress_row.group(4))
require(
    (table_completed, table_remaining, table_total) == (checked, open_items, total),
    "roadmap progress table counters do not match checklist truth",
)
require(abs(table_percent - expected_percent) < 0.05, "roadmap progress table percentage does not match checklist math")
require(f"ROADMAP-{expected_percent:.1f}%25" in roadmap, "roadmap progress badge percentage drifted")
require(f"DONE-{checked}%2F{total}" in roadmap, "roadmap DONE badge counter drifted")
require(f"{checked} of {total} tasks complete, {expected_percent:.1f} percent" in roadmap, "roadmap SVG alt text drifted")
require("../assets/readme/progress-mini.svg" in roadmap, "roadmap mini SVG path missing")

print(
    "GTT Win64 runner qualification contract: PASS "
    f"(version={version}, probe={probe_branch}, roadmap={checked}/{total}={expected_percent:.1f}%, open={open_items})"
)
