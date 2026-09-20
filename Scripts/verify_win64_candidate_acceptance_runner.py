#!/usr/bin/env python3
"""Validate the one-command exact Win64 candidate acceptance runner contract."""

from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "Scripts" / "run_win64_candidate_acceptance.ps1"
PACKAGE = ROOT / "Scripts" / "package_windows.ps1"
CONFIG = ROOT / "Config" / "DefaultGame.ini"

MINIMUM_CANDIDATE_VERSION = (0, 1, 61)


def fail(message: str) -> None:
    raise AssertionError(message)


def require(text: str, token: str, scope: str) -> None:
    if token not in text:
        fail(f"{scope}: missing required token: {token}")


def forbid(text: str, token: str, scope: str) -> None:
    if token in text:
        fail(f"{scope}: forbidden token present: {token}")


def require_order(text: str, tokens: list[str], scope: str) -> None:
    positions: list[int] = []
    for token in tokens:
        pos = text.find(token)
        if pos < 0:
            fail(f"{scope}: order token missing: {token}")
        positions.append(pos)
    if positions != sorted(positions):
        fail(f"{scope}: fail-closed stage order changed: {tokens}")


def current_project_version(config: str) -> tuple[str, tuple[int, int, int]]:
    match = re.search(r"(?m)^ProjectVersion=(\d+)\.(\d+)\.(\d+)\s*$", config)
    if not match:
        fail("project metadata: ProjectVersion must be a semantic x.y.z version")
    version_tuple = tuple(int(part) for part in match.groups())
    if version_tuple < MINIMUM_CANDIDATE_VERSION:
        fail(
            "project metadata: candidate version regressed below the 0.1.61 exact-candidate baseline: "
            + ".".join(str(part) for part in version_tuple)
        )
    return match.group(0).split("=", 1)[1].strip(), version_tuple


def main() -> int:
    runner = RUNNER.read_text(encoding="utf-8")
    package = PACKAGE.read_text(encoding="utf-8")
    config = CONFIG.read_text(encoding="utf-8")

    candidate_version, _ = current_project_version(config)

    require(package, '[string]$Version = ""', "package helper")
    require(package, "Config\\DefaultGame.ini", "package helper")
    require(package, "ProjectVersion", "package helper")
    require(package, "does not match ProjectVersion", "package helper")
    forbid(package, '[string]$Version = "0.1.14"', "package helper")

    for token in (
        "Config\\DefaultGame.ini",
        "ProjectVersion",
        "40-character git SHA",
        "clean tracked working tree",
        "$env:GITHUB_SHA = $GitSha",
        "preflight_win64_unreal.ps1",
        "import_gtt_farm_trailer_unreal.ps1",
        "SK_GTT_FarmTrailer.uasset",
        "gtt.authored-trailer-import.v1",
        "package_windows.ps1",
        "smoke_test_windows.ps1",
        '-MinimumAliveSeconds", 472',
        '-LaunchTimeoutSeconds", 505',
        "evaluate_native_chaos_runtime.ps1",
        "evaluate_native_authority_runtime.ps1",
        "evaluate_drivetrain_scenario.ps1",
        "evaluate_authored_trailer_runtime.ps1",
        "evaluate_farm_cargo_runtime.ps1",
        "promote_demo_gate_workshop_priority_pickup.ps1",
        "DEMO_TECHNICAL_GATE.json",
        "schema -ne 17",
        "capture_demo_visual_evidence.ps1",
        '-MinimumAliveSeconds", 178',
        '-LaunchTimeoutSeconds", 205',
        "evaluate_demo_visual_evidence.ps1",
        "DEMO_VISUAL_EVIDENCE.json",
        "GTT_visual_*.png",
        "gtt.win64-candidate-acceptance.v1",
        'native_authority_runtime = "PASS"',
        'human_visual_review = "REQUIRED"',
        "demo_release_authorized = $false",
        "WIN64_ACCEPTANCE_SUMMARY.json",
        "Compress-Archive",
        "Get-FileHash -Algorithm SHA256",
    ):
        require(runner, token, "acceptance runner")

    require_order(
        runner,
        [
            'Invoke-GTTScript "preflight_win64_unreal.ps1"',
            'Invoke-GTTScript "import_gtt_farm_trailer_unreal.ps1"',
            'Invoke-GTTScript "package_windows.ps1"',
            'Invoke-GTTScript "smoke_test_windows.ps1"',
            '"evaluate_native_chaos_runtime.ps1"',
            '"evaluate_native_authority_runtime.ps1"',
            '"evaluate_authored_trailer_runtime.ps1"',
            'Invoke-GTTScript "evaluate_demo_candidate.ps1"',
            '"promote_demo_gate_workshop_priority_pickup.ps1"',
            'Invoke-GTTScript "capture_demo_visual_evidence.ps1"',
            'Invoke-GTTScript "evaluate_demo_visual_evidence.ps1"',
            'schema = "gtt.win64-candidate-acceptance.v1"',
            "Compress-Archive",
        ],
        "acceptance runner",
    )

    forbid(runner, "softprops/action-gh-release", "acceptance runner")
    forbid(runner, "visual_review_passed=true", "acceptance runner")

    print(
        f"GTT {candidate_version} Win64 candidate acceptance runner sanity: PASS "
        "(rolling canonical version + exact SHA + UE import/package/runtime/native-authority/rendered evidence + human-review boundary)"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
