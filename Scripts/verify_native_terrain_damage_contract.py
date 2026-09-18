from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
environment_cpp = (root / "Source/GTT/Private/Vehicles/GTTFieldmasterNativeEnvironment.cpp").read_text(encoding="utf-8")
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
    "GetMesh()->AddForce",
    "ImpactDamageCooldownSeconds",
]
for token in required_definitions:
    if token not in environment_cpp:
        raise SystemExit(f"Native terrain/damage implementation missing token: {token}")

if "Fieldmaster->ApplyNativeMudResponse(DragStrength, TireWearPerSecond, DeltaSeconds);" not in mud_cpp:
    raise SystemExit("Authored mud zone is not connected to Native Fieldmaster terrain response")

# Mud wear writes the same persistent tire-integrity value consumed by the 0.0.58 traction controller.
for token in ["VehicleState.TireIntegrity", "OutFrontTraction", "OutRearTraction", "traction_risk=%.2f"]:
    if token not in traction_cpp:
        raise SystemExit(f"Native traction controller no longer consumes shared tire state: {token}")

for token in ["mud", "collision", "TireIntegrity", "NATIVE_TERRAIN_RESPONSE", "NATIVE_IMPACT_DAMAGE"]:
    if token not in playtest:
        raise SystemExit(f"0.0.59 playtest missing terrain/damage evidence: {token}")

if "Native Terrain Grip & Collision Consequences" not in changelog:
    raise SystemExit("0.0.59 changelog milestone title missing")

for token in (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->",
    "<!-- ROADMAP-PROGRESS:START -->",
    "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress",
    "../assets/readme/progress-mini.svg",
):
    if token not in roadmap:
        raise SystemExit("SWIR roadmap SVG-only presentation missing: " + token)
completed = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
remaining = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
if (completed, remaining, completed + remaining) != (125, 5, 130):
    raise SystemExit(f"Roadmap unexpectedly changed: {completed}/130 complete, {remaining} remaining")
if "DONE-125%2F130" not in roadmap or "ROADMAP-96.2%25" not in roadmap or "| **125** | **5** | **130** | **96.2%** |" not in roadmap:
    raise SystemExit("ROADMAP-PROGRESS numeric dashboard is not synchronized with the checklist")
if roadmap.count("../assets/readme/progress-mini.svg") != 1:
    raise SystemExit("Roadmap must embed exactly one canonical progress-mini.svg")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    raise SystemExit("Legacy text/Unicode roadmap progress meter must not return")

print("Native terrain grip / collision consequences and roadmap honesty verified with SVG-only progress presentation.")