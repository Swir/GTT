#!/usr/bin/env python3
"""Validate the exact-source Win64 candidate and release qualification contract."""

from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / ".github" / "workflows" / "win64-package-evidence.yml"
RELEASE = ROOT / ".github" / "workflows" / "release-windows.yml"
CONFIG = ROOT / "Config" / "DefaultGame.ini"
IMPORT_WRAPPER = ROOT / "Scripts" / "import_gtt_farm_trailer_unreal.ps1"
IMPORT_SCRIPT = ROOT / "Scripts" / "Unreal" / "import_gtt_farm_trailer.py"
PACKAGE_HELPER = ROOT / "Scripts" / "package_windows.ps1"
ACCEPTANCE_RUNNER = ROOT / "Scripts" / "run_win64_candidate_acceptance.ps1"

EXPECTED_VERSION = "0.1.61"
FINAL_ASSET = "Content/GTT/Vehicles/Trailer/SK_GTT_FarmTrailer.uasset"


def fail(message: str) -> None:
    raise AssertionError(message)


def require(text: str, token: str, scope: str) -> None:
    if token not in text:
        fail(f"{scope}: missing required token: {token}")


def forbid(text: str, token: str, scope: str) -> None:
    if token in text:
        fail(f"{scope}: forbidden stale token present: {token}")


def require_order(text: str, tokens: list[str], scope: str) -> None:
    positions = []
    for token in tokens:
        pos = text.find(token)
        if pos < 0:
            fail(f"{scope}: order token missing: {token}")
        positions.append(pos)
    if positions != sorted(positions):
        fail(f"{scope}: gate order is not fail-closed: {tokens}")


def input_block(text: str, key: str) -> str:
    marker = f"      {key}:"
    start = text.find(marker)
    if start < 0:
        fail(f"workflow input {key!r} missing")
    tail = text[start + len(marker):]
    match = re.search(r"(?m)^      [A-Za-z0-9_-]+:\s*$", tail)
    end = start + len(marker) + (match.start() if match else len(tail))
    return text[start:end]


def require_current_default(block: str, scope: str) -> None:
    match = re.search(r"(?m)^\s+default:\s*['\"]([^'\"]+)['\"]\s*$", block)
    if not match:
        fail(f"{scope}: current-version default missing")
    if match.group(1) != EXPECTED_VERSION:
        fail(f"{scope}: default {match.group(1)!r} does not match ProjectVersion {EXPECTED_VERSION}")


