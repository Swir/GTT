#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.41 garage/workshop recovery integration."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def text(path: str) -> str:
    p = ROOT / path
    if not p.is_file():
        raise AssertionError(f"missing required file: {path}")
    return p.read_text(encoding="utf-8")


def require(source: str, needle: str, label: str) -> None:
    if needle not in source:
        raise AssertionError(f"missing {label}: {needle}")


def main() -> int:
    policy = text("Source/GTT/Public/World/GTTGarageServicePolicy.h")
    slot = text("Source/GTT/Private/World/GTTGarageSlotTerminal.cpp")
    office = text("Source/GTT/Private/World/GTTGarageTerminal.cpp")
    workshop = text("Source/GTT/Private/World/GTTServiceTerminal.cpp")
    recovery = text("Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp")
    roadmap = text("Docs/ROADMAP.md")
    readme = text("README.md")

    # One shared policy owns the hard hold. Mobile advisory states intentionally remain dispatchable.
    require(policy, "RequiresWorkshopBeforeDispatch", "shared workshop-hold policy")
    require(policy, 'Snapshot.ServiceStatus == TEXT("TOW")', "TOW hard hold")
    require(policy, 'Snapshot.ServiceStatus == TEXT("IMMOBILE")', "IMMOBILE hard hold")
    if 'Snapshot.ServiceStatus == TEXT("LIMP")' in policy or 'Snapshot.ServiceStatus == TEXT("SERVICE")' in policy:
        raise AssertionError("LIMP/SERVICE must remain advisory, not hard garage holds")

    # Garage dispatch must fail closed before any recall/teleport or economy debit.
    require(slot, "GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(Snapshot)", "garage dispatch hold check")
    require(slot, "WORKSHOP HOLD", "garage hold UX")
    hold_pos = slot.find("GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(Snapshot)", slot.find("Interact_Implementation"))
    recall_pos = min(p for p in (slot.find("RecallToTransform", hold_pos), slot.find("SetActorTransform", hold_pos)) if p >= 0)
    spend_pos = slot.find("SpendCash(RecallServiceCost", hold_pos)
    if not (0 <= hold_pos < recall_pos and 0 <= hold_pos < spend_pos):
        raise AssertionError("workshop hold must be checked before vehicle movement and dispatch charge")
    require(slot, "Damage, fuel and tuning were preserved.", "garage recall preservation contract")

    # Garage office exposes the consequence rather than silently hiding an unavailable slot.
    require(office, "WorkshopHoldCount", "garage office hold count")
    require(office, "repair/service clears TOW/IMMOBILE status", "garage office recovery guidance")

    # Workshop is the only path that clears a hard hold by repairing authoritative native state.
    require(workshop, "GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(FleetSnapshot)", "workshop hold detection")
    require(workshop, "ApplyNativeWorkshopService()", "authoritative native workshop service")
    require(workshop, "WORKSHOP HOLD cleared; garage dispatch is available again.", "hold-clear UX")
    require(workshop, "GameMode->SaveProgress()", "service persistence checkpoint")
    if workshop.find("ApplyNativeWorkshopService()") > workshop.find("WORKSHOP HOLD cleared"):
        raise AssertionError("workshop must apply native service before claiming the hold is cleared")

    # Roadside tow remains damage-preserving and ends at the workshop; the new hold closes recall bypass.
    require(recovery, "destination=WORKSHOP", "roadside workshop destination")
    require(recovery, "serviced=NO", "ordinary tow does not auto-repair")
    require(recovery, "bDamagePreserved", "tow damage preservation proof")
    require(recovery, "bIdentityPreserved", "tow identity preservation proof")

    # SWIR presentation and roadmap mathematics remain unchanged by this gameplay milestone.
    require(roadmap, "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "roadmap structure marker")
    require(roadmap, "../assets/readme/progress-mini.svg", "roadmap progress mini")
    require(roadmap, "| **125** | **5** | **130** | **96.2%** |", "authoritative roadmap count")
    require(readme, "<!-- SWIR-README-STANDARD:v2 -->", "README v2 marker")
    require(readme, "assets/readme/progress-card.svg", "README progress card")
    require(readme, "## 🔎 Search Keywords", "README search keywords")

    # Reject the retired character-art meter in maintained progress dashboards.
    progress_block = roadmap.split("<!-- ROADMAP-PROGRESS:START -->", 1)[1].split("<!-- ROADMAP-PROGRESS:END -->", 1)[0]
    legacy_patterns = [r"[█▓▒░]{4,}", r"\[[#=\-]{5,}\]"]
    if any(re.search(pattern, progress_block) for pattern in legacy_patterns):
        raise AssertionError("legacy text progress meter returned to ROADMAP-PROGRESS")

    print("GTT 0.1.41 garage/workshop recovery integration: PASS")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"GTT 0.1.41 garage/workshop recovery integration: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
