from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterNativePawn.h").read_text(encoding="utf-8")
cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp").read_text(encoding="utf-8")
rig_header = (ROOT / "Source/GTT/Public/Vehicles/GTTChaosRigContract.h").read_text(encoding="utf-8")
statics = (ROOT / "Source/GTT/Private/Core/GTTGameplayStatics.cpp").read_text(encoding="utf-8")
garage = (ROOT / "Source/GTT/Private/World/GTTGarageSlotTerminal.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.45.md").read_text(encoding="utf-8")

for token in [
    "public AWheeledVehiclePawn, public IGTTInteractable",
    "Interact_Implementation",
    "GetInteractionText_Implementation",
    "ExitNativeVehicle",
    "TryActivateLegacyTakeover",
    "DeactivateLegacyTakeover",
    "RecallToTransform",
    "IsLegacyTakeoverActive",
    "GetDriverPawn",
    "IsOccupied",
]:
    assert token in header, f"native takeover header missing {token}"

for token in [
    "SetActorHiddenInGame(true)",
    "SetActorEnableCollision(false)",
    "LegacyVehicle->IsOwnedByPlayer()",
    "LegacyVehicle->IsOccupied()",
    "ImportLegacyGameplayState",
    "LegacyVehicle->SetActorTickEnabled(false)",
    "SetActorHiddenInGame(false)",
    "Rig.DriverSocket",
    "Rig.ExitSocket",
    'BindAction(TEXT("ExitVehicle")',
    'BindAction(TEXT("QuickSave")',
    'BindAction(TEXT("QuickLoad")',
    'BindAction(TEXT("RadioNext")',
    "SyncLegacyMirror",
    "RestorePersistentState",
    "NativeIdleFuelBurnPerSecond",
    "NativeFullThrottleFuelBurnPerSecond",
    "SetTargetGear",
]:
    assert token in cpp, f"native takeover implementation missing {token}"

assert 'DriverSocket = TEXT("driver_seat")' in rig_header
assert 'ExitSocket = TEXT("driver_exit")' in rig_header

for token in [
    'Cast<AGTTFieldmasterNativePawn>(Pawn)',
    "GetDriverPawn()",
    "FindWantedComponentForPawn",
    "FindEconomyComponentForPawn",
    "FindRadioComponentForPawn",
]:
    assert token in statics, f"gameplay statics missing native handoff token {token}"

for token in [
    "ResolveActiveNativeFieldmaster",
    "IsLegacyTakeoverActive",
    "NativeFieldmaster->RecallToTransform",
    "RustyFieldmaster60",
]:
    assert token in garage, f"garage native recall missing {token}"

assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "📊 Overall progress" in roadmap
checked = len(re.findall(r"^\s*- \[x\] ", roadmap, re.MULTILINE))
open_count = len(re.findall(r"^\s*- \[ \] ", roadmap, re.MULTILINE))
total = checked + open_count
assert (checked, total) == (125, 130), f"roadmap changed unexpectedly: {checked}/{total}"
assert "125/130" in roadmap and "96.2%" in roadmap
assert "███████████████████░ 96.2%" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap

for token in ["takeover", "enter", "exit", "save", "garage", "fallback", "Win64", "rendered"]:
    assert token.lower() in playtest.lower(), f"0.0.45 playtest missing {token}"

print("Fieldmaster native runtime takeover sanity passed")
