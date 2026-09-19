from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h"
CORE_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp"
ENVIRONMENT_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativeEnvironment.cpp"
OBSOLETE_RUNTIME_CPP = ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawnRuntime.cpp"
MUD_CPP = ROOT / "Source/GTT/Private/World/GTTMudZone.cpp"

errors: list[str] = []

for path in (HEADER, CORE_CPP, ENVIRONMENT_CPP, MUD_CPP):
    if not path.is_file():
        errors.append(f"missing required file: {path.relative_to(ROOT)}")

if OBSOLETE_RUNTIME_CPP.exists():
    errors.append(
        "obsolete GTTFieldmasterNativePawnRuntime.cpp still exists; native environment/runtime methods must have one canonical translation unit"
    )

if errors:
    print("\n".join(f"ERROR: {item}" for item in errors))
    sys.exit(1)

header = HEADER.read_text(encoding="utf-8")
core_cpp = CORE_CPP.read_text(encoding="utf-8")
environment_cpp = ENVIRONMENT_CPP.read_text(encoding="utf-8")
mud_cpp = MUD_CPP.read_text(encoding="utf-8")

fieldmaster_cpp_paths = sorted((ROOT / "Source/GTT/Private/Vehicles").glob("GTTFieldmasterNativePawn*.cpp"))
fieldmaster_cpp_paths += [ENVIRONMENT_CPP]
# Deduplicate while preserving deterministic order.
fieldmaster_cpp_paths = list(dict.fromkeys(fieldmaster_cpp_paths))
translation_units = {path: path.read_text(encoding="utf-8") for path in fieldmaster_cpp_paths}
combined_cpp = "\n".join(translation_units.values())

required_definitions = (
    "AGTTFieldmasterNativePawn::NotifyHit(",
    "AGTTFieldmasterNativePawn::ApplyNativeMudResponse(",
    "AGTTFieldmasterNativePawn::ApplyNativeImpactDamage(",
    "AGTTFieldmasterNativePawn::GetNativeMudSeverity() const",
    "AGTTFieldmasterNativePawn::GetNativeTerrainGripFactor() const",
    "AGTTFieldmasterNativePawn::TryGetRearHitchTransform(",
)

for token in required_definitions:
    owners = [path.relative_to(ROOT).as_posix() for path, text in translation_units.items() if token in text]
    count = sum(text.count(token) for text in translation_units.values())
    if count != 1:
        errors.append(f"expected exactly one repository definition for {token!r}, found {count} in {owners}")
    elif owners != [ENVIRONMENT_CPP.relative_to(ROOT).as_posix()]:
        errors.append(f"{token!r} must be owned by canonical environment unit, found in {owners}")

required_runtime_tokens = (
    "Super::NotifyHit(",
    "ImpactDamageCooldownSeconds",
    "NormalImpulse.Size() / GetMesh()->GetMass()",
    "MigrationSnapshot.ConditionPercent = FMath::Clamp(",
    "MigrationSnapshot.TireIntegrity = FMath::Clamp(",
    "GetMesh()->AddForce(",
    "LastNativeMudResponseTimeSeconds",
    "NativeMudHoldSeconds",
    "Rig.HitchSocket",
    "GetSocketTransform(Rig.HitchSocket, RTS_World)",
    "SyncLegacyMirror();",
)

for token in required_runtime_tokens:
    if token not in environment_cpp:
        errors.append(f"canonical native environment contract missing token {token!r}")

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

if re.search(r"ConditionPercent\s*-\s*[^;\n]*100", environment_cpp):
    errors.append("canonical impact damage appears to use 0..100 condition math")

# Guard the exact ODR/linker class of regression that prompted this hardening pass.
for match in re.finditer(r"\bAGTTFieldmasterNativePawn::([A-Za-z_][A-Za-z0-9_]*)\s*\(", combined_cpp):
    method = match.group(1)
    signature = f"AGTTFieldmasterNativePawn::{method}("
    occurrences = sum(text.count(signature) for text in translation_units.values())
    if occurrences > 1:
        owners = [path.relative_to(ROOT).as_posix() for path, text in translation_units.items() if signature in text]
        errors.append(f"duplicate Fieldmaster out-of-line definition surface for {method}: {occurrences} occurrences in {owners}")

if errors:
    print("Fieldmaster native link/runtime contract FAILED")
    for item in errors:
        print(f" - {item}")
    sys.exit(1)

print("Fieldmaster native link/runtime contract verified.")
print(f"Canonical implementation: {ENVIRONMENT_CPP.relative_to(ROOT)}")
print(f"Definitions verified exactly once: {len(required_definitions)}")
print(f"Translation units scanned for duplicate member definitions: {len(translation_units)}")
