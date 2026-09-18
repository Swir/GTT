#!/usr/bin/env python3
# SWIR-PROGRESS-SVG-PRO:v1
from __future__ import annotations

import argparse
import math
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parents[1]
ROADMAP = ROOT / "Docs" / "ROADMAP.md"
README = ROOT / "README.md"
TEMPLATE = ROOT / "assets" / "readme" / "progress-template.svg"
CARD = ROOT / "assets" / "readme" / "progress-card.svg"
MINI = ROOT / "assets" / "readme" / "progress-mini.svg"

PROJECT = "GTT · Grand Theft Tractor"
SCOPE = "Roadmap checklist"
STATUS = "IN PROGRESS"
RELEASE_READINESS = "NOT READY — Win64/runtime/visual gates open"
CARD_TRACK_X = 50.0
CARD_TRACK_WIDTH = 1100.0
MINI_TRACK_X = 170.0
MINI_TRACK_WIDTH = 700.0
SVG_NS = "http://www.w3.org/2000/svg"
ET.register_namespace("", SVG_NS)

# Real textual meter glyphs are forbidden in maintained README / active roadmap status.
# Numeric percentages/counters stay as accessible fallback; visual meter = SVG only.
LEGACY_METER_RE = re.compile(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}[^\n]*%?", re.MULTILINE)


@dataclass(frozen=True)
class Progress:
    completed: int
    total: int
    fraction: float | None
    percentage_text: str
    counter_text: str


def compute_progress(completed: int, total: int) -> Progress:
    if completed < 0 or total < 0 or completed > total:
        raise ValueError(f"invalid checklist counts: {completed}/{total}")
    if total == 0:
        return Progress(completed, total, None, "N/A", "N/A tasks")
    fraction = completed / total
    pct = fraction * 100.0
    if completed < total and pct >= 100.0:
        pct = math.nextafter(100.0, 0.0)
    return Progress(completed, total, fraction, f"{pct:.1f}%", f"{completed} / {total} tasks")


def self_test_math() -> None:
    assert compute_progress(0, 10).percentage_text == "0.0%"
    assert compute_progress(10, 10).percentage_text == "100.0%"
    assert compute_progress(1, 13).percentage_text == "7.7%"
    assert compute_progress(0, 0).percentage_text == "N/A"
    assert len("Long milestone / hardware gate / runtime acceptance scope") > 40
    try:
        compute_progress(11, 10)
    except ValueError:
        pass
    else:
        raise AssertionError("invalid completed > total must fail")


def read_progress() -> Progress:
    text = ROADMAP.read_text(encoding="utf-8")
    checked = len(re.findall(r"^\s*-\s*\[x\]", text, flags=re.MULTILINE | re.IGNORECASE))
    unchecked = len(re.findall(r"^\s*-\s*\[ \]", text, flags=re.MULTILINE))
    progress = compute_progress(checked, checked + unchecked)
    if progress.fraction is None:
        raise AssertionError("GTT roadmap scope is unexpectedly empty; progress must be N/A until scope is restored")

    expected_dashboard = [
        "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
        "<!-- ROADMAP-PROGRESS:START -->",
        "<!-- ROADMAP-PROGRESS:END -->",
        "## 📊 Overall progress",
        "../assets/readme/progress-mini.svg",
        f"| **{progress.completed}** | **{progress.total - progress.completed}** | **{progress.total}** | **{progress.percentage_text}** |",
    ]
    for token in expected_dashboard:
        if token not in text:
            raise AssertionError(f"Docs/ROADMAP.md dashboard does not match checklist truth: missing {token!r}")
    if LEGACY_METER_RE.search(text):
        raise AssertionError("Docs/ROADMAP.md contains a legacy text/Unicode progress meter; use progress-mini.svg only")

    block_match = re.search(
        r"<!-- ROADMAP-PROGRESS:START -->(.*?)<!-- ROADMAP-PROGRESS:END -->",
        text,
        flags=re.DOTALL,
    )
    if not block_match:
        raise AssertionError("Docs/ROADMAP.md ROADMAP-PROGRESS block is missing")
    block = block_match.group(1)
    if block.count("../assets/readme/progress-mini.svg") != 1:
        raise AssertionError("ROADMAP-PROGRESS block must embed exactly one progress-mini.svg")
    return progress


def fmt_number(value: float) -> str:
    if not math.isfinite(value):
        raise ValueError("SVG geometry must be finite")
    return f"{value:.3f}".rstrip("0").rstrip(".")


def set_text(root: ET.Element, element_id: str, value: str) -> None:
    node = root.find(f".//*[@id='{element_id}']")
    if node is None:
        raise AssertionError(f"progress template missing id={element_id}")
    node.text = value


