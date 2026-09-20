#!/usr/bin/env python3
"""Validate the rolling exact-source Win64 candidate and reviewed-release qualification contract."""

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
ATTESTED_RUNNER = ROOT / "Scripts" / "run_win64_attested_candidate_acceptance.ps1"
ATTESTOR = ROOT / "Scripts" / "write_win64_candidate_attestation.ps1"

MINIMUM_CANDIDATE_VERSION = (0, 1, 61)
FINAL_ASSET = "Content/GTT/Vehicles/Trailer/SK_GTT_FarmTrailer.uasset"


def fail(message: str) -> None:
    raise AssertionError(message)


def require(text: str, token: str, scope: str) -> None:
    if token not in text:
        fail(f"{scope}: missing required token: {token}")


def forbid(text: str, token: str, scope: str) -> None:
    if token in text:
        fail(f"{scope}: forbidden token present: {token}")


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


def current_project_version(config: str) -> str:
    match = re.search(r"(?m)^ProjectVersion=(\d+)\.(\d+)\.(\d+)\s*$", config)
    if not match:
        fail("DefaultGame.ini: ProjectVersion must be a semantic x.y.z version")
    version_tuple = tuple(int(part) for part in match.groups())
    if version_tuple < MINIMUM_CANDIDATE_VERSION:
        fail(
            "DefaultGame.ini: candidate version regressed below the 0.1.61 qualification baseline: "
            + ".".join(str(part) for part in version_tuple)
        )
    return ".".join(match.groups())


def main() -> int:
    package = PACKAGE.read_text(encoding="utf-8")
    release = RELEASE.read_text(encoding="utf-8")
    config = CONFIG.read_text(encoding="utf-8")
    wrapper = IMPORT_WRAPPER.read_text(encoding="utf-8")
    importer = IMPORT_SCRIPT.read_text(encoding="utf-8")
    package_helper = PACKAGE_HELPER.read_text(encoding="utf-8")
    acceptance_runner = ACCEPTANCE_RUNNER.read_text(encoding="utf-8")
    attested_runner = ATTESTED_RUNNER.read_text(encoding="utf-8")
    attestor = ATTESTOR.read_text(encoding="utf-8")

    candidate_version = current_project_version(config)

    package_version = input_block(package, "version")
    release_version = input_block(release, "version")
    require(package_version, "required: true", "package version input")
    require(release_version, "required: true", "release version input")
    require(package, "does not match ProjectVersion", "package workflow")
    require(release, "does not match exact-candidate ProjectVersion", "release workflow")

    for stale in ("default: '0.1.52'", 'default: "0.1.52"', "default: '0.1.20'", 'default: "0.1.20"'):
        forbid(package + release, stale, "candidate workflows")

    for token in (
        "runs-on: [self-hosted, windows, x64, unreal-5.8]",
        "actions/checkout@v4",
        "lfs: true",
        "fetch-depth: 0",
        "run_win64_attested_candidate_acceptance.ps1",
        "WIN64_ACCEPTANCE_SUMMARY.json",
        "WIN64_CANDIDATE_ATTESTATION.json",
        "FIELDMASTER_HILL_HAUL_RUNTIME.json",
        "NATIVE_AUTHORITY_RUNTIME.json",
        "FINAL_SHA256SUMS.txt",
        "gtt.win64-candidate-attestation.v1",
        "fieldmaster_hill_haul_runtime",
        "human_visual_review",
        "demo_release_authorized",
        "Get-FileHash -Algorithm SHA256",
        "GTT-${{ inputs.version }}-Win64-technical-candidate",
        "actions/upload-artifact@v4",
    ):
        require(package, token, "package workflow")
    require_order(
        package,
        [
            "Validate explicit candidate version against project metadata",
            "Run sealed exact-candidate acceptance",
            "Verify sealed candidate evidence",
            "Upload sealed Win64 evidence",
        ],
        "package workflow",
    )
    forbid(package, "Compress-Archive", "package workflow")

    for token in (
        "ProjectVersion",
        "Candidate run must originate from main",
        "Candidate run SHA",
        "GTT-${{ inputs.version }}-Win64-technical-candidate",
        "WIN64_CANDIDATE_ATTESTATION.json",
        "FINAL_SHA256SUMS.txt",
        "FIELDMASTER_HILL_HAUL_RUNTIME.json",
        "gtt.win64-candidate-attestation.v1",
        "visual_review_passed",
        "write_demo_visual_acceptance.ps1",
        "Publish reviewed Demo prerelease",
    ):
        require(release, token, "release workflow")
    require_order(
        release,
        [
            "Verify candidate workflow provenance",
            "Checkout the exact candidate source contract",
            "Verify release version against exact candidate project metadata",
            "Download exact candidate artifact",
            "Verify candidate archive integrity and expand evidence",
            "Verify sealed exact-candidate attestation and integrity manifest",
            "Verify authored trailer import evidence is from exact candidate",
            "Verify rendered evidence is from exact candidate",
            "Bind human visual acceptance to reviewed screenshots",
            "Re-evaluate full exact-candidate gate with visual acceptance",
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

    for token in (
        "40-character git SHA",
        "clean tracked working tree",
        "preflight_win64_unreal.ps1",
        "import_gtt_farm_trailer_unreal.ps1",
        "package_windows.ps1",
        "smoke_test_windows.ps1",
        "evaluate_native_chaos_runtime.ps1",
        "evaluate_native_authority_runtime.ps1",
        "evaluate_authored_trailer_runtime.ps1",
        "DEMO_TECHNICAL_GATE.json",
        "capture_demo_visual_evidence.ps1",
        "evaluate_demo_visual_evidence.ps1",
        "gtt.win64-candidate-acceptance.v1",
        'human_visual_review = "REQUIRED"',
        "demo_release_authorized = $false",
    ):
        require(acceptance_runner, token, "base acceptance runner")

    for token in (
        "run_win64_candidate_acceptance.ps1",
        "evaluate_fieldmaster_hill_haul_runtime.ps1",
        "write_win64_candidate_attestation.ps1",
        "fieldmaster_hill_haul_runtime",
    ):
        require(attested_runner, token, "attested acceptance runner")
    require_order(
        attested_runner,
        ["& $BaseRunner", "& $HillHaulEvaluator", "& $Attestor"],
        "attested acceptance runner",
    )

    for token in (
        'Read-JsonRequired "FIELDMASTER_HILL_HAUL_RUNTIME.json"',
        'Read-JsonRequired "NATIVE_AUTHORITY_RUNTIME.json"',
        "FINAL_SHA256SUMS.txt",
        "WIN64_CANDIDATE_ATTESTATION.json",
        "Get-FileHash -Algorithm SHA256",
        "Compress-Archive",
    ):
        require(attestor, token, "candidate attestor")

    print(
        f"GTT {candidate_version} Win64 candidate pipeline sanity: PASS "
        "(single attested runner, exact-SHA sealed evidence, reviewed release provenance)"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
