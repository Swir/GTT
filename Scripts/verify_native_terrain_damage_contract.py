from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
terrain_cpp = (root / "Source/GTT/Private/Vehicles/GTTFieldmasterNativeTerrainDamage.cpp").read_text(encoding="utf-8")
mud_cpp = (root / "Source/GTT/Private/World/GTTMudZone.cpp").read_text(encoding="utf-8")
traction_cpp = (root / "Source/GTT/Private/Vehicles/GTTNativeStabilitySubsystem.cpp").read_text(encoding="utf-8")
roadmap = (root / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (root / "Docs/PLAYTEST_0.0.59.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.d/0.0.59.md").read_text(encoding="utf-8")

required_header = [
    "void ApplyNativeMudResponse(float DragStrength, float TireWearPerSecond, float DeltaSeconds);",
    "void ApplyNativeImpactDamage(float ImpactSpeedKmh, float DamageScale = 1.0f);",
    "float GetNativeMudSeverity() const;",
    "float GetNativeTerrainGripFactor() const;",
    "LastNativeMudResponseTimeSeconds",
    "LastNativeMudSeverity",
]
for token in required_header:
    if token not in header:
        raise SystemExit(f"Native terrain/damage header contract missing token: {token}")

required_definitions = [
    "AGTTFieldmasterNativePawn::GetNativeMudSeverity() const",
    "AGTTFieldmasterNativePawn::GetNativeTerrainGripFactor() const",
    "AGTTFieldmasterNativePawn::ApplyNativeMudResponse(float DragStrength, float TireWearPerSecond, float DeltaSeconds)",
    "AGTTFieldmasterNativePawn::ApplyNativeImpactDamage(float ImpactSpeedKmh, float DamageScale)",
    "AGTTFieldmasterNativePawn::NotifyHit(",
    "NATIVE_TERRAIN_RESPONSE",
    "NATIVE_IMPACT_DAMAGE",
    "MigrationSnapshot.TireIntegrity",
    "MigrationSnapshot.ConditionPercent",
    "Movement->SetThrottleInput",
    "VehicleMesh->AddForce",
]
for token in required_definitions:
    if token not in terrain_cpp:
        raise SystemExit(f"Native terrain/damage implementation missing token: {token}")

if "Fieldmaster->ApplyNativeMudResponse(DragStrength, TireWearPerSecond, DeltaSeconds);" not in mud_cpp:
    raise SystemExit("Authored mud zone is not connected to Native Fieldmaster terrain response")

# Mud wear must feed the same persistent tire-integrity value consumed by traction control.
for token in ["VehicleState.TireIntegrity", "OutFrontTraction", "OutRearTraction", "traction_risk=%.2f"]:
    if token not in traction_cpp:
        raise SystemExit(f"Native traction controller no longer consumes shared tire state: {token}")

for token in ["mud", "collision", "TireIntegrity", "NATIVE_TERRAIN_RESPONSE", "NATIVE_IMPACT_DAMAGE"]:
    if token not in playtest:
        raise SystemExit(f"0.0.59 playtest missing terrain/damage evidence: {token}")

if "Native Terrain & Collision Contract Repair" not in changelog:
    raise SystemExit("0.0.59 changelog milestone title missing")

if "<!-- SWIR-ROADMAP-STANDARD:v1 -->" not in roadmap:
    raise SystemExit("SWIR roadmap style lock marker missing")
completed = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE))
remaining = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
if (completed, remaining, completed + remaining) != (125, 5, 130):
    raise SystemExit(f"Roadmap unexpectedly changed: {completed}/130 complete, {remaining} remaining")
if "DONE-125%2F130" not in roadmap or "96.2%" not in roadmap or "███████████████████░ 96.2%" not in roadmap:
    raise SystemExit("ROADMAP-PROGRESS dashboard is not synchronized with the checklist")

print("Native terrain / collision implementation contract and roadmap honesty verified.")
