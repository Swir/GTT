#!/usr/bin/env python3
"""Source/integration verifier for GTT 0.1.68 sealed Win64 archive integrity."""

from __future__ import annotations

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
ARCHIVE = ROOT / "Scripts/verify_win64_candidate_archive.ps1"
FIXTURE = ROOT / "Scripts/test_win64_candidate_archive_verifier.ps1"
RUNNER = ROOT / "Scripts/run_win64_attested_candidate_acceptance.ps1"
ATTESTOR = ROOT / "Scripts/write_win64_candidate_attestation.ps1"
WORKFLOW = ROOT / ".github/workflows/gtt-v0.1.61-win64-attested-candidate.yml"
SANITY_WORKFLOW = ROOT / ".github/workflows/v0-1-68-win64-archive-integrity-sanity.yml"
CONFIG = ROOT / "Config/DefaultGame.ini"
ROADMAP = ROOT / "Docs/ROADMAP.md"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.1.68.md"
CHANGELOG = ROOT / "CHANGELOG.d/0.1.68-win64-archive-integrity.md"
REGRESSION_0167 = ROOT / "Scripts/verify_v0_1_67_fieldmaster_hud_runtime_evidence.py"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


archive = ARCHIVE.read_text(encoding="utf-8")
fixture = FIXTURE.read_text(encoding="utf-8")
runner = RUNNER.read_text(encoding="utf-8")
attestor = ATTESTOR.read_text(encoding="utf-8")
workflow = WORKFLOW.read_text(encoding="utf-8")
sanity_workflow = SANITY_WORKFLOW.read_text(encoding="utf-8")
config = CONFIG.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")
playtest = PLAYTEST.read_text(encoding="utf-8")
changelog = CHANGELOG.read_text(encoding="utf-8")
regression = REGRESSION_0167.read_text(encoding="utf-8")

project_version_match = re.search(r"^ProjectVersion=(\d+)\.(\d+)\.(\d+)\s*$", config, flags=re.MULTILINE)
require(project_version_match is not None, "Config/DefaultGame.ini must declare a semantic ProjectVersion")
project_version = ".".join(project_version_match.groups())
project_version_tuple = tuple(int(part) for part in project_version_match.groups())
require(project_version_tuple >= (0, 1, 68), "archive-integrity regression requires ProjectVersion >= 0.1.68")

for token in (
    "FINAL_SHA256SUMS.txt",
    "WIN64_CANDIDATE_ATTESTATION.json",
    "gtt.win64-candidate-attestation.v1",
    "gtt.win64-candidate-archive-verification.v1",
    "Get-FileHash -Algorithm SHA256",
    "OpenRead($ZipPath)",
    "Expand-Archive",
    "does not exactly match FINAL_SHA256SUMS.txt",
    "unsafe path",
    "duplicate path",
    "evidence_file_count",
    "Attested evidence hash mismatch",
    'Get-ChildItem -Path $tempRoot -Recurse -File -Filter "GTT.exe"',
    "packaged_exe_sha256",
    "native_authority_runtime",
    "fieldmaster_hill_haul_runtime",
    "technical_gate_schema",
    "rendered_screenshot_count",
    'human_visual_review -ne "REQUIRED"',
    "demo_release_authorized",
    "$ZipPath.verify.json",
):
    require(token in archive, f"archive verifier missing fail-closed contract: {token}")

for token in (
    'Write-Fixture -Name "pass"',
    'Write-Fixture -Name "bad-sidecar"',
    'Write-Fixture -Name "unmanifested"',
    'Write-Fixture -Name "bad-boundary"',
    'sidecar hash does not match',
    'does not exactly match',
    'human visual review / Demo Release boundary',
):
    require(token in fixture, f"archive fixture test missing case: {token}")

require('"verify_win64_candidate_archive.ps1"' in runner, "attested runner must require archive verifier")
require(
    runner.index("& $Attestor") < runner.index("& $ArchiveVerifier"),
    "archive round-trip verification must run after final attestation/archive creation",
)
require("WIN64_ARCHIVE_VERIFICATION" in runner, "attested runner must validate external archive verification result")
require("$ZipPath.verify.json" in archive, "archive verification evidence must live outside sealed ZIP")

for token in (
    f"default: '{project_version}'",
    "run_win64_attested_candidate_acceptance.ps1",
    "WIN64_CANDIDATE_ATTESTATION.json",
    "FINAL_SHA256SUMS.txt",
    ".zip.verify.json",
    "github.sha",
    "human_visual_review",
    "demo_release_authorized",
    "actions/upload-artifact@v4",
):
    require(token in workflow, f"qualifying Win64 workflow missing archive-integrity integration: {token}")

for token in (
    "windows-latest",
    "test_win64_candidate_archive_verifier.ps1",
    "verify_v0_1_68_win64_archive_integrity.py",
    "verify_v0_1_67_fieldmaster_hud_runtime_evidence.py",
    "verify_progress_presentation.py",
    "verify_project.py",
):
    require(token in sanity_workflow, f"0.1.68 sanity workflow missing: {token}")

require("gtt.win64-candidate-attestation.v1" in attestor, "0.1.68 must retain the established exact-candidate attestation")
require("FINAL_SHA256SUMS.txt" in attestor and "Compress-Archive" in attestor, "attestor must still create sealed manifest + ZIP")

require("version_tuple" in regression and ">= (0, 1, 67)" in regression, "0.1.67 verifier must remain a forward-compatible regression guard")

checkboxes = re.findall(r"^- \[([ xX])\] ", roadmap, flags=re.MULTILINE)
done = sum(1 for value in checkboxes if value.lower() == "x")
require((done, len(checkboxes)) == (125, 130), f"roadmap checkbox math drifted to {done}/{len(checkboxes)}")
require("**125** | **5** | **130** | **96.2%**" in roadmap, "roadmap table must remain 125/130 = 96.2%")
require(roadmap.count("../assets/readme/progress-mini.svg") == 1, "roadmap must embed exactly one mini SVG")
require(not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, re.MULTILINE), "legacy text progress meter returned")

for required_open in (
    "Dedicated native Chaos wheeled tractor movement",
    "Full Unreal compile + packaged Win64 smoke test",
    "Dedicated native Chaos drivetrain/suspension/wheel setup",
    "Authored skeletal trailer wheel assets and final hitch sockets",
    "Full Win64 CI/build runner",
):
    require(f"- [ ] {required_open}" in roadmap, f"runtime/art gate closed without evidence: {required_open}")

for token in (
    "0.1.68",
    "archive",
    "FINAL_SHA256SUMS.txt",
    "exact candidate",
    "human visual review",
    "does not close",
):
    require(token.lower() in playtest.lower(), f"playtest missing: {token}")
    require(token.lower() in changelog.lower(), f"changelog missing: {token}")

print(f"GTT 0.1.68 Win64 archive integrity: forward-compatible source/integration contract OK for candidate {project_version}")
print("Sealed ZIP keeps fail-closed round-trip verification with full manifest/evidence identity checks")
print("Roadmap truth preserved: 125/130 = 96.2%; five runtime/art gates remain open")
