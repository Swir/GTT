from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp").read_text(encoding="utf-8")
statics = (ROOT / "Source/GTT/Private/Core/GTTGameplayStatics.cpp").read_text(encoding="utf-8")
garage = (ROOT / "Source/GTT/Private/World/GTTGarageSlotTerminal.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.45.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")

# Native pawn is now a real interactable lifecycle participant, not only a setup shell.
for token in [
    "public AWheeledVehiclePawn, public IGTTInteractable",
    "TryActivateLegacyTakeover",
    "DeactivateLegacyTakeover",
    "ExitNativeVehicle",
    "RecallToTransform",
    "IsLegacyTakeoverActive",
    "GetDriverPawn",
]:
    assert token in header, f"native takeover API missing {token}"

# Takeover must be strict, ownership-gated and preserve the proven legacy fallback/mirror.
for token in [
    "if (!bNativeReady || !GetWorld())",
    "LegacyVehicle->IsOwnedByPlayer()",
    "LegacyVehicle->IsOccupied()",
    "ImportLegacyGameplayState(LegacyVehicle",
    "LegacyVehicle->SetActorHiddenInGame(true)",
    "LegacyVehicle->SetActorEnableCollision(false)",
    "LegacyVehicle->SetActorTickEnabled(false)",
    "RestorePersistentState",
    "SyncLegacyMirror",
    "MigrationSnapshot.FuelLiters",
    "SetTargetGear",
    'BindAction(TEXT("ExitVehicle")',
    'BindAction(TEXT("QuickSave")',
    'BindAction(TEXT("QuickLoad")',
]:
    assert token in cpp, f"native takeover implementation missing {token}"

# Existing gameplay services must follow the hidden driver while native pawn is possessed.
assert "AGTTFieldmasterNativePawn" in statics
assert "ResolveGTTDriverPawn" in statics
for token in ["FindWantedComponentForPawn", "FindEconomyComponentForPawn", "FindRadioComponentForPawn"]:
    assert token in statics

# Garage must recall the active native representation, not the hidden compatibility actor.
assert "ResolveActiveNativeFieldmaster" in garage
assert "NativeFieldmaster->RecallToTransform" in garage
assert "IsLegacyTakeoverActive" in garage

# Roadmap style lock and arithmetic stay exact; Native Chaos remains deliberately open.
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "📊 Overall progress" in roadmap
checked = roadmap.count("- [x]")
remaining = roadmap.count("- [ ]")
total = checked + remaining
assert (checked, remaining, total) == (125, 5, 130), f"roadmap arithmetic mismatch: {checked}/{total}, remaining={remaining}"
assert "125/130" in roadmap and "96.2%" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap

assert "Win64" in playtest and "fallback" in playtest.lower() and "garage" in playtest.lower()
assert "Verify Fieldmaster native runtime takeover" in workflow

print(f"Fieldmaster native runtime takeover sanity passed; roadmap {checked}/{total}")
