#!/usr/bin/env python3
"""Static contract checks for the Win64 UE 5.8 runner-qualification handoff."""
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
require(version == "0.1.68", f"runner-qualification milestone must stay on current candidate 0.1.68, got {version}")

qualifier = read("Scripts/qualify_win64_runner.ps1")
fixture = read("Scripts/test_win64_runner_qualification.ps1")
manual_workflow = read(".github/workflows/win64-runner-qualification.yml")
candidate_workflow = read(".github/workflows/gtt-v0.1.61-win64-attested-candidate.yml")
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
    'runs-on: [self-hosted, windows, x64, unreal-5.8]',
    "Scripts/qualify_win64_runner.ps1",
    "WIN64_RUNNER_QUALIFICATION.json",
    "actions/upload-artifact@v4",
    "qualification-only",
):
    require(token in manual_workflow, f"manual runner workflow missing {token!r}")
require(f"default: '{version}'" in manual_workflow, "manual runner workflow version default is stale")
require("release" not in manual_workflow.lower() or "release authorization" in manual_workflow.lower(),
        "runner qualification workflow must not publish a GitHub Release")

qualifier_pos = candidate_workflow.find("qualify_win64_runner.ps1")
acceptance_pos = candidate_workflow.find("run_win64_attested_candidate_acceptance.ps1")
require(qualifier_pos >= 0, "exact-candidate workflow must qualify the runner")
require(acceptance_pos >= 0, "exact-candidate workflow lost the attested acceptance runner")
require(qualifier_pos < acceptance_pos, "runner qualification must execute before expensive exact-candidate acceptance")
require("WIN64_RUNNER_QUALIFICATION.json" in candidate_workflow, "exact-candidate workflow must persist runner qualification evidence")
require(f"default: '{version}'" in candidate_workflow, "exact-candidate workflow version default is stale")

for token in (
    "test_win64_runner_qualification.ps1",
    "verify_win64_runner_qualification_contract.py",
    "verify_progress_presentation.py",
    "verify_project.py",
):
    require(token in sanity_workflow, f"runner qualification sanity workflow missing {token!r}")
require("runs-on: windows-latest" in sanity_workflow, "fixture must execute in real PowerShell on windows-latest")

for token in (
    "positive qualification",
    "wrong-version",
    "wrong-sha",
    "human_visual_review",
    "demo_release_authorized",
):
    require(token in fixture, f"PowerShell fixture coverage missing {token!r}")

require("self-hosted" in docs and "unreal-5.8" in docs, "runner docs must state qualifying labels")
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
    f"(version={version}, roadmap={checked}/{total}={expected_percent:.1f}%, open={open_items})"
)
