from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/GTT/Private/Vehicles/GTTNativeStabilitySubsystem.cpp").read_text(encoding="utf-8")
header = (root / "Source/GTT/Public/Vehicles/GTTNativeStabilitySubsystem.h").read_text(encoding="utf-8")
pawn_header = (root / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
roadmap = (root / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (root / "Docs/PLAYTEST_0.0.58.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.d/0.0.58.md").read_text(encoding="utf-8")

required_cpp = [
    "TractionLossGraceSeconds",
    "ComputeTractionRisk",
    "TractionLossSeconds",
    "GetRequestedThrottleInput",
    "OutSlipAngleDegrees",
    "OutFrontTraction",
    "OutRearTraction",
    "traction_risk=%.2f",
    "slip_deg=%.1f",
    "front_traction=%.2f",
    "rear_traction=%.2f",
    "traction_control=%s",
    "throttle_limit=%.2f",
    "Movement->SetThrottleInput(RequestedThrottle * ThrottleLimit)",
]
for token in required_cpp:
    if token not in cpp:
        raise SystemExit(f"Native traction implementation missing token: {token}")

for token in ["LastTractionRisk", "LastSlipAngleDegrees", "LastFrontTraction", "LastRearTraction"]:
    if token not in header:
        raise SystemExit(f"Native traction state contract missing token: {token}")

if "GetRequestedThrottleInput() const" not in pawn_header:
    raise SystemExit("Native Fieldmaster does not expose requested throttle for traction limiting")

for token in ["traction_risk", "slip_deg", "front_traction", "rear_traction", "traction_control", "throttle_limit"]:
    if token not in playtest:
        raise SystemExit(f"0.0.58 playtest missing runtime evidence field: {token}")

if "Native Traction & Slip Control" not in changelog:
    raise SystemExit("0.0.58 changelog milestone title missing")

if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
    raise SystemExit("SWIR roadmap style lock marker missing")
completed = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
remaining = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
if (completed, remaining, completed + remaining) != (125, 5, 130):
    raise SystemExit(f"Roadmap unexpectedly changed: {completed}/130 complete, {remaining} remaining")
if "DONE-125%2F130" not in roadmap or "96.2%" not in roadmap or "███████████████████░ 96.2%" not in roadmap:
    raise SystemExit("ROADMAP-PROGRESS dashboard is not synchronized with the checklist")

print("Native traction / slip / axle-grip control milestone verified.")
