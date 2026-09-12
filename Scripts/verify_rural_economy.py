#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

required = {
    "Source/GTT/Public/Save/GTTRuralEconomySave.h": ["ContrabandUnits", "bInsuranceActive", "ImpoundedVehicleId", "SpeedingCitations"],
    "Source/GTT/Public/World/GTTRuralEconomySubsystem.h": ["GTT_RuralEconomy_01", "SellContraband", "BuyOrUseInsurance", "ReleaseImpoundedVehicle", "HandleArrestImpound"],
    "Source/GTT/Private/World/GTTRuralEconomySubsystem.cpp": [
        "BACKLOT FENCE", "FARM MUTUAL", "COUNTY IMPOUND", "SPEEDING", "RECKLESS SPEED",
        "GetSpeedLimitAtLocation", "RecallToTransform", "SaveGameToSlot"
    ],
    "Source/GTT/Private/Economy/GTTPlayerEconomyComponent.cpp": ["Reason.StartsWith(TEXT(\"ARRESTED\"))", "HandleArrestImpound"],
    "Source/GTT/Private/Activities/GTTForestPoachingSpot.cpp": ["AddContraband", "fence value", "WARDEN ALERT"],
    "Source/GTT/Public/World/GTTRoadGraph.h": ["SpeedLimitKmh", "LaneCount", "bPriorityRoad", "GetSpeedLimitAtLocation"],
    "Source/GTT/Private/World/GTTRoadGraph.cpp": ["45.0f", "70.0f", "GetLaneCountAtLocation", "IsPriorityRoadAtLocation"],
    "Source/GTT/Public/World/GTTRuralEconomyTerminal.h": ["Fence", "Insurance", "Impound"],
    "Source/GTT/Private/World/GTTRuralEconomyTerminal.cpp": ["SellContraband", "BuyOrUseInsurance", "ReleaseImpoundedVehicle"],
}

for rel, tokens in required.items():
    path = ROOT / rel
    if not path.is_file():
        raise SystemExit(f"[FAIL] missing {rel}")
    text = path.read_text(encoding="utf-8")
    missing = [token for token in tokens if token not in text]
    if missing:
        raise SystemExit(f"[FAIL] {rel} missing hooks: {missing}")

road = (ROOT / "Source/GTT/Private/World/GTTRoadGraph.cpp").read_text(encoding="utf-8")
node_rows = re.findall(r'\{TEXT\("[^"]+"\), TEXT\("[^"]+"\), FVector\([^\)]+\),\s*([0-9.]+)f,\s*(\d+),\s*(true|false)', road)
if len(node_rows) < 23:
    raise SystemExit(f"[FAIL] expected authored road-law metadata on >=23 nodes, found {len(node_rows)}")
limits = [float(row[0]) for row in node_rows]
if min(limits) >= max(limits):
    raise SystemExit("[FAIL] road speed limits are not differentiated")
if not any(row[1] == "1" for row in node_rows) or not any(row[1] == "2" for row in node_rows):
    raise SystemExit("[FAIL] road lane metadata lacks one/two-lane differentiation")

# The rural-economy regression suite must validate the dashboard contract without
# freezing the repository to the 0.0.22 checkbox count forever.
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap or "ROADMAP-PROGRESS:START" not in roadmap:
    raise SystemExit("[FAIL] SWIR roadmap dashboard standard missing")
checks = re.findall(r'^- \[(x| )\]', roadmap, flags=re.M)
done = sum(1 for state in checks if state == "x")
total = len(checks)
if total == 0:
    raise SystemExit("[FAIL] roadmap has no checklist tasks")
percent = round(done * 100.0 / total, 1)
expected_done_badge = f"DONE-{done}%2F{total}"
expected_percent = f"{percent:.1f}%"
if expected_done_badge not in roadmap or expected_percent not in roadmap:
    raise SystemExit(f"[FAIL] roadmap dashboard is inconsistent with checklist: expected {done}/{total} = {percent:.1f}%")

print("[OK] GTT rural economy, insurance, impound, road law and live roadmap dashboard contract look structurally sane.")
