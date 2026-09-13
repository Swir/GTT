from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp").read_text(encoding="utf-8")
statics = (ROOT / "Source/GTT/Private/Core/GTTGameplayStatics.cpp").read_text(encoding="utf-8")
garage = (ROOT / "Source/GTT/Private/World/GTTGarageSlotTerminal.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.45.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")

for token in [
    "IGTTInteractable",
    "TryActivateLegacyTakeover",
    "DeactivateLegacyTakeover",
    "ExitNativeVehicle",
    "RecallToTransform",
    "IsLegacyTakeoverActive",
    "GetDriverPawn",
]:
    assert token in header, f"native takeover API missing {token}"

for token in [
    "TryActivateLegacyTakeover",
    "LegacyVehicle->IsOwnedByPlayer()",
    "LegacyVehicle->IsOccupied()",
    "ImportLegacyGameplayState",
    "SetActorTickEnabled(false)",
    "SyncLegacyMirror",
    "RestorePersistentState",
    "MigrationSnapshot.FuelLiters",
    "SetTargetGear",
]:
    assert token in cpp, f"native takeover implementation missing {token}"

assert "AGTTFieldmasterNativePawn" in statics
assert "ResolveGTTDriverPawn" in statics
assert "FindWantedComponentForPawn" in statics
assert "FindEconomyComponentForPawn" in statics
assert "FindRadioComponentForPawn" in statics

assert "ResolveActiveNativeFieldmaster" in garage
assert "IsLegacyTakeoverActive" in garage
assert "RecallToTransform(Destination)" in garage

assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "ROADMAP-96.2%25" in roadmap
assert "DONE-125%2F130" in roadmap
assert "| **125** | **5** | **130** | **96.2%** |" in roadmap
assert "███████████████████░ 96.2%" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap

assert "Fieldmaster Native Runtime Takeover" in playtest
assert "Win64" in playtest
assert "fallback" in playtest.lower()
assert "Verify Fieldmaster native runtime takeover" in workflow

print("Fieldmaster native runtime takeover sanity passed; roadmap remains 125/130 (96.2%)")
