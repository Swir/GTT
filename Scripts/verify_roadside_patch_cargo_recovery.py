#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.34 roadside patch + cargo recovery choice."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = ROOT / path
    assert p.is_file(), f"missing required file: {path}"
    return p.read_text(encoding="utf-8")

def require(text: str, token: str, where: str) -> None:
    assert token in text, f"{where}: missing {token!r}"

def function_slice(text: str, signature: str, next_signature: str | None = None) -> str:
    start = text.find(signature)
    assert start >= 0, f"missing function signature: {signature}"
    if next_signature:
        end = text.find(next_signature, start + len(signature))
        assert end > start, f"missing next function signature after {signature}: {next_signature}"
        return text[start:end]
    return text[start:]

readme = read("README.md")
roadmap = read("Docs/ROADMAP.md")
vehicle_h = read("Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h")
decision_h = read("Source/GTT/Public/Vehicles/GTTBreakdownDecisionSubsystem.h")
decision_cpp = read("Source/GTT/Private/Vehicles/GTTBreakdownDecisionSubsystem.cpp")
roadside_h = read("Source/GTT/Public/Vehicles/GTTRoadsideRecoverySubsystem.h")
roadside_cpp = read("Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp")
cargo_h = read("Source/GTT/Public/Activities/GTTFarmCargoBreakdownRecoverySubsystem.h")
cargo_cpp = read("Source/GTT/Private/Activities/GTTFarmCargoBreakdownRecoverySubsystem.cpp")
hud_h = read("Source/GTT/Public/UI/GTTGameHUD.h")
hud_cpp = read("Source/GTT/Private/UI/GTTGameHUD.cpp")
playtest = read("Docs/PLAYTEST_0.1.34.md")
changelog = read("CHANGELOG.d/0.1.34.md")

require(readme, "<!-- SWIR-README-STANDARD:v2 -->", "README")
require(readme, "## 🔎 Search Keywords", "README")
require(readme, "0.1.34", "README")
require(readme, "125 / 130 tasks complete (96.2%)", "README")
require(roadmap, "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "ROADMAP")
require(roadmap, "<!-- ROADMAP-PROGRESS:START -->", "ROADMAP")
require(roadmap, "<!-- ROADMAP-PROGRESS:END -->", "ROADMAP")
require(roadmap, "███████████████████░ 96.2%", "ROADMAP")
require(roadmap, "**125** | **5** | **130** | **96.2%**", "ROADMAP")
require(roadmap, "0.1.34 roadside recovery choice", "ROADMAP")
require(readme, "Release readiness: **NOT READY**", "README")
checks = re.findall(r"^- \[(x| )\] ", roadmap, flags=re.MULTILINE)
assert checks, "ROADMAP: no checklist items found"
done = sum(1 for item in checks if item == "x")
assert (done, len(checks)) == (125, 130), f"ROADMAP math changed unexpectedly: {done}/{len(checks)}"

require(vehicle_h, "ApplyNativeEmergencyRoadsidePatch", "native vehicle")
patch_method = function_slice(vehicle_h, "bool ApplyNativeEmergencyRoadsidePatch()", "UFUNCTION(BlueprintCallable, Category=\"GTT|Vehicle|Service\") bool RepairNativeTires")
for token in ("0.30f", "0.32f", "5.0f", "SyncLegacyMirror()"):
    require(patch_method, token, "native patch")
for forbidden in ("BodyDamage =", "DetachedPanelCount =", "ApplyNativeWorkshopService"):
    assert forbidden not in patch_method, f"native patch must not mutate body/full-service state: {forbidden}"

for token in ("EmergencyPatchEstimate", "bEmergencyPatchPossible", "CalculateRoadsidePatchEstimate", "CanEmergencyPatch"):
    require(decision_h, token, "breakdown header")
for token in ("FMath::Clamp(55 + MechanicalAid + TireAid + FuelAid, 55, 260)", "Structural.DamageSeverity >= 0.72f", "Result.bEmergencyPatchPossible"):
    require(decision_cpp, token, "breakdown implementation")

for token in ("EmergencyPatch", "RequestEmergencyRoadsidePatch", "IsRoadsidePatchPending", "bPatchRequested", "PendingPatchQuote"):
    require(roadside_h, token, "roadside header")
for token in ("EKeys::Y", "EKeys::Gamepad_DPad_Left", "EKeys::T", "EKeys::Gamepad_DPad_Up"):
    require(roadside_cpp, token, "roadside input")
require(roadside_cpp, "IsPlayerRecoveryChoiceEligible", "roadside implementation")
require(roadside_cpp, "Assessment.Recommendation == EGTTBreakdownRecommendation::TowRecommended", "roadside voluntary eligibility")
require(roadside_cpp, "if (!bHardStranded)", "police auto-impound hard-stranded guard")
require(roadside_cpp, "Runtime.bTowRequested || Runtime.bPatchRequested", "recovery choice mutual exclusion")
require(roadside_cpp, "if (WantedLevel > 0)", "patch/tow wanted gate")
require(roadside_cpp, "SpendCash(PatchQuote", "patch billing")
complete_patch = function_slice(roadside_cpp, "bool UGTTRoadsideRecoverySubsystem::CompleteEmergencyPatch", "bool UGTTRoadsideRecoverySubsystem::CompleteRecovery")
for token in ("ApplyNativeEmergencyRoadsidePatch", "bIdentityPreserved", "bBodyPreserved", "bLimpFloorsApplied", "NATIVE_ROADSIDE_PATCH_COMPLETE"):
    require(complete_patch, token, "patch completion")
for forbidden in ("SetActorTransform", "ApplyNativeWorkshopService", "ClearWanted"):
    assert forbidden not in complete_patch, f"emergency patch must not tow/service/clear wanted: {forbidden}"

for token in ("PatchPending", "Patched"):
    require(cargo_h, token, "cargo recovery header")
for token in ("IsRoadsidePatchPending", "cargo-roadside-patch-pre-service", "cargo-roadside-patch-post-service", "POST_PATCH_VERIFY", "VerifyExactCargoVehicle", "timer_paused=NO", "transfer_allowed=NO"):
    require(cargo_cpp, token, "cargo recovery implementation")
require(cargo_cpp, "GetPersistentVehicleId() == ExpectedId", "exact cargo identity")
assert "AddCash(" not in cargo_cpp and "SpendCash(" not in cargo_cpp, "cargo policy must not own billing/payout"

require(hud_h, "DrawFarmCargoRecoveryPanel", "HUD header")
for token in ("DrawFarmCargoRecoveryPanel(NativeRoad)", "Y / D-Pad Left PATCH", "T / D-Pad Up TOW", "contract clock running", "PATCH UNAVAILABLE", "Another vehicle cannot deliver this load"):
    require(hud_cpp, token, "HUD")
assert playtest.count("\n") >= 65, "playtest matrix unexpectedly short"
require(changelog, "0.1.34", "changelog")
for previous in ("Scripts/verify_farm_cargo_breakdown_recovery.py", "Scripts/verify_farm_cargo_breakdown_runtime.py", "Scripts/generate_progress_svg.py", "assets/readme/progress-card.svg", "assets/readme/progress-mini.svg", "assets/readme/progress-template.svg"):
    assert (ROOT / previous).exists(), f"missing regression/progress dependency: {previous}"
print("GTT 0.1.34 roadside patch + Farm Cargo recovery choice contract: PASS")
print(f"Roadmap: {done}/{len(checks)} = {done / len(checks) * 100:.1f}%")
