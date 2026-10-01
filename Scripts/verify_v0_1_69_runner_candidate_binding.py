#!/usr/bin/env python3
"""Fail-closed source contract for exact UE 5.8 runner -> sealed Win64 candidate binding."""

from __future__ import annotations

from pathlib import Path
import hashlib
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BINDER = ROOT / "Scripts" / "bind_win64_runner_to_candidate.ps1"
WORKFLOW = ROOT / ".github" / "workflows" / "gtt-v0.1.61-win64-attested-candidate.yml"
QUALIFIER = ROOT / "Scripts" / "qualify_win64_runner.ps1"
PREFLIGHT = ROOT / "Scripts" / "preflight_win64_unreal.ps1"


def fail(message: str) -> None:
    raise AssertionError(message)


def require(text: str, token: str, scope: str) -> None:
    if token not in text:
        fail(f"{scope}: missing required token: {token}")


def require_order(text: str, tokens: list[str], scope: str) -> None:
    positions: list[int] = []
    for token in tokens:
        pos = text.find(token)
        if pos < 0:
            fail(f"{scope}: order token missing: {token}")
        positions.append(pos)
    if positions != sorted(positions):
        fail(f"{scope}: fail-closed stage order changed: {tokens}")


def digest(path: Path) -> str:
    h = hashlib.sha256()
    h.update(path.read_bytes())
    return h.hexdigest()


def exercise_mutation_boundary() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        qualification = root / "WIN64_RUNNER_QUALIFICATION.json"
        archive = root / "GTT.zip"
        qualification.write_text('{"result":"PASS","git_sha":"a"}', encoding="utf-8")
        archive.write_bytes(b"sealed-candidate")
        qualification_hash = digest(qualification)
        archive_hash = digest(archive)
        qualification.write_text('{"result":"FAIL","git_sha":"b"}', encoding="utf-8")
        archive.write_bytes(b"mutated-candidate")
        if digest(qualification) == qualification_hash:
            fail("mutated qualification unexpectedly retained sealed digest")
        if digest(archive) == archive_hash:
            fail("mutated candidate archive unexpectedly retained sealed digest")


def main() -> int:
    binder = BINDER.read_text(encoding="utf-8")
    workflow = WORKFLOW.read_text(encoding="utf-8")
    qualifier = QUALIFIER.read_text(encoding="utf-8")
    preflight = PREFLIGHT.read_text(encoding="utf-8")

    for token in (
        "gtt.win64-runner-candidate-binding.v1",
        "gtt.win64-runner-qualification.v1",
        "gtt.win64-candidate-archive-verification.v1",
        "GTT_WIN64_UNREAL_PREFLIGHT",
        "ExpectedGitSha must be an exact 40-character Git SHA",
        "Runner qualification Git SHA does not match exact candidate",
        "Runner qualification version does not match exact candidate",
        "Runner qualification must contain PASS preflight, editor build, and editor probe evidence",
        "Runner preflight Git SHA does not match exact candidate",
        "Runner qualification/preflight host machine mismatch.",
        "GitHub Actions runner identity is incomplete",
        "GitHub Actions runner OS must be Windows.",
        "GitHub Actions runner architecture must be X64.",
        "Runner qualification was produced on a different machine than the live GitHub Actions runner.",
        "Runner preflight was produced on a different machine than the live GitHub Actions runner.",
        "preflight_machine",
        "same_machine_verified",
        "engine-version-5.8",
        "project-engine-association",
        "chaos-vehicles-plugin",
        "gtt-runtime-module",
        "msvc-toolchain",
        "windows-sdk",
        "netfx-sdk",
        "free-disk",
        "Candidate archive verification identity does not match exact candidate",
        "Candidate ZIP SHA256 sidecar does not match archive bytes",
        "Candidate archive verification hash does not match sealed ZIP bytes",
        "qualificationSha256",
        "preflightSha256",
        "archiveVerificationSha256",
        'human_visual_review = "REQUIRED"',
        "demo_release_authorized = $false",
        "Get-FileHash -Algorithm SHA256",
        "round-trip exact identity/hash/release-boundary validation",
    ):
        require(binder, token, "runner/candidate binder")

    require_order(
        binder,
        [
            '$qualification = Read-JsonRequired',
            '$preflight = Read-JsonRequired',
            '$isGitHubActions = [string]$env:GITHUB_ACTIONS -eq "true"',
            '$archiveVerification = Read-JsonRequired',
            '$archiveSha256 = (Get-FileHash',
            '$qualificationSha256 = (Get-FileHash',
            '$binding = [ordered]@{',
            '$roundTrip = Read-JsonRequired',
        ],
        "runner/candidate binder",
    )

    for token in (
        'schema = "gtt.win64-runner-qualification.v1"',
        'preflight = $PreflightResult',
        'editor_build = $EditorBuildResult',
        'editor_probe = $EditorProbeResult',
        'human_visual_review = "REQUIRED"',
        'demo_release_authorized = $false',
    ):
        require(qualifier, token, "runner qualifier")

    for token in (
        'evidence_schema = 2',
        'gate = "GTT_WIN64_UNREAL_PREFLIGHT"',
        'machine = $env:COMPUTERNAME',
        'Add-Check "msvc-toolchain"',
        'Add-Check "windows-sdk"',
        'Add-Check "netfx-sdk"',
        'Add-Check "engine-version-5.8"',
        'Add-Check "free-disk"',
    ):
        require(preflight, token, "Win64 preflight")

    for token in (
        "Qualify self-hosted UE 5.8 runner before expensive acceptance",
        "Qualify exact Win64 candidate",
        "Bind exact qualifying runner/toolchain to sealed candidate archive",
        "bind_win64_runner_to_candidate.ps1",
        'WIN64_RUNNER_QUALIFICATION.json',
        'WIN64_RUNNER_QUALIFICATION.json.preflight.json',
        'WIN64_RUNNER_QUALIFICATION.json.editor-build.log',
        '.zip.runner-bind.json',
        "Upload exact attested technical candidate",
        "actions/upload-artifact@v4",
    ):
        require(workflow, token, "attested candidate workflow")

    require_order(
        workflow,
        [
            "Qualify self-hosted UE 5.8 runner before expensive acceptance",
            "Qualify exact Win64 candidate",
            "Verify exact-candidate identity and archive boundary before upload",
            "Bind exact qualifying runner/toolchain to sealed candidate archive",
            "Upload exact attested technical candidate",
        ],
        "attested candidate workflow",
    )

    if "softprops/action-gh-release" in workflow or "gh release" in workflow.lower():
        fail("runner/candidate binding lane must not publish a release")

    exercise_mutation_boundary()
    print(
        "GTT runner/candidate binding sanity: PASS "
        "(same-machine UE 5.8 qualification/preflight + live Windows X64 runner identity + MSVC/SDK + exact SHA/version + sealed ZIP hash binding + release boundary)"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}")
        raise SystemExit(1)
