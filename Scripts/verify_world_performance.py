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
for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "<!-- ROADMAP-PROGRESS:START -->", "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress", "../assets/readme/progress-mini.svg",
):
    if token not in roadmap:
        raise SystemExit(f"[FAIL] roadmap structure/progress missing: {token}")
if "- [x] Performance passes" not in roadmap:
    raise SystemExit("[FAIL] performance roadmap milestone not checked")

checks = re.findall(r'^- \[(x| )\]', roadmap, flags=re.M | re.IGNORECASE)
done = sum(state.lower() == "x" for state in checks)
total = len(checks)
remaining = total - done
percent = round(done * 100.0 / total, 1)
expect = [
    f"ROADMAP-{percent:.1f}%25", f"DONE-{done}%2F{total}",
    f"| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |",
]
missing = [token for token in expect if token not in roadmap]
if missing:
    raise SystemExit(f"[FAIL] roadmap dashboard mismatch for {done}/{total}={percent:.1f}%: {missing}")
if roadmap.count("../assets/readme/progress-mini.svg") != 1:
    raise SystemExit("[FAIL] roadmap must embed exactly one progress-mini.svg")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    raise SystemExit("[FAIL] legacy text/Unicode roadmap progress meter must not return")

print(f"[OK] GTT 0.0.25 world performance milestone sane; roadmap {done}/{total} = {percent:.1f}% with SVG-only presentation.")
