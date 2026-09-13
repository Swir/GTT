from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
trailer_h = (ROOT / "Source/GTT/Public/Vehicles/GTTFarmTrailer.h").read_text(encoding="utf-8")
trailer_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTFarmTrailer.cpp").read_text(encoding="utf-8")
haul_h = (ROOT / "Source/GTT/Public/Activities/GTTHeavyHaulDirector.h").read_text(encoding="utf-8")
haul_cpp = (ROOT / "Source/GTT/Private/Activities/GTTHeavyHaulDirector.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.50.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.0.50.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")

for token in ["PerformRoadsideRepair", "GetLostWheelCount", "RestoreWheel"]:
    assert token in trailer_h, f"trailer recovery API missing {token}"
for token in [
    "RestoreWheel(LeftWheel",
    "RestoreWheel(RightWheel",
    "TrailerIntegrity = FMath::Clamp",
    "AttachToNativeFieldmaster(Native)",
    "AttachToVehicle(Legacy)",
]:
    assert token in trailer_cpp, f"trailer recovery behavior missing {token}"

for token in ["TryRoadsideRepair", "RoadsideRepairBaseCost", "RoadsideRepairEscalation", "RoadsideRepairCount"]:
    assert token in haul_h, f"heavy-haul recovery API missing {token}"
for token in [
    "Economy->SpendCash",
    "Trailer->PerformRoadsideRepair()",
    "Economy->AddCash(RepairCost, TEXT(\"Roadside repair refund\"))",
    "TimeRemaining = FMath::Max(0.0f, TimeRemaining - 22.0f)",
    "Trailer->GetLostWheelCount() * 80",
    "ROADSIDE REPAIR AVAILABLE",
]:
    assert token in haul_cpp, f"heavy-haul recovery integration missing {token}"

assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "ROADMAP-96.2%25" in roadmap
assert "DONE-125%2F130" in roadmap
assert "| **125** | **5** | **130** | **96.2%** |" in roadmap
assert "███████████████████░ 96.2%" in roadmap
assert "- [ ] Authored skeletal trailer wheel assets and final hitch sockets" in roadmap
assert "Heavy-Haul Roadside Recovery" in playtest
assert "Heavy-Haul Roadside Recovery" in changelog
assert "125/130 (96.2%)" in changelog
assert "Verify trailer roadside recovery" in workflow

print("Trailer roadside recovery sanity passed; roadmap remains 125/130 (96.2%)")
