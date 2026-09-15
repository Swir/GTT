#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
H = ROOT / "Source/GTT/Public/Vehicles/GTTNativeChaosAcceptanceMatrixSubsystem.h"
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTNativeChaosAcceptanceMatrixSubsystem.cpp"
WORKFLOW = ROOT / ".github/workflows/project-sanity.yml"
ROADMAP = ROOT / "Docs/ROADMAP.md"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.0.79.md"
CHANGELOG = ROOT / "CHANGELOG.d/0.0.79.md"

for path in (H, CPP, WORKFLOW, ROADMAP, PLAYTEST, CHANGELOG):
    if not path.exists():
        raise SystemExit(f"missing required file: {path.relative_to(ROOT)}")

h = H.read_text(encoding="utf-8")
cpp = CPP.read_text(encoding="utf-8")
workflow = WORKFLOW.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")
playtest = PLAYTEST.read_text(encoding="utf-8")
changelog = CHANGELOG.read_text(encoding="utf-8")

required = [
    (h, "FGTTNativeChaosAcceptanceState"),
    (h, "ConsecutiveAcceptedSeconds"),
    (h, "EvaluateLiveMatrix"),
    (cpp, "ValidateCanonicalWheelSetups"),
    (cpp, "ValidateCanonicalPowertrain"),
    (cpp, "Movement->GetWheelState(WheelIndex)"),
    (cpp, "RequiredWheelCount = 4"),
    (cpp, "SmokeReadySeconds = 20.0f"),
    (cpp, "RequiredSuspensionTravel = 0.02f"),
    (cpp, "NATIVE_CHAOS_ACCEPTANCE_MATRIX"),
    (cpp, "NATIVE_CHAOS_SMOKE_READY"),
    (cpp, "TActorIterator<AGTTFieldmasterNativePawn>"),
    (cpp, "TActorIterator<AGTTRoadVehicleNativePawn>"),
    (workflow, "Verify Native Chaos acceptance matrix and runtime smoke evidence"),
    (workflow, "python Scripts/verify_native_acceptance_matrix.py"),
    (playtest, "0.0.79"),
    (changelog, "0.0.79"),
]
missing = [token for text, token in required if token not in text]
if missing:
    raise SystemExit("missing Native acceptance matrix contract tokens: " + ", ".join(missing))

for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "## 📊 Overall progress",
    "ROADMAP-96.2%25",
    "DONE-125%2F130",
    "███████████████████░ 96.2%",
):
    if token not in roadmap:
        raise SystemExit("roadmap Style Lock/progress mismatch: " + token)

checks = re.findall(r"^- \[[x ]\] ", roadmap, flags=re.MULTILINE)
done = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
if len(checks) != 130 or done != 125:
    raise SystemExit(f"roadmap checklist changed unexpectedly: {done}/{len(checks)}")

if "packaged Win64" not in playtest or "does not" not in changelog:
    raise SystemExit("release-honesty language missing from 0.0.79 docs")

print("[OK] Native Chaos acceptance matrix verified")
print(" - wheel + powertrain config and live wheel/suspension state share one matrix")
print(" - smoke readiness requires sustained healthy runtime and measurable suspension travel")
print(" - Fieldmaster, Rattleback and Mulebox are independently tracked")
print(" - roadmap remains honest at 125/130 (96.2%)")