def render_card(progress: Progress) -> str:
    tree = ET.parse(TEMPLATE)
    root = tree.getroot()
    root.set("aria-label", f"{PROJECT}: {SCOPE}, {progress.counter_text}, {progress.percentage_text}, {STATUS}. Release readiness: {RELEASE_READINESS}.")

    title = root.find(f"{{{SVG_NS}}}title")
    desc = root.find(f"{{{SVG_NS}}}desc")
    if title is None or desc is None:
        raise AssertionError("progress template must include title and desc")
    title.text = f"{PROJECT} roadmap progress"
    desc.text = f"{SCOPE}: {progress.counter_text}, {progress.percentage_text}, status {STATUS}. Release readiness is separate: {RELEASE_READINESS}."

    set_text(root, "project", PROJECT)
    set_text(root, "scope", SCOPE)
    set_text(root, "status", STATUS)
    set_text(root, "percentage", progress.percentage_text)
    set_text(root, "counter", progress.counter_text)
    set_text(root, "readiness", f"Release readiness: {RELEASE_READINESS}")

    fill = root.find(".//*[@id='progress-fill']")
    glow = root.find(".//*[@id='progress-glow']")
    if fill is None or glow is None:
        raise AssertionError("progress template missing bounded fill elements")
    width = 0.0 if progress.fraction is None else CARD_TRACK_WIDTH * progress.fraction
    width = min(CARD_TRACK_WIDTH, max(0.0, width))
    fill.set("width", fmt_number(width))
    glow.set("width", fmt_number(width))
    opacity = "0" if width <= 0.0 else "1"
    fill.set("opacity", opacity)
    glow.set("opacity", "0" if width <= 0.0 else "0.34")

    ET.indent(tree, space="  ")
    body = ET.tostring(root, encoding="unicode", short_empty_elements=True)
    return '<?xml version="1.0" encoding="UTF-8"?>\n' + body + "\n"


def render_mini(progress: Progress) -> str:
    fraction = progress.fraction
    width = 0.0 if fraction is None else MINI_TRACK_WIDTH * fraction
    width = min(MINI_TRACK_WIDTH, max(0.0, width))
    fill = "" if width <= 0.0 else (
        f'<rect id="mini-progress-fill" x="{fmt_number(MINI_TRACK_X)}" y="49" width="{fmt_number(width)}" height="10" rx="5" fill="url(#miniGradient)" />'
    )
    glow = "" if width <= 0.0 else (
        f'<rect id="mini-progress-glow" x="{fmt_number(MINI_TRACK_X)}" y="49" width="{fmt_number(width)}" height="10" rx="5" fill="url(#miniGradient)" opacity="0.28" filter="url(#miniGlow)" />'
    )
    aria = escape(f"{PROJECT}: {progress.percentage_text}; {progress.counter_text}; status {STATUS}. Release readiness is separate and {RELEASE_READINESS}.", {'"': '&quot;'})
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="900" height="72" viewBox="0 0 900 72" role="img" aria-label="{aria}">
  <title>{escape(PROJECT)} roadmap progress</title>
  <desc>{escape(SCOPE)}: {escape(progress.counter_text)}, {escape(progress.percentage_text)}, status {STATUS}. Release readiness: {escape(RELEASE_READINESS)}.</desc>
  <defs>
    <linearGradient id="miniGradient" x1="0" x2="1"><stop offset="0" stop-color="#0088FF"/><stop offset="1" stop-color="#62E5FF"/></linearGradient>
    <filter id="miniGlow" x="-10%" y="-80%" width="120%" height="260%"><feGaussianBlur stdDeviation="4"/></filter>
  </defs>
  <rect x="0.5" y="0.5" width="899" height="71" rx="14" fill="#02050A" stroke="#17364A"/>
  <path d="M16 18H884M16 36H884M16 54H884" stroke="#07111C" stroke-width="1"/>
  <text x="20" y="29" fill="#F4FAFF" font-family="Segoe UI,Arial,sans-serif" font-size="17" font-weight="700">GTT ROADMAP</text>
  <text x="20" y="53" fill="#8DA8B8" font-family="Segoe UI,Arial,sans-serif" font-size="12">{escape(progress.counter_text)}</text>
  <rect x="170" y="49" width="700" height="10" rx="5" fill="#07111C" stroke="#17364A"/>
  {glow}
  {fill}
  <text x="870" y="31" text-anchor="end" fill="#62E5FF" font-family="Segoe UI,Arial,sans-serif" font-size="21" font-weight="700">{escape(progress.percentage_text)}</text>
  <text x="870" y="44" text-anchor="end" fill="#8DA8B8" font-family="Segoe UI,Arial,sans-serif" font-size="10">{STATUS}</text>
