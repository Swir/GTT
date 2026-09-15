from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTNativeChaosRuntimeGuardSubsystem.h").read_text(encoding="utf-8")
cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeChaosRuntimeGuardSubsystem.cpp").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.52.md").read_text(encoding="utf-8")

required_header = [
    "UTickableWorldSubsystem",
    "EvaluateFieldmaster",
    "InvalidRuntimeSeconds",
    "EvidenceLogSeconds",
]
required_cpp = [
    "NATIVE_CHAOS_RUNTIME_ACCEPTANCE",
    "InvalidRuntimeGraceSeconds",
    "RuntimeEvidenceIntervalSeconds",
    "IsLegacyTakeoverActive",
    "IsNativeFieldmasterReady",
    "GetVehicleMovementComponent",
    "GetPhysicsAsset",
    "GetCollisionEnabled",
    "Movement->IsActive()",
    "DeactivateLegacyTakeover",
]

for token in required_header:
    assert token in header, f"missing runtime-guard header token: {token}"
for token in required_cpp:
    assert token in cpp, f"missing runtime-guard implementation token: {token}"

assert "python Scripts/verify_native_chaos_runtime_guard.py" in workflow
assert "0.0.52" in playtest
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "DONE-125%2F130" in roadmap
assert "96.2%" in roadmap
assert roadmap.count("- [ ]") == 5, "native runtime guard must not falsely close runtime-only roadmap work"

print("Native Chaos runtime guard milestone verified")
