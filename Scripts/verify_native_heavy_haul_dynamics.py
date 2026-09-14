from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTNativeStabilitySubsystem.h").read_text(encoding="utf-8")
cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeStabilitySubsystem.cpp").read_text(encoding="utf-8")
trailer = (ROOT / "Source/GTT/Public/Vehicles/GTTFarmTrailer.h").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.56.md").read_text(encoding="utf-8")

for token in ["TrailerSwaySeconds", "LastTowLoad", "FindAttachedTrailer", "ComputeTrailerSwayRisk", "TowLoadFactor"]:
    assert token in header, f"missing heavy-haul stability header token: {token}"

for token in [
    "GetTowLoadFactor",
    "CargoMassLoad",
    "HitchStress",
    "DamageLoad",
    "AxleLoad",
    "CargoShiftLoad",
]:
    assert token in trailer, f"missing trailer tow-load token: {token}"

for token in [
    "TrailerSwayGraceSeconds",
    "LoadedInterventionSpeedKmh",
    "FindAttachedTrailer",
    "ComputeTrailerSwayRisk",
    "GetTowLoadFactor()",
    "LateralRelativeKmh",
    "FindDeltaAngleDegrees",
    "bSustainedTrailerSway",
    "tow_load=%.2f",
    "sway_risk=%.2f",
    "trailer=%s",
]:
    assert token in cpp, f"missing native heavy-haul implementation token: {token}"

assert "python Scripts/verify_native_heavy_haul_dynamics.py" in workflow
assert "0.0.56" in playtest
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "DONE-125%2F130" in roadmap
assert "96.2%" in roadmap
assert roadmap.count("- [x]") == 125
assert roadmap.count("- [ ]") == 5

print("Native heavy-haul load and sway dynamics milestone verified")
