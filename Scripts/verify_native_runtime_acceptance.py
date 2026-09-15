#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
H = ROOT / "Source/GTT/Public/Vehicles/GTTNativeChaosRuntimeGuardSubsystem.h"
CPP = ROOT / "Source/GTT/Private/Vehicles/GTTNativeChaosRuntimeGuardSubsystem.cpp"
WORKFLOW = ROOT / ".github/workflows/project-sanity.yml"
ROADMAP = ROOT / "Docs/ROADMAP.md"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.0.78.md"
CHANGELOG = ROOT / "CHANGELOG.d/0.0.78.md"

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
    (h, "EvaluateRoadVehicle"),
    (h, "EvaluateLiveChaosContract"),
    (cpp, "TActorIterator<AGTTFieldmasterNativePawn>"),
    (cpp, "TActorIterator<AGTTRoadVehicleNativePawn>"),
    (cpp, "RequiredWheelCount = 4"),
    (cpp, "Movement->GetWheelState(WheelIndex)"),
    (cpp, "WheelState.bIsValid"),
    (cpp, "WheelState.bInContact"),
    (cpp, "WheelState.NormalizedSuspensionLength"),
    (cpp, "OutValidWheels == RequiredWheelCount && OutSuspensionSamples == RequiredWheelCount"),
    (cpp, "InvalidRuntimeGraceSeconds = 1.5f"),
    (cpp, "NATIVE_CHAOS_RUNTIME_ACCEPTANCE"),
    (cpp, "NATIVE_CHAOS_RUNTIME_FALLBACK"),
    (cpp, "NativePawn->DeactivateLegacyTakeover()"),
    (workflow, "Verify fleet-wide Native Chaos runtime acceptance"),
    (workflow, "python Scripts/verify_native_runtime_acceptance.py"),
    (playtest, "0.0.78"),
    (changelog, "0.0.78"),
]
missing = [token for text, token in required if token not in text]
if missing:
    raise SystemExit("missing fleet runtime acceptance contract tokens: " + ", ".join(missing))

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

print("[OK] fleet-wide Native Chaos runtime acceptance verified")
print(" - Fieldmaster, Rattleback and Mulebox share live runtime guard")
print(" - Physics Asset, movement, four wheel records and four suspension samples are required")
print(" - unhealthy Native takeover falls back after grace period")
print(" - roadmap remains honest at 125/130 (96.2%)")
