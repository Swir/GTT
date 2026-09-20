#!/usr/bin/env python3
"""Fail-closed source contract for GTT exact-candidate attestation."""

from __future__ import annotations

from pathlib import Path
import hashlib
import re
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ATTEST = ROOT / "Scripts" / "write_win64_candidate_attestation.ps1"
RUNNER = ROOT / "Scripts" / "run_win64_attested_candidate_acceptance.ps1"
WORKFLOW = ROOT / ".github" / "workflows" / "gtt-v0.1.61-win64-attested-candidate.yml"


def fail(message: str) -> None:
    raise AssertionError(message)


def require(text: str, token: str, scope: str) -> None:
    if token not in text:
        fail(f"{scope}: missing required token: {token}")


def require_order(text: str, tokens: list[str], scope: str) -> None:
    positions = []
    for token in tokens:
        pos = text.find(token)
        if pos < 0:
            fail(f"{scope}: order token missing: {token}")
        positions.append(pos)
    if positions != sorted(positions):
        fail(f"{scope}: fail-closed stage order changed: {tokens}")


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def exercise_hash_failure_boundary() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        evidence = root / "NATIVE_AUTHORITY_RUNTIME.json"
        evidence.write_text('{"result":"PASS","authority_faults":0}', encoding="utf-8")
        sealed = digest(evidence)
        if not re.fullmatch(r"[0-9a-f]{64}", sealed):
            fail("hash fixture did not produce canonical lowercase SHA256")
        evidence.write_text('{"result":"FAIL","authority_faults":1}', encoding="utf-8")
        if digest(evidence) == sealed:
            fail("mutated authority evidence unexpectedly retained sealed digest")


def main() -> int:
    attest = ATTEST.read_text(encoding="utf-8")
    runner = RUNNER.read_text(encoding="utf-8")
    workflow = WORKFLOW.read_text(encoding="utf-8")

    for token in (
        "ExpectedGitSha must be an exact 40-character Git SHA",
        "BUILD_INFO.json",
        "BUILD_ATTEMPT.json",
        "AUTHORED_TRAILER_IMPORT.json",
        "RUNTIME_SMOKE.json",
        "NATIVE_CHAOS_RUNTIME.json",
        "NATIVE_AUTHORITY_RUNTIME.json",
        'authority must be NATIVE_CHAOS',
        'contains split-authority faults',
        "NATIVE_TRAILER_RUNTIME.json",
        "DEMO_TECHNICAL_GATE.json",
        "schema -ne 17",
        "DEMO_VISUAL_EVIDENCE.json",
        "WIN64_ACCEPTANCE_SUMMARY.json",
        'native authority runtime is not PASS',
        "human_visual_review",
        "demo_release_authorized",
        'Get-ChildItem -Path $PackageDirectory -Recurse -File -Filter "GTT.exe"',
        'Get-ChildItem -Path (Join-Path $PackageDirectory "DemoVisualEvidence")',
        "gtt.win64-candidate-attestation.v1",
        'native_authority_runtime = "PASS"',
        "native_authority_faults = 0",
        "WIN64_CANDIDATE_ATTESTATION.json",
        "FINAL_SHA256SUMS.txt",
        "Get-FileHash -Algorithm SHA256",
        "Compress-Archive",
    ):
        require(attest, token, "attestation writer")

    for token in (
        "run_win64_candidate_acceptance.ps1",
        "write_win64_candidate_attestation.ps1",
        "WIN64_ACCEPTANCE_SUMMARY.json",
        "human_visual_review",
        "demo_release_authorized",
        "Base acceptance summary does not match requested candidate identity",
    ):
        require(runner, token, "attested acceptance wrapper")
    require_order(
        runner,
        [
            "& $BaseRunner",
            "$summary = Get-Content -Raw $summaryPath | ConvertFrom-Json",
            "& $Attestor",
        ],
        "attested acceptance wrapper",
    )

    for token in (
        "workflow_dispatch:",
        "runs-on: [self-hosted, windows, x64, unreal-5.8]",
        "timeout-minutes: 120",
        "actions/checkout@v4",
        "lfs: true",
        "fetch-depth: 0",
        "run_win64_attested_candidate_acceptance.ps1",
        "WIN64_CANDIDATE_ATTESTATION.json",
        "FINAL_SHA256SUMS.txt",
        "WIN64_ACCEPTANCE_SUMMARY.json",
        "github.sha",
        "actions/upload-artifact@v4",
    ):
        require(workflow, token, "attested candidate workflow")

    if "softprops/action-gh-release" in workflow or "gh release" in workflow.lower():
        fail("attested candidate workflow must not publish a release")
    if "cancel-in-progress: true" in workflow:
        fail("a running long Win64 qualification must not be cancelled by a later dispatch")

    exercise_hash_failure_boundary()
    print(
        "GTT candidate attestation sanity: PASS "
        "(exact SHA/version/config + native authority + packaged EXE/evidence hashes + final manifest + self-hosted lane)"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
