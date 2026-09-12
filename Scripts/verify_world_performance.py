#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

required = {
    "Source/GTT/Public/World/GTTWorldPerformanceSubsystem.h": [
        "EGTTWorldSimulationTier", "Critical", "Near", "Mid", "Far", "Dormant",
        "GetRecommendedTickInterval", "AllowsExpensiveQueries",
        "CriticalRadius = 2500.0f", "NearRadius = 6000.0f",
        "MidRadius = 11000.0f", "FarRadius = 17000.0f"
    ],
    "Source/GTT/Private/World/GTTWorldPerformanceSubsystem.cpp": [
        "0.05f", "0.20f", "0.65f", "1.50f", "EGTTWorldSimulationTier::Dormant"
    ],
    "Source/GTT/Private/NPC/GTTCitizenPawn.cpp": [
        "GTTWorldPerformanceSubsystem", "bUrgentSimulation", "SetActorTickInterval",
        "EGTTWorldSimulationTier::Dormant", "CombatTarget.IsValid()", "IsFactionHostile()"
    ],
    "Source/GTT/Private/Traffic/GTTTrafficCarPawn.cpp": [
        "GTTWorldPerformanceSubsystem", "GetRecommendedTickInterval", "AllowsExpensiveQueries",
        "bAllowExpensiveQueries", "LineTraceSingleByChannel", "FMath::Min(BudgetInterval, 0.35f)"
    ],
    "Docs/PLAYTEST_0.0.25.md": ["World Performance", "traffic", "combat", "distant"],
    "CHANGELOG.md": ["[0.0.25]", "World Performance", "distance-based", "Critical / Near / Mid / Far / Dormant"],
    ".github/workflows/project-sanity.yml": ["Verify world performance milestone", "verify_world_performance.py"],
}

for rel, tokens in required.items():
    path = ROOT / rel
    if not path.is_file():
        raise SystemExit(f"[FAIL] missing {rel}")
    text = path.read_text(encoding="utf-8")
    missing = [token for token in tokens if token not in text]
    if missing:
        raise SystemExit(f"[FAIL] {rel} missing hooks: {missing}")

roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
    raise SystemExit("[FAIL] SWIR roadmap style marker missing")
if "ROADMAP-PROGRESS:START" not in roadmap or "ROADMAP-PROGRESS:END" not in roadmap:
    raise SystemExit("[FAIL] roadmap progress block missing")
if "- [x] Performance passes" not in roadmap:
    raise SystemExit("[FAIL] performance roadmap milestone not checked")

checks = re.findall(r'^- \[(x| )\]', roadmap, flags=re.M)
done = sum(state == "x" for state in checks)
total = len(checks)
remaining = total - done
percent = round(done * 100.0 / total, 1)
filled = round(done * 20.0 / total)
bar = "█" * filled + "░" * (20 - filled)
expect = [
    f"ROADMAP-{percent:.1f}%25", f"DONE-{done}%2F{total}",
    f"{bar} {percent:.1f}%", f"**{done}**", f"**{remaining}**", f"**{total}**", f"**{percent:.1f}%**"
]
missing = [token for token in expect if token not in roadmap]
if missing:
    raise SystemExit(f"[FAIL] roadmap dashboard mismatch for {done}/{total}={percent:.1f}%: {missing}")

print(f"[OK] GTT 0.0.25 world performance milestone sane; roadmap {done}/{total} = {percent:.1f}% ({filled}/20 cells).")
