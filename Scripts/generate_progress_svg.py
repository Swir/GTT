#!/usr/bin/env python3
# SWIR-PROGRESS-SVG-PRO:v1 — deterministic project rendering from Docs/ROADMAP.md
from __future__ import annotations

import argparse
import math
import re
import sys
import xml.etree.ElementTree as ET

ET.register_namespace("", "http://www.w3.org/2000/svg")
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROADMAP = ROOT / "Docs" / "ROADMAP.md"
TEMPLATE = ROOT / "assets" / "readme" / "progress-template.svg"
CARD = ROOT / "assets" / "readme" / "progress-card.svg"
MINI = ROOT / "assets" / "readme" / "progress-mini.svg"

CARD_TRACK_X = 50.0
CARD_TRACK_WIDTH = 1100.0
MINI_TRACK_X = 170.0
MINI_TRACK_WIDTH = 700.0
PROJECT = "Grand Theft Tractor"
SCOPE = "Roadmap completion"
STATUS = "PRE-ALPHA"


@dataclass(frozen=True)
class Progress:
    completed: int
    total: int

    @property
    def fraction(self) -> float | None:
        if self.total <= 0:
            return None
        return self.completed / self.total

    @property
    def percent(self) -> float | None:
        value = self.fraction
        return None if value is None else value * 100.0

    @property
    def percent_text(self) -> str:
        value = self.percent
        return "N/A" if value is None else f"{value:.1f}%"

    @property
    def counter_text(self) -> str:
        return "N/A roadmap items" if self.total <= 0 else f"{self.completed} / {self.total} roadmap items"


def read_progress() -> Progress:
    text = ROADMAP.read_text(encoding="utf-8")
    checked = len(re.findall(r"^\s*-\s*\[x\]", text, flags=re.MULTILINE | re.IGNORECASE))
    unchecked = len(re.findall(r"^\s*-\s*\[ \]", text, flags=re.MULTILINE))
    total = checked + unchecked
    if checked > total:
        raise AssertionError("completed roadmap items exceed total")
    return Progress(checked, total)


def fmt(value: float) -> str:
    if not math.isfinite(value):
        raise AssertionError("non-finite SVG geometry")
    rounded = round(value, 6)
    return str(int(rounded)) if rounded.is_integer() else f"{rounded:.6f}".rstrip("0").rstrip(".")


def load_template() -> ET.ElementTree:
    try:
        tree = ET.parse(TEMPLATE)
    except ET.ParseError as exc:
        raise AssertionError(f"invalid progress template XML: {exc}") from exc
    root = tree.getroot()
    if root.attrib.get("viewBox") != "0 0 1200 180":
        raise AssertionError("progress template viewBox drifted from 1200x180 canonical card")
    return tree


def find_by_id(root: ET.Element, element_id: str) -> ET.Element:
    for element in root.iter():
        if element.attrib.get("id") == element_id:
            return element
    raise AssertionError(f"template missing id={element_id}")


def card_svg(progress: Progress) -> str:
    tree = load_template()
    root = tree.getroot()
    title = find_by_id(root, "progress-title")
    desc = find_by_id(root, "progress-desc")
    title.text = f"{PROJECT} — {SCOPE}: {progress.percent_text}"
    desc.text = (
        f"{PROJECT}. Measured scope: {SCOPE}. Status: {STATUS}. "
        f"Verified counter: {progress.counter_text}. Progress: {progress.percent_text}. "
        "This graphic measures roadmap checklist completion, not demo or release readiness."
    )
    find_by_id(root, "project-name").text = PROJECT
    find_by_id(root, "scope-label").text = f"Measured scope: {SCOPE}"
    find_by_id(root, "status-label").text = STATUS
    find_by_id(root, "counter-label").text = progress.counter_text
    find_by_id(root, "percent-label").text = progress.percent_text
    find_by_id(root, "release-note").text = "Demo / release readiness is a separate runtime + visual gate."
    fraction = progress.fraction
    width = 0.0 if fraction is None else CARD_TRACK_WIDTH * max(0.0, min(1.0, fraction))
    fill = find_by_id(root, "progress-fill")
    fill.set("width", fmt(width))
    if width <= 0.0:
        fill.attrib.pop("filter", None)
    ET.indent(tree, space="  ")
    body = ET.tostring(root, encoding="unicode")
    return '<?xml version="1.0" encoding="UTF-8"?>\n' + body + "\n"


