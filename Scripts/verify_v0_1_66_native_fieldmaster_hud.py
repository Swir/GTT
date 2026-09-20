#!/usr/bin/env python3
"""Deterministic source verifier for GTT 0.1.66 Native Fieldmaster HUD integration."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HUD_H = ROOT / "Source/GTT/Public/UI/GTTGameHUD.h"
HUD_CPP = ROOT / "Source/GTT/Private/UI/GTTGameHUD.cpp"
MOVEMENT_H = ROOT / "Source/GTT/Public/Vehicles/GTTFieldmasterChaosMovementComponent.h"
ROADMAP = ROOT / "Docs/ROADMAP.md"
CONFIG = ROOT / "Config/DefaultGame.ini"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.1.66.md"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


header = HUD_H.read_text(encoding="utf-8")
cpp = HUD_CPP.read_text(encoding="utf-8")
movement = MOVEMENT_H.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")
config = CONFIG.read_text(encoding="utf-8")
compact = re.sub(r"\s+", "", cpp)

require("ProjectVersion=0.1.66" in config, "project version must be 0.1.66")
require("class AGTTFieldmasterNativePawn;" in header, "HUD must forward-declare native Fieldmaster pawn")
for symbol in (
    "BuildNativeFieldmasterStatus",
    "BuildNativeFieldmasterAlert",
    "BuildContextHint",
):
    require(symbol in header and symbol in cpp, f"missing native Fieldmaster HUD helper: {symbol}")

for include in (
    '#include "Vehicles/GTTFieldmasterNativePawn.h"',
    '#include "Vehicles/GTTFieldmasterChaosMovementComponent.h"',
):
    require(include in cpp, f"missing native Fieldmaster HUD include: {include}")

require(
    "AGTTFieldmasterNativePawn*NativeFieldmaster=Cast<AGTTFieldmasterNativePawn>(ControlledPawn);" in compact,
    "DrawHUD must detect the possessed native Fieldmaster",
)
require(
    "BuildContextHint(ControlledPawn,Vehicle,NativeRoad,NativeFieldmaster)" in compact,
    "native Fieldmaster must participate in contextual controls",
)
require(
    'if(NativeFieldmaster)returnTEXT("Fexit|Rradio|F5saveF9load");' in compact,
    "native Fieldmaster must expose vehicle controls instead of on-foot combat hints",
)

native_branch = compact.find("elseif(NativeFieldmaster)")
combat_branch = compact.find("elseif(Combat)")
require(native_branch >= 0 and combat_branch > native_branch, "native Fieldmaster HUD branch must render before on-foot combat fallback")

for token in (
    "GetMigrationSnapshot()",
    "GetVehicleDisplayName()",
    "GetVelocity().Size()*0.036f",
    "GetTowLoadFactor()",
    "GetTrailerBrakeHeat01()",
    "GetTrailerBrakeAuthority()",
    "GetTrailerBrakeThermalState()",
    "IsTrailerRunawayMitigationActive()",
    "IsTrailerBrakeCoolingActive()",
    "IsDownhillTowBrakeActive()",
    "IsHillHoldActive()",
):
    require(token.replace(" ", "") in compact, f"HUD is missing authoritative native telemetry: {token}")

for label in (
    "TRAILER RUNAWAY ASSIST",
    "TRAILER BRAKES CRITICAL",
    "TRAILER BRAKE FADE",
    "TRAILER BRAKES HOT",
    "TRAILER BRAKES COOLING",
    "DESCENT ASSIST",
    "HILL HOLD ACTIVE",
):
    require(label in cpp, f"driver-facing native Fieldmaster state missing: {label}")

for movement_api in (
    "GetTowLoadFactor",
    "GetTrailerBrakeHeat01",
    "GetTrailerBrakeAuthority",
    "GetTrailerBrakeThermalState",
    "IsTrailerRunawayMitigationActive",
    "IsTrailerBrakeCoolingActive",
    "IsDownhillTowBrakeActive",
    "IsHillHoldActive",
):
    require(movement_api in movement, f"HUD references unavailable movement telemetry API: {movement_api}")

for noisy_literal in ("VEHICLE DYNAMICS |", "TUNING | ENGINE", "CONTROLS | LMB attack"):
    require(noisy_literal not in cpp, f"legacy HUD clutter returned: {noisy_literal}")

require("<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap, "roadmap standard marker is missing")
require("**125** | **5** | **130** | **96.2%**" in roadmap, "0.1.66 must not inflate roadmap progress")
checkboxes = re.findall(r"^- \[([ xX])\] ", roadmap, flags=re.MULTILINE)
done = sum(1 for value in checkboxes if value.lower() == "x")
require((done, len(checkboxes)) == (125, 130), f"roadmap checkbox math drifted to {done}/{len(checkboxes)}")
for required_open in (
    "Dedicated native Chaos wheeled tractor movement",
    "Full Unreal compile + packaged Win64 smoke test",
    "Dedicated native Chaos drivetrain/suspension/wheel setup",
    "Authored skeletal trailer wheel assets and final hitch sockets",
    "Full Win64 CI/build runner",
):
    require(f"- [ ] {required_open}" in roadmap, f"runtime gate closed without evidence: {required_open}")

require(PLAYTEST.exists(), "PLAYTEST_0.1.66.md must document real runtime validation")
print("GTT 0.1.66 Native Fieldmaster HUD integration: source contract OK")
print("GTT 0.1.66: trailer thermal/runaway/hill-haul driver feedback wired to authoritative Native Chaos telemetry")
print("Roadmap preserved honestly at 125/130 (96.2%); packaged UE 5.8 acceptance remains open")
