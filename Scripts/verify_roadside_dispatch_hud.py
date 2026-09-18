#!/usr/bin/env python3
"""Source-level verifier for GTT 0.1.37 roadside dispatch HUD integration.

This proves that the native HUD consumes the authoritative roadside-dispatch
contract from 0.1.36 instead of recomputing mutable quotes/ETAs. It does not
claim an Unreal compile, package, or packaged-runtime test.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
hud = (ROOT / "Source/GTT/Private/UI/GTTGameHUD.cpp").read_text(encoding="utf-8")
road_h = (ROOT / "Source/GTT/Public/Vehicles/GTTRoadsideRecoverySubsystem.h").read_text(encoding="utf-8")
road_cpp = (ROOT / "Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
readme = (ROOT / "README.md").read_text(encoding="utf-8")
errors = []


def req(text: str, needle: str, label: str) -> None:
    if needle not in text:
        errors.append(f"missing {label}: {needle}")


# 0.1.36 remains the single authority for pending mode, quote, ETA and target identity.
for needle, label in [
    ("GetPendingRecoveryMode", "pending mode API"),
    ("GetPendingRecoveryQuote", "locked quote API"),
    ("GetPendingRecoverySecondsRemaining", "ETA API"),
    ("GetPendingRecoveryVehicleId", "target identity API"),
    ("HasPendingRoadsideService", "pending service API"),
]:
    req(road_h, needle, label)

for needle, label in [
    ("Runtime.PendingTowQuote = TowQuote", "request-time tow quote lock"),
    ("Runtime.PendingPatchQuote = PatchQuote", "request-time patch quote lock"),
    ("Runtime.PendingPersistentVehicleId = Vehicle->GetPersistentVehicleId()", "request-time target pin"),
    ("NATIVE_ROADSIDE_DISPATCH_TARGET_MISMATCH", "wrong-vehicle guard"),
    ("NATIVE_ROADSIDE_DISPATCH_CANCELLED", "cancel evidence"),
]:
    req(road_cpp, needle, label)

# HUD must read those exact authority methods in both the generic native-vehicle line
# and the Farm Cargo recovery panel. Mutable breakdown estimates are allowed only for
# pre-dispatch choices, never as the displayed pending-service contract.
for needle, label in [
    ('#include "Vehicles/GTTRoadsideRecoverySubsystem.h"', "roadside HUD include"),
    ("Roadside->HasPendingRoadsideService(Vehicle)", "generic pending-service branch"),
    ("Roadside->GetPendingRecoveryMode(Vehicle)", "generic authoritative mode"),
    ("Roadside->GetPendingRecoveryQuote(Vehicle)", "generic locked quote"),
    ("Roadside->GetPendingRecoverySecondsRemaining(Vehicle)", "generic ETA"),
    ("Roadside->GetPendingRecoveryVehicleId(Vehicle)", "generic target ID"),
    ("Roadside->GetPendingRecoveryMode(NativeRoad)", "cargo authoritative mode"),
    ("Roadside->GetPendingRecoveryQuote(NativeRoad)", "cargo locked quote"),
    ("Roadside->GetPendingRecoverySecondsRemaining(NativeRoad)", "cargo ETA"),
    ("Roadside->GetPendingRecoveryVehicleId(NativeRoad)", "cargo target ID"),
    ("CARGO PIN OK", "exact cargo identity presentation"),
    ("same key cancels before arrival", "cancellation UX"),
    ("LOCKED $%d", "locked-price presentation"),
    ("ETA %.1fs", "dispatch ETA presentation"),
]:
    req(hud, needle, label)

pending_start = hud.find("case EGTTFarmCargoRecoveryState::PatchPending:")
pending_end = hud.find("case EGTTFarmCargoRecoveryState::Patched:")
if pending_start < 0 or pending_end <= pending_start:
    errors.append("cargo pending-service HUD block missing")
else:
    pending = hud[pending_start:pending_end]
    if "Assessment.EmergencyPatchEstimate" in pending or "Assessment.TowEstimate" in pending:
        errors.append("pending cargo HUD must not recompute mutable patch/tow estimates")
    if "GetPendingRecoveryQuote" not in pending or "GetPendingRecoverySecondsRemaining" not in pending:
        errors.append("pending cargo HUD must display locked quote and authoritative ETA")

# SVG-only SWIR presentation remains protected; this gameplay milestone must not inflate progress.
checks = re.findall(r"^\s*-\s*\[(x| )\]\s+", roadmap, flags=re.MULTILINE | re.IGNORECASE)
done = sum(item.lower() == "x" for item in checks)
if (done, len(checks)) != (125, 130):
    errors.append(f"roadmap truth changed unexpectedly: {done}/{len(checks)}")
for needle, label in [
    ("<!-- SWIR-ROADMAP-STANDARD:v1 -->", "roadmap standard marker"),
    ("<!-- ROADMAP-PROGRESS:START -->", "roadmap progress marker"),
    ("../assets/readme/progress-mini.svg", "roadmap mini SVG"),
    ("| **125** | **5** | **130** | **96.2%** |", "roadmap numeric truth"),
]:
    req(roadmap, needle, label)
for needle, label in [
    ("<!-- SWIR-README-STANDARD:v2 -->", "README v2 marker"),
    ("assets/readme/progress-card.svg", "README progress card"),
    ("## 🔎 Search Keywords", "README Search Keywords"),
    ("Release readiness: **NOT READY**", "separate release readiness"),
]:
    req(readme, needle, label)
legacy_meter = re.compile(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}[^\n]*%?", re.MULTILINE)
if legacy_meter.search(roadmap) or legacy_meter.search(readme):
    errors.append("legacy text progress meter returned to maintained documentation")

if errors:
    print("GTT 0.1.37 roadside dispatch HUD contract: FAIL")
    for error in errors:
        print(" - " + error)
    sys.exit(1)

print("GTT 0.1.37 roadside dispatch HUD contract: PASS")
print("HUD consumes locked dispatch mode/quote/ETA/target identity; Farm Cargo exact-vehicle pin is visible and source guards remain intact.")
print("NOTE: source verifier only; no UE compile/package/runtime claim.")