</svg>
'''


def validate_svg(path: Path, expected_track_width: float, expected_fraction: float | None) -> None:
    tree = ET.parse(path)
    root = tree.getroot()
    view_box = root.get("viewBox")
    if not view_box or len(view_box.split()) != 4:
        raise AssertionError(f"{path}: valid viewBox required")
    text = path.read_text(encoding="utf-8")
    for token in ("#02050A", "#07111C", "#0088FF", "#62E5FF", "#F4FAFF", "#8DA8B8"):
        if token not in text:
            raise AssertionError(f"{path}: missing SWIR color token {token}")
    if "foreignObject" in text or "<script" in text or "http://" in text.replace('xmlns="http://www.w3.org/2000/svg"', "") or "https://" in text:
        raise AssertionError(f"{path}: SVG must be self-contained")

    fill_id = "progress-fill" if path.name == "progress-card.svg" else "mini-progress-fill"
    fill = root.find(f".//*[@id='{fill_id}']")
    if expected_fraction is None:
        if fill is not None:
            raise AssertionError(f"{path}: zero/unknown progress must omit the luminous mini fill")
        return
    if fill is None:
        raise AssertionError(f"{path}: missing bounded progress fill {fill_id}")
    width = float(fill.get("width", "nan"))
    if not math.isfinite(width) or width < 0 or width > expected_track_width + 1e-6:
        raise AssertionError(f"{path}: progress fill escaped its track")
    expected = expected_track_width * expected_fraction
    if abs(width - expected) > 0.0015:
        raise AssertionError(f"{path}: progress fill is stale: {width} vs {expected}")


def verify_embeddings(progress: Progress) -> None:
    readme = README.read_text(encoding="utf-8")
    roadmap = ROADMAP.read_text(encoding="utf-8")
    if "<!-- SWIR-README-STANDARD:v2 -->" not in readme:
        raise AssertionError("README v2 marker missing or downgraded")
    if readme.count("assets/readme/progress-card.svg") != 1:
        raise AssertionError("README must embed exactly one progress-card.svg for roadmap scope")
    if "progress-mini.svg" in readme:
        raise AssertionError("README roadmap scope must not duplicate card + mini")
    if roadmap.count("../assets/readme/progress-mini.svg") != 1:
        raise AssertionError("Docs/ROADMAP.md must embed exactly one progress-mini.svg with the correct relative path")
    if "progress-card.svg" in roadmap:
        raise AssertionError("Roadmap scope must not duplicate mini + card")
    if "progress-template.svg" in readme or "progress-template.svg" in roadmap:
        raise AssertionError("progress-template.svg is a TEMPLATE and must not be embedded as project data")
    if f"{progress.completed} / {progress.total} tasks complete ({progress.percentage_text})" not in readme:
        raise AssertionError("README textual numeric progress fallback is missing or stale")
    if "Release readiness: **NOT READY**" not in readme:
        raise AssertionError("README must keep release readiness separate from roadmap completion")
    if LEGACY_METER_RE.search(readme) or LEGACY_METER_RE.search(roadmap):
        raise AssertionError("legacy text/Unicode progress meter detected; maintained surfaces must be SVG-only")


def verify_template() -> None:
    text = TEMPLATE.read_text(encoding="utf-8")
    ET.parse(TEMPLATE)
    if "TEMPLATE" not in text.upper():
        raise AssertionError("progress-template.svg must be clearly marked TEMPLATE")
    if "N/A" not in text:
        raise AssertionError("progress-template.svg must use N/A rather than fabricated project progress")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate deterministic SWIR Progress SVG PRO assets for GTT.")
    parser.add_argument("--check", action="store_true", help="fail if generated assets, math or embeds are stale")
    args = parser.parse_args()

    self_test_math()
    progress = read_progress()
    expected_card = render_card(progress)
    expected_mini = render_mini(progress)

    if args.check:
        mismatches: list[str] = []
        for path, expected in ((CARD, expected_card), (MINI, expected_mini)):
            if not path.is_file() or path.read_text(encoding="utf-8") != expected:
                mismatches.append(str(path.relative_to(ROOT)))
        if mismatches:
            raise AssertionError("stale generated progress assets: " + ", ".join(mismatches))
        validate_svg(CARD, CARD_TRACK_WIDTH, progress.fraction)
        validate_svg(MINI, MINI_TRACK_WIDTH, progress.fraction)
        verify_template()
        verify_embeddings(progress)
        print(f"SWIR Progress SVG PRO: PASS — {progress.counter_text}, {progress.percentage_text}; release readiness kept separate; legacy meters absent.")
        return 0

    CARD.write_text(expected_card, encoding="utf-8")
    MINI.write_text(expected_mini, encoding="utf-8")
    verify_template()
    verify_embeddings(progress)
    print(f"generated {CARD.relative_to(ROOT)} and {MINI.relative_to(ROOT)} from Docs/ROADMAP.md: {progress.counter_text}, {progress.percentage_text}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, ValueError) as exc:
        print(f"progress SVG verification failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
