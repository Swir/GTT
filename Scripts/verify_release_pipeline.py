from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    p = ROOT / path
    assert p.exists(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")


package = read("Scripts/package_windows.ps1")
validator = read("Scripts/validate_windows_package.ps1")
release_workflow = read(".github/workflows/release-windows.yml")
sanity_workflow = read(".github/workflows/project-sanity.yml")
roadmap = read("Docs/ROADMAP.md")
changelog = read("CHANGELOG.md")
playtest = read("Docs/PLAYTEST_0.0.27.md")
release_doc = read("Docs/RELEASE_WINDOWS.md")

# Packaging must remain an actual Unreal Automation Tool build/cook/package path.
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

# Release workflow must be opt-in and explicitly bound to a project-controlled UE runner.
for token in (
    "workflow_dispatch:",
    "self-hosted",
    "Windows",
    "X64",
    "unreal-5.8",
    "lfs: true",
    "package_windows.ps1",
    "actions/upload-artifact@v4",
    "if-no-files-found: error",
):
    assert token in release_workflow, f"release workflow missing {token}"

assert "verify_release_pipeline.py" in sanity_workflow, "project sanity does not execute release verification"
assert "0.0.27" in changelog, "0.0.27 changelog entry missing"
assert "Release Pipeline" in changelog, "release milestone not documented"
assert "Win64" in playtest and "SHA256" in playtest, "release playtest is incomplete"
assert "self-hosted" in release_doc and "Unreal Engine 5.8" in release_doc, "release runner boundary is undocumented"

# SWIR roadmap lock and arithmetic. This milestone remains completed even as later milestones advance the totals.
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "<!-- ROADMAP-PROGRESS:START -->" in roadmap
assert "<!-- ROADMAP-PROGRESS:END -->" in roadmap
assert "## 📊 Overall progress" in roadmap
assert "alt=\"CI\"" in roadmap and "alt=\"Roadmap progress\"" in roadmap
assert "alt=\"Completed\"" in roadmap and "alt=\"Status\"" in roadmap

checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
uncheck = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + uncheck
assert total >= 130 and checked >= 118, f"release milestone regressed: got {checked}/{total}"
percent = round(checked / total * 100, 1)
filled = round(checked / total * 20)
bar = "█" * filled + "░" * (20 - filled)

assert f"ROADMAP-{percent:.1f}%25" in roadmap
assert f"DONE-{checked}%2F{total}" in roadmap
assert f"{bar} {percent:.1f}%" in roadmap
assert f"| **{checked}** | **{total - checked}** | **{total}** | **{percent:.1f}%** |" in roadmap
assert "- [x] Packaging/release automation" in roadmap
assert "- [ ] Full Win64 CI/build runner" in roadmap

print(f"Release pipeline sanity OK: roadmap {checked}/{total} ({percent:.1f}%), bar={bar}")
