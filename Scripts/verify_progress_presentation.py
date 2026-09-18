#!/usr/bin/env python3
"""Independent SWIR Visual Report v3 presentation gate for maintained GTT status surfaces."""
from __future__ import annotations

import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
ROADMAP = ROOT / "Docs/ROADMAP.md"
CARD = ROOT / "assets/readme/progress-card.svg"
MINI = ROOT / "assets/readme/progress-mini.svg"
TEMPLATE = ROOT / "assets/readme/progress-template.svg"
LEGACY_METER_RE = re.compile(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}[^\n]*%?", re.MULTILINE)


def fail(message: str) -> None:
    raise AssertionError(message)


def main() -> int:
    readme = README.read_text(encoding="utf-8")
    roadmap = ROADMAP.read_text(encoding="utf-8")
    if "<!-- SWIR-README-STANDARD:v2 -->" not in readme:
        fail("README PRO v2 marker missing or downgraded")
    for token in (
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
        "<!-- ROADMAP-PROGRESS:START -->",
        "<!-- ROADMAP-PROGRESS:END -->",
        "## 📊 Overall progress",
        "| ✅ Completed | ⏳ Remaining | 📦 Total | 🎯 Progress |",
    ):
        if token not in roadmap:
            fail(f"roadmap protected structure missing: {token}")

    checks = re.findall(r"^\s*-\s*\[(x|X| )\]\s+", roadmap, flags=re.MULTILINE)
    completed = sum(mark.lower() == "x" for mark in checks)
    total = len(checks)
    if total <= 0:
        fail("roadmap checklist scope is empty")
    remaining = total - completed
    percent = round(completed * 100.0 / total, 1)
    if (completed, remaining, total, percent) != (125, 5, 130, 96.2):
        fail(f"authoritative roadmap truth drifted: {completed}/{total} ({percent:.1f}%)")

    for token in (
        f"ROADMAP-{percent:.1f}%25",
        f"DONE-{completed}%2F{total}",
        f"| **{completed}** | **{remaining}** | **{total}** | **{percent:.1f}%** |",
    ):
        if token not in roadmap:
            fail(f"roadmap numeric dashboard mismatch: {token}")

    if readme.count("assets/readme/progress-card.svg") != 1:
        fail("README must embed exactly one roadmap progress-card.svg")
    if "progress-mini.svg" in readme:
        fail("README must not duplicate card and mini for the same roadmap scope")
    if roadmap.count("../assets/readme/progress-mini.svg") != 1:
        fail("Roadmap must embed exactly one progress-mini.svg with the canonical relative path")
    if "progress-card.svg" in roadmap:
        fail("Roadmap must not duplicate mini and card for the same scope")
    if "progress-template.svg" in readme or "progress-template.svg" in roadmap:
        fail("TEMPLATE asset must not be embedded as live project data")
    if LEGACY_METER_RE.search(readme) or LEGACY_METER_RE.search(roadmap):
        fail("legacy text/Unicode progress meter detected on a maintained status surface")

    for path in (CARD, MINI, TEMPLATE):
        if not path.is_file():
            fail(f"missing progress asset: {path.relative_to(ROOT)}")
        ET.parse(path)
        text = path.read_text(encoding="utf-8")
        for color in ("#02050A", "#07111C", "#0088FF", "#62E5FF"):
            if color not in text:
                fail(f"{path.name} missing SWIR visual token {color}")
    if "TEMPLATE" not in TEMPLATE.read_text(encoding="utf-8").upper() or "N/A" not in TEMPLATE.read_text(encoding="utf-8"):
        fail("progress-template.svg must be clearly marked TEMPLATE and use N/A placeholders")

    subprocess.run([sys.executable, str(ROOT / "Scripts/generate_progress_svg.py"), "--check"], check=True)
    print(f"SWIR Visual Report v3 presentation gate: PASS — roadmap {completed}/{total} ({percent:.1f}%), SVG-only, release readiness kept separate")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, ET.ParseError, subprocess.CalledProcessError) as exc:
        print(f"SWIR Visual Report v3 presentation gate: FAIL — {exc}", file=sys.stderr)
        raise SystemExit(1)
