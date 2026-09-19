#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing required file: {rel}")
    return path.read_text(encoding="utf-8")


def require(text: str, needle: str, where: str) -> None:
    if needle not in text:
        raise AssertionError(f"{where}: missing contract token {needle!r}")


header = read("Source/GTT/Public/World/GTTWorkshopJobBoardTerminal.h")
source = read("Source/GTT/Private/World/GTTWorkshopJobBoardTerminal.cpp")
garage = read("Source/GTT/Private/World/GTTGarageTerminal.cpp")
playtest = read("Docs/PLAYTEST_0.1.50.md")
fragment = read("CHANGELOG.d/0.1.50.md")
roadmap = read("Docs/ROADMAP.md")

require(header, "AGTTWorkshopJobBoardTerminal", "job-board header")
require(header, "PendingCancelVehicleId", "job-board header")
require(header, "CancelConfirmationSeconds", "job-board header")

for token in (
    "GetQueueSnapshots()",
    "CancelQueuedRepair",
    "WORKSHOP_JOB_BOARD_CANCEL_ARMED",
    "WORKSHOP_JOB_BOARD_CANCEL_CONFIRMED",
    "board_authority=PRESENTATION_ONLY",
    "two-step cancel",
):
    require(source, token, "job-board source")

# The board may inspect and cancel queue entries, but must never become a second economy/repair authority.
for forbidden in ("SpendCash(", "ApplyNativeWorkshopService(", "AddCash("):
    if forbidden in source:
        raise AssertionError(f"job-board source must not own economy/repair mutation: found {forbidden}")

require(garage, '#include "World/GTTWorkshopJobBoardTerminal.h"', "garage integration")
require(garage, "SpawnActor<AGTTWorkshopJobBoardTerminal>", "garage integration")
require(garage, "HasNearbyWorkshopJobBoard", "garage integration")
require(garage, "WORKSHOP JOB BOARD", "garage integration")

require(playtest, "48-case matrix", "playtest")
require(playtest, "No packaged Win64 proof is claimed", "playtest")
require(fragment, "0.1.50", "changelog fragment")
require(fragment, "two-step", "changelog fragment")

checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
un = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
total = checked + un
if (checked, un, total) != (125, 5, 130):
    raise AssertionError(f"roadmap math drifted: {checked}/{total}, remaining {un}")
if "| **125** | **5** | **130** | **96.2%** |" not in roadmap:
    raise AssertionError("roadmap progress table is not synchronized to 125/130 = 96.2%")

legacy_meter_patterns = [
    r"[█▓▒░]{4,}",
    r"(?:■|□){4,}",
    r"(?:▰|▱){4,}",
]
for pattern in legacy_meter_patterns:
    if re.search(pattern, roadmap):
        raise AssertionError(f"legacy text progress meter returned to roadmap: {pattern}")

print("GTT 0.1.50 workshop job board sanity: PASS")
print("- operational board reads authoritative multi-vehicle queue snapshots")
print("- cancellation is exact-ID and requires a timed two-interaction confirmation")
print("- board owns no cash or repair mutation")
print("- garage spawns only one nearby source-built fallback board")
print("- roadmap remains 125/130 = 96.2% with no legacy text meter")
