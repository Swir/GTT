from __future__ import annotations

import re
import runpy
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")


package = read("Scripts/package_windows.ps1")
validator = read("Scripts/validate_windows_package.ps1")
release_workflow = read(".github/workflows/release-windows.yml")
candidate_workflow = read(".github/workflows/win64-package-evidence.yml")
sanity_workflow = read(".github/workflows/project-sanity.yml")
roadmap = read("Docs/ROADMAP.md")
changelog = read("CHANGELOG.md")
playtest = read("Docs/PLAYTEST_0.0.27.md")
release_doc = read("Docs/RELEASE_WINDOWS.md")

for token in (
    "BuildCookRun",
    "-platform=Win64",
    "-build",
    "-cook",
    "-stage",
    "-pak",
    "-iostore",
    "-archive",
    "validate_windows_package.ps1",
    "SHA256SUMS.txt",
    "BUILD_INFO.json",
    "Compress-Archive",
):
    assert token in package, f"package_windows.ps1 missing {token}"

for token in (
    "GTT.exe",
    "PACKAGE_VALIDATION.json",
    ".pak",
    ".utoc",
    ".ucas",
    "Shipping",
    ".pdb",
    "10MB",
):
    assert token in validator, f"validate_windows_package.ps1 missing {token}"

for token in (
    "workflow_dispatch:",
    "self-hosted",
    "windows",
    "x64",
    "unreal-5.8",
    "lfs: true",
    "package_windows.ps1",
    "smoke_test_windows.ps1",
    "evaluate_demo_candidate.ps1",
    "capture_demo_visual_evidence.ps1",
    "evaluate_demo_visual_evidence.ps1",
    "actions/upload-artifact@v4",
    "if-no-files-found: error",
):
    assert token in candidate_workflow, f"candidate workflow missing {token}"

for token in (
    "candidate_run_id",
    "expected_sha",
    "visual_review_passed",
    "visual_review_notes",
    "actions/download-artifact@v4",
    "run-id:",
    "write_demo_visual_acceptance.ps1",
    "evaluate_demo_candidate.ps1",
    "-RequireVisual",
    "softprops/action-gh-release@v2",
    "prerelease: true",
):
    assert token in release_workflow, f"reviewed release workflow missing {token}"

assert "package_windows.ps1" not in release_workflow
assert "preflight_win64_unreal.ps1" not in release_workflow
assert "confirm_runtime_smoke" not in release_workflow
assert "verify_release_pipeline.py" in sanity_workflow
assert "0.0.27" in changelog and "Release Pipeline" in changelog
assert "Win64" in playtest and "SHA256" in playtest
assert "self-hosted" in release_doc and "Unreal Engine 5.8" in release_doc
assert "candidate_run_id" in release_doc and "exact packaged candidate" in release_doc.lower()

# Preserve the protected roadmap data contract while enforcing the current
# SWIR Visual Report v3 SVG-only presentation. The old 20-cell text bar is
# deliberately retired; ordinary numeric table data remains authoritative.
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "<!-- ROADMAP-PROGRESS:START -->" in roadmap and "<!-- ROADMAP-PROGRESS:END -->" in roadmap
assert "## 📊 Overall progress" in roadmap
assert "alt=\"CI\"" in roadmap
assert "alt=\"Roadmap progress\"" in roadmap
assert "alt=\"Completed\"" in roadmap
assert "alt=\"Status\"" in roadmap
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(
    r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}",
    roadmap,
    flags=re.MULTILINE,
), "legacy text/Unicode roadmap progress meter must not return"

checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
uncheck = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + uncheck
assert total >= 130 and checked >= 118, f"release milestone regressed: got {checked}/{total}"
percent = round(checked / total * 100, 1)
assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"| **{checked}** | **{total - checked}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert "- [x] Packaging/release automation" in roadmap
assert "- [ ] Full Win64 CI/build runner" in roadmap

runpy.run_path(str(ROOT / "Scripts/verify_demo_candidate_gate.py"), run_name="__main__")
runpy.run_path(str(ROOT / "Scripts/verify_visual_release_gate.py"), run_name="__main__")

print(
    f"Release pipeline sanity OK: roadmap {checked}/{total} ({percent:.1f}%), "
    "SVG-only progress presentation verified"
)
