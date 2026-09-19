from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h"
CORE_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp"
RUNTIME_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawnRuntime.cpp"
MUD_CPP = ROOT / "Source/GTT/Private/World/GTTMudZone.cpp"

errors: list[str] = []

for path in (HEADER, CORE_CPP, RUNTIME_CPP, MUD_CPP):
    if not path.is_file():
        errors.append(f"missing required file: {path.relative_to(ROOT)}")

if errors:
    print("\n".join(f"ERROR: {item}" for item in errors))
    sys.exit(1)

header = HEADER.read_text(encoding="utf-8")
core_cpp = CORE_CPP.read_text(encoding="utf-8")
runtime_cpp = RUNTIME_CPP.read_text(encoding="utf-8")
mud_cpp = MUD_CPP.read_text(encoding="utf-8")
combined_cpp = core_cpp + "\n" + runtime_cpp

required_definitions = (
    "AGTTFieldmasterNativePawn::NotifyHit(",
    "AGTTFieldmasterNativePawn::ApplyNativeMudResponse(",
    "AGTTFieldmasterNativePawn::ApplyNativeImpactDamage(",
    "AGTTFieldmasterNativePawn::GetNativeMudSeverity() const",
    "AGTTFieldmasterNativePawn::GetNativeTerrainGripFactor() const",
    "AGTTFieldmasterNativePawn::TryGetRearHitchTransform(",
)

for token in required_definitions:
    count = combined_cpp.count(token)
    if count != 1:
        errors.append(f"expected exactly one definition for {token!r}, found {count}")

required_runtime_tokens = (
    "Super::NotifyHit(",
    "NativeImpactCooldownSeconds",
    "NormalImpulse.Size() / MassKg",
    "MigrationSnapshot.ConditionPercent = FMath::Clamp(",
    "MigrationSnapshot.TireIntegrity = FMath::Clamp(",
    "VehicleMesh->AddForce(",
    "TireReinforcement",
    "LastNativeMudResponseTimeSeconds",
    "NativeMudResponseHoldSeconds",
    "Rig.HitchSocket",
    "GetSocketTransform(Rig.HitchSocket, RTS_World)",
    "SyncLegacyMirror();",
)

for token in required_runtime_tokens:
    if token not in runtime_cpp:
        errors.append(f"runtime contract missing token {token!r}")

header_contract = (
    "virtual void NotifyHit(",
    "void ApplyNativeMudResponse(",
    "void ApplyNativeImpactDamage(",
    "float GetNativeMudSeverity() const;",
    "float GetNativeTerrainGripFactor() const;",
    "bool TryGetRearHitchTransform(FTransform& OutTransform) const;",
)

for token in header_contract:
    if token not in header:
        errors.append(f"header contract missing token {token!r}")

if "Fieldmaster->ApplyNativeMudResponse(DragStrength, TireWearPerSecond, DeltaSeconds);" not in mud_cpp:
    errors.append("mud-zone native Fieldmaster bridge is missing")

if re.search(r"ConditionPercent\s*-\s*[^;\n]*100", runtime_cpp):
    errors.append("runtime impact damage appears to use 0..100 condition math")

if errors:
    print("\n".join(f"ERROR: {item}" for item in errors))
    sys.exit(1)

print("Fieldmaster native link/runtime contract verified.")
print(f"Definitions verified: {len(required_definitions)}")
