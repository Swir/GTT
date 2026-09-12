#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
required = {
    "Source/GTT/Public/Missions/GTTArc4Director.h": ["EGTTArc4Stage", "ProveGround", "PrepareContraband", "NorthPass", "EscapePolice", "RidgeExchange", "GTT_MainStory_Arc4_01"],
    "Source/GTT/Private/Missions/GTTArc4Director.cpp": ["GetFactionVictories", "GetContrabandUnits", "bContrabandPrepared", "NorthPassHeat", "AddHeat", "NEXT ROAD", "ARCS 1-4 COMPLETE"],
    "Source/GTT/Public/Save/GTTArc4Save.h": ["Arc4Stage", "StartingFactionVictories", "bContrabandPrepared"],
    "Source/GTT/Private/World/GTTArc4WorldSubsystem.cpp": ["NORTH PASS", "RIDGE EXCHANGE", "SpawnActor<AGTTArc4Director>"],
    "Source/GTT/Private/World/GTTRoadGraph.cpp": ["NorthPassApproach", "NORTH PASS CHECKPOINT", "RiverFord", "RIDGE EXCHANGE", "QuarryNorth"],
}
for rel, tokens in required.items():
    path = ROOT / rel
    if not path.is_file():
        raise SystemExit(f"[FAIL] missing {rel}")
    text = path.read_text(encoding="utf-8")
    missing = [t for t in tokens if t not in text]
    if missing:
        raise SystemExit(f"[FAIL] {rel} missing hooks: {missing}")

road = (ROOT / "Source/GTT/Private/World/GTTRoadGraph.cpp").read_text(encoding="utf-8")
node_rows = re.findall(r'\{TEXT\("[^"]+"\), TEXT\("[^"]+"\), FVector\([^\)]+\),\s*([0-9.]+)f,\s*(\d+),\s*(true|false)', road)
if len(node_rows) < 28:
    raise SystemExit(f"[FAIL] expected >=28 shared road nodes after North Pass expansion, found {len(node_rows)}")

roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap or "ROADMAP-PROGRESS:START" not in roadmap:
    raise SystemExit("[FAIL] roadmap style lock/dashboard marker missing")
checks = re.findall(r'^- \[(x| )\]', roadmap, flags=re.M)
done = sum(1 for x in checks if x == 'x')
total = len(checks)
if (done, total) != (113, 130):
    raise SystemExit(f"[FAIL] roadmap checkbox count expected 113/130, got {done}/{total}")
if "DONE-113%2F130" not in roadmap or "86.9%" not in roadmap or "█████████████████░░░ 86.9%" not in roadmap:
    raise SystemExit("[FAIL] roadmap dashboard does not match 113/130 = 86.9%")
print("[OK] GTT 0.0.23 Arc 4, faction/fence/police integration and North Pass countryside look structurally sane.")