def mini_svg(progress: Progress) -> str:
    fraction = progress.fraction
    width = 0.0 if fraction is None else MINI_TRACK_WIDTH * max(0.0, min(1.0, fraction))
    glow = '' if width <= 0.0 else ' filter="url(#soft-glow)"'
    title = f"{PROJECT} — {SCOPE}: {progress.percent_text}"
    desc = (
        f"Status {STATUS}. {progress.counter_text}. Progress {progress.percent_text}. "
        "Roadmap completion only; release readiness is separate."
    )
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="900" height="72" viewBox="0 0 900 72" role="img" aria-labelledby="mini-title mini-desc">
  <title id="mini-title">{title}</title>
  <desc id="mini-desc">{desc}</desc>
  <defs>
    <linearGradient id="panel" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#02050A"/><stop offset="1" stop-color="#07111C"/></linearGradient>
    <linearGradient id="bar" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#0088FF"/><stop offset="1" stop-color="#62E5FF"/></linearGradient>
    <filter id="soft-glow" x="-20%" y="-100%" width="140%" height="300%"><feGaussianBlur stdDeviation="3" result="blur"/><feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge></filter>
    <clipPath id="mini-track-clip"><rect x="170" y="43" width="700" height="12" rx="6"/></clipPath>
  </defs>
  <rect x="1" y="1" width="898" height="70" rx="16" fill="url(#panel)" stroke="#62E5FF" stroke-opacity="0.32" stroke-width="2"/>
  <text x="24" y="29" fill="#F4FAFF" font-family="Segoe UI,Arial,sans-serif" font-size="15" font-weight="700">GTT · {SCOPE}</text>
  <text x="876" y="29" text-anchor="end" fill="#62E5FF" font-family="Segoe UI,Arial,sans-serif" font-size="14" font-weight="700">{STATUS} · {progress.percent_text}</text>
  <text x="24" y="54" fill="#8DA8B8" font-family="Segoe UI,Arial,sans-serif" font-size="12">{progress.counter_text}</text>
  <rect x="170" y="43" width="700" height="12" rx="6" fill="#0A1A28" stroke="#62E5FF" stroke-opacity="0.18"/>
  <rect id="progress-fill" x="170" y="43" width="{fmt(width)}" height="12" rx="6" fill="url(#bar)" clip-path="url(#mini-track-clip)"{glow}/>
</svg>
'''


def validate_svg(path: Path, expected_width: float, track_width: float) -> None:
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as exc:
        raise AssertionError(f"{path}: invalid XML: {exc}") from exc
    if "viewBox" not in root.attrib:
        raise AssertionError(f"{path}: missing viewBox")
    fill = find_by_id(root, "progress-fill")
    width = float(fill.attrib["width"])
    if not math.isfinite(width) or width < 0 or width > track_width + 1e-6:
        raise AssertionError(f"{path}: fill width out of bounds: {width}")
    if abs(width - expected_width) > 1e-3:
        raise AssertionError(f"{path}: stale fill width {width}; expected {expected_width}")


def check_edges() -> None:
    cases = [Progress(0, 10), Progress(10, 10), Progress(1, 13), Progress(0, 0)]
    expected = [0.0, CARD_TRACK_WIDTH, CARD_TRACK_WIDTH / 13.0, 0.0]
    for case, expected_width in zip(cases, expected):
        fraction = case.fraction
        width = 0.0 if fraction is None else CARD_TRACK_WIDTH * fraction
        if abs(width - expected_width) > 1e-6:
            raise AssertionError(f"edge-case geometry mismatch for {case}")
        if case.total == 0 and case.percent_text != "N/A":
            raise AssertionError("unknown denominator must render N/A")
        if case.completed < case.total and case.percent_text == "100.0%":
            raise AssertionError("incomplete scope rounded to 100.0%")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate/check SWIR Progress SVG PRO assets from Docs/ROADMAP.md")
    parser.add_argument("--check", action="store_true", help="Fail if committed SVGs differ from deterministic output")
    args = parser.parse_args()

    progress = read_progress()
    check_edges()
    expected_card = card_svg(progress)
    expected_mini = mini_svg(progress)

    if args.check:
        for path, expected in ((CARD, expected_card), (MINI, expected_mini)):
            if not path.is_file():
                raise AssertionError(f"missing generated SVG: {path.relative_to(ROOT)}")
            actual = path.read_text(encoding="utf-8")
            if actual != expected:
                raise AssertionError(f"stale generated SVG: {path.relative_to(ROOT)}")
        fraction = progress.fraction
        card_width = 0.0 if fraction is None else CARD_TRACK_WIDTH * fraction
        mini_width = 0.0 if fraction is None else MINI_TRACK_WIDTH * fraction
        validate_svg(CARD, card_width, CARD_TRACK_WIDTH)
        validate_svg(MINI, mini_width, MINI_TRACK_WIDTH)
        load_template()
        print(f"SWIR Progress SVG PRO: PASS — {progress.completed}/{progress.total} ({progress.percent_text})")
        return 0

    CARD.parent.mkdir(parents=True, exist_ok=True)
    CARD.write_text(expected_card, encoding="utf-8", newline="\n")
    MINI.write_text(expected_mini, encoding="utf-8", newline="\n")
    print(f"Generated progress SVGs from Docs/ROADMAP.md: {progress.completed}/{progress.total} ({progress.percent_text})")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
