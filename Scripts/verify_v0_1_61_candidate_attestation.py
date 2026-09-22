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
ARCHIVE = ROOT / "Scripts" / "verify_win64_candidate_archive.ps1"
WORKFLOW = ROOT / ".github" / "workflows" / "gtt-v0.1.61-win64-attested-candidate.yml"
QUALIFICATION_WORKFLOW = ROOT / ".github" / "workflows" / "win64-runner-qualification.yml"


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
    archive = ARCHIVE.read_text(encoding="utf-8")
    workflow = WORKFLOW.read_text(encoding="utf-8")
    qualification_workflow = QUALIFICATION_WORKFLOW.read_text(encoding="utf-8")

    # AUDIT #16 FINISH-FIRST: the attestation must seal the concrete packaged-runtime
    # observations for the existing five-gate target, not only generic PASS files.
    for token in (
        "ExpectedGitSha must be an exact 40-character Git SHA",
        "BUILD_INFO.json",
        "BUILD_ATTEMPT.json",
        "AUTHORED_TRAILER_IMPORT.json",
        "RUNTIME_SMOKE.json",
        "NATIVE_CHAOS_RUNTIME.json",
        "gtt.native-chaos-runtime.v1",
        "native_physics_accepted",
        "wheel_setup_observed",
        "deterministic_fieldmaster_motion",
        "deterministic_fieldmaster_control",
        "physics_fallback_observed",
        "movement_active_samples",
        "max_valid_wheels",
        "max_suspension_samples",
        "configured_forward_gears",
        "unsafe_direction_shift_commits",
        "NATIVE_AUTHORITY_RUNTIME.json",
        'authority must be NATIVE_CHAOS',
        'contains split-authority faults',
        "NATIVE_DRIVETRAIN_SCENARIO.json",
        "gtt.native-drivetrain-scenario.v1",
        "diagnostic_failure_count",
        "max_forward_gear_observed",
        "reverse_interlock_speed_kmh",
        "reverse_commit_speed_kmh",
        "reverse_motion_signed_speed_kmh",
        "forward_commit_speed_kmh",
        "forward_motion_signed_speed_kmh",
        "NATIVE_TRAILER_RUNTIME.json",
        "gtt.native-trailer-runtime.v1",
        "authored_active_samples",
        "native_tow_samples",
        "dual_contact_samples",
        "safe_hitch_samples",
        "deterministic_loaded_tow",
        "safe_loaded_motion_samples",
        "controlled_stop_proven",
        "invalid_rig_observation_count",
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
        'native_chaos_tractor_movement = "PASS"',
        'native_chaos_drivetrain_suspension_wheels = "PASS"',
        'native_drivetrain_scenario = "PASS"',
        'native_authority_runtime = "PASS"',
        "native_authority_faults = 0",
        'authored_trailer_runtime = "PASS"',
        "authored_trailer_dual_contact_samples",
        "authored_trailer_safe_loaded_motion_samples",
        "authored_trailer_controlled_stop",
        "authored_trailer_invalid_rig_observations",
        "WIN64_CANDIDATE_ATTESTATION.json",
        "FINAL_SHA256SUMS.txt",
        "Get-FileHash -Algorithm SHA256",
        "Compress-Archive",
        "identity/current-gate/boundary validation",
    ):
        require(attest, token, "attestation writer")

    require_order(
        attest,
        [
            '$chaos = Read-JsonRequired "NATIVE_CHAOS_RUNTIME.json"',
            '$authority = Read-JsonRequired "NATIVE_AUTHORITY_RUNTIME.json"',
            '$drivetrain = Read-JsonRequired "NATIVE_DRIVETRAIN_SCENARIO.json"',
            '$trailer = Read-JsonRequired "NATIVE_TRAILER_RUNTIME.json"',
            '$gate = Read-JsonRequired "DEMO_TECHNICAL_GATE.json"',
            '$visual = Read-JsonRequired "DEMO_VISUAL_EVIDENCE.json"',
            '$attestation = [ordered]@{',
            '$roundTrip = Get-Content -Raw $attestationPath | ConvertFrom-Json',
        ],
        "attestation writer",
    )

    # The archive verifier is the consumer-side boundary after compression. It must
    # independently reject a sealed ZIP whose attestation lost any current target
    # Native Chaos or authored-trailer proof, even when the generic result says PASS.
    for token in (
        "gtt.win64-candidate-attestation.v1",
        "native_chaos_tractor_movement",
        "native_chaos_movement_samples",
        "native_chaos_max_speed_kmh",
        "native_chaos_drivetrain_suspension_wheels",
        "native_chaos_valid_wheels",
        "native_chaos_suspension_samples",
        "native_chaos_contact_samples",
        "native_drivetrain_scenario",
        "native_drivetrain_max_forward_gear",
        "native_drivetrain_diagnostic_failures",
        "authored_trailer_runtime",
        "authored_trailer_dual_contact_samples",
        "authored_trailer_safe_hitch_samples",
        "authored_trailer_safe_loaded_motion_samples",
        "authored_trailer_controlled_stop",
        "authored_trailer_invalid_rig_observations",
        "dedicated Native Chaos tractor movement acceptance",
        "Native Chaos drivetrain/suspension/wheel acceptance",
        "authored trailer runtime/hitch/wheel acceptance",
        "technical_gate_schema",
        "rendered_visual_evidence",
        "human_visual_review",
        "demo_release_authorized",
        "FINAL_SHA256SUMS.txt",
        "packaged_exe_sha256",
    ):
        require(archive, token, "archive verifier")

    require_order(
        archive,
        [
            '$attestation = Get-Content -Raw $attestationPath | ConvertFrom-Json',
            '$attestation.native_chaos_tractor_movement',
            '$attestation.native_chaos_drivetrain_suspension_wheels',
            '$attestation.native_drivetrain_scenario',
            '$attestation.authored_trailer_runtime',
            '$attestation.native_authority_runtime',
            '$attestation.technical_gate_schema',
            '$attestation.rendered_visual_evidence',
            '$attestation.human_visual_review',
        ],
        "archive verifier",
    )

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
        "workflow_call:",
        "workflow_dispatch:",
        "version:",
        "configuration:",
        "engine_root:",
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

    # Once a matching runner is available, qualification must hand the same caller
    # ref/SHA directly into the sealed technical candidate workflow rather than
    # stopping after a probe and requiring a second manual dispatch.
    for token in (
        "exact-candidate-after-qualification:",
        "needs: qualification-only",
        "uses: ./.github/workflows/gtt-v0.1.61-win64-attested-candidate.yml",
        "version: ${{ inputs.version || '0.1.69' }}",
        "configuration: Shipping",
        "engine_root: ${{ inputs.engine_root || 'C:\\Program Files\\Epic Games\\UE_5.8' }}",
        '"exact_candidate_handoff_required": True',
    ):
        require(qualification_workflow, token, "runner-to-candidate handoff")

    require_order(
        qualification_workflow,
        [
            "qualification-only:",
            "exact-candidate-after-qualification:",
            "needs: qualification-only",
            "uses: ./.github/workflows/gtt-v0.1.61-win64-attested-candidate.yml",
        ],
        "runner-to-candidate handoff",
    )

    if "softprops/action-gh-release" in workflow or "gh release" in workflow.lower():
        fail("attested candidate workflow must not publish a release")
    if "cancel-in-progress: true" in workflow:
        fail("a running long Win64 qualification must not be cancelled by a later dispatch")

    exercise_hash_failure_boundary()
    print(
        "GTT candidate attestation sanity: PASS "
        "(exact SHA/version/config + concrete Native Chaos movement/drivetrain/suspension/wheels + "
        "authored trailer runtime + sealed archive round-trip + packaged EXE/evidence hashes + "
        "qualified-runner exact-candidate handoff + human-review boundary)"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