def main() -> int:
    package = PACKAGE.read_text(encoding="utf-8")
    release = RELEASE.read_text(encoding="utf-8")
    config = CONFIG.read_text(encoding="utf-8")
    wrapper = IMPORT_WRAPPER.read_text(encoding="utf-8")
    importer = IMPORT_SCRIPT.read_text(encoding="utf-8")
    package_helper = PACKAGE_HELPER.read_text(encoding="utf-8")
    acceptance_runner = ACCEPTANCE_RUNNER.read_text(encoding="utf-8")

    require(config, f"ProjectVersion={EXPECTED_VERSION}", "DefaultGame.ini")

    for stale in ("default: '0.1.52'", 'default: "0.1.52"', "default: '0.1.20'", 'default: "0.1.20"'):
        forbid(package + release, stale, "candidate workflows")

    package_version = input_block(package, "version")
    release_version = input_block(release, "version")
    require(package_version, "required: true", "package version input")
    require(release_version, "required: true", "release version input")
    require_current_default(package_version, "package version input")
    require_current_default(release_version, "release version input")

    require(package, "runs-on: [self-hosted, windows, x64, unreal-5.8]", "package workflow")
    require(package, "ProjectVersion", "package workflow")
    require(package, "import_gtt_farm_trailer_unreal.ps1", "package workflow")
    require(package, FINAL_ASSET.replace("/", "\\"), "package workflow")
    require(package, "AUTHORED_TRAILER_IMPORT.json", "package workflow")
    require(package, "gtt.authored-trailer-import.v1", "package workflow")
    require(package, 'ExpectedGitSha "$env:GITHUB_SHA"', "package workflow")
    require(package, "capture_demo_visual_evidence.ps1", "package workflow")
    require(package, "evaluate_demo_visual_evidence.ps1", "package workflow")
    require(package, "GTT-${{ inputs.version }}-Win64-technical-candidate", "package workflow")
    require_order(
        package,
        [
            "Validate explicit candidate version against project metadata",
            "Win64 runner and Unreal 5.8 preflight",
            "Import and validate authored trailer in UE 5.8",
            "Bind authored trailer import evidence to exact candidate",
            "Package Win64",
            "Runtime smoke test packaged EXE with deterministic scenarios",
            "Run rendered visual evidence route without NullRHI",
            "Upload verified Win64 evidence",
        ],
        "package workflow",
    )

    require(release, "ProjectVersion", "release workflow")
    require(release, "AUTHORED_TRAILER_IMPORT.json", "release workflow")
    require(release, "gtt.authored-trailer-import.v1", "release workflow")
    require(release, "Candidate run must originate from main", "release workflow")
    require(release, "Candidate run SHA", "release workflow")
    require(release, "visual_review_passed", "release workflow")
    require(release, "deterministic end-to-end gameplay/runtime route", "release notes")
    forbid(release, "33-step gameplay route", "release notes")
    require_order(
        release,
        [
            "Verify candidate workflow provenance",
            "Checkout the exact candidate source contract",
            "Verify release version against exact candidate project metadata",
            "Download exact candidate artifact",
            "Verify authored trailer import evidence is from exact candidate",
            "Verify rendered evidence is from exact candidate",
            "Bind human visual acceptance to reviewed screenshots",
            "Publish reviewed Demo prerelease",
        ],
        "release workflow",
    )

    require(wrapper, "[string]$UnrealEditorCmd", "authored trailer import wrapper")
    require(wrapper, "AUTHORED_TRAILER_IMPORT result=PASS", "authored trailer import wrapper")
    require(importer, 'DESTINATION = "/Game/GTT/Vehicles/Trailer"', "authored trailer importer")
    require(importer, 'ASSET_NAME = "SK_GTT_FarmTrailer"', "authored trailer importer")
    require(importer, "create_physics_asset", "authored trailer importer")
    require(importer, "PHYSICS_ASSET_UNASSIGNED", "authored trailer importer")
    for socket in ("socket_hitch", "socket_cargo", "socket_axle_l", "socket_axle_r"):
        require(importer, socket, "authored trailer importer")

    require(package_helper, '[string]$Version = ""', "package helper")
    require(package_helper, "ProjectVersion", "package helper")
    require(package_helper, "does not match ProjectVersion", "package helper")
    forbid(package_helper, '[string]$Version = "0.1.14"', "package helper")

    for token in (
        "40-character git SHA",
        "clean tracked working tree",
        "preflight_win64_unreal.ps1",
        "import_gtt_farm_trailer_unreal.ps1",
        "package_windows.ps1",
        "smoke_test_windows.ps1",
        "evaluate_native_chaos_runtime.ps1",
        "evaluate_authored_trailer_runtime.ps1",
        "DEMO_TECHNICAL_GATE.json",
        "capture_demo_visual_evidence.ps1",
        "evaluate_demo_visual_evidence.ps1",
        "gtt.win64-candidate-acceptance.v1",
        'human_visual_review = "REQUIRED"',
        "demo_release_authorized = $false",
    ):
        require(acceptance_runner, token, "one-command acceptance runner")

    print(
        "GTT 0.1.61 Win64 candidate pipeline sanity: PASS "
        "(current version, UE 5.8 preflight, authored trailer import, package/runtime/visual evidence, "
        "local exact-candidate runner, exact-SHA release gate)"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
