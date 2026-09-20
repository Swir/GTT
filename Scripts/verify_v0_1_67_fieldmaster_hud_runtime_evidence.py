#!/usr/bin/env python3
"""Source/integration verifier for GTT 0.1.67 Fieldmaster HUD packaged-runtime evidence."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HUD_CPP = ROOT / "Source/GTT/Private/UI/GTTGameHUD.cpp"
TELEMETRY = ROOT / "Source/GTT/Private/Vehicles/GTTNativeRuntimeTelemetrySubsystem.cpp"
EVALUATOR = ROOT / "Scripts/evaluate_fieldmaster_hud_runtime.ps1"
RUNNER = ROOT / "Scripts/run_win64_attested_candidate_acceptance.ps1"
ATTESTOR = ROOT / "Scripts/write_win64_candidate_attestation.ps1"
CONFIG = ROOT / "Config/DefaultGame.ini"
ROADMAP = ROOT / "Docs/ROADMAP.md"
PLAYTEST = ROOT / "Docs/PLAYTEST_0.1.67.md"
CHANGELOG = ROOT / "CHANGELOG.d/0.1.67-fieldmaster-hud-runtime-evidence.md"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


hud = HUD_CPP.read_text(encoding="utf-8")
telemetry = TELEMETRY.read_text(encoding="utf-8")
evaluator = EVALUATOR.read_text(encoding="utf-8")
runner = RUNNER.read_text(encoding="utf-8")
attestor = ATTESTOR.read_text(encoding="utf-8")
config = CONFIG.read_text(encoding="utf-8")
roadmap = ROADMAP.read_text(encoding="utf-8")
playtest = PLAYTEST.read_text(encoding="utf-8")
changelog = CHANGELOG.read_text(encoding="utf-8")

require("ProjectVersion=0.1.67" in config, "project version must be 0.1.67")
for token in (
    "TRAILER RUNAWAY ASSIST",
    "TRAILER BRAKES CRITICAL",
    "TRAILER BRAKE FADE",
    "TRAILER BRAKES HOT",
    "TRAILER BRAKES COOLING",
    "DESCENT ASSIST",
    "HILL HOLD ACTIVE",
):
    require(token in hud, f"HUD source label drifted: {token}")

for token in (
    "trailer_brake_heat=%.3f",
    "trailer_brake_authority=%.3f",
    "trailer_brake_state=%s",
    "trailer_brake_fade=%s",
    "trailer_brake_cooling=%s",
    "runaway_mitigation=%s",
    "downhill_tow_brake=%s",
    "hill_hold=%s",
):
    require(token in telemetry, f"runtime telemetry contract missing: {token}")

for token in (
    "gtt.fieldmaster-hud-runtime.v1",
    "FIELDMASTER_HUD_RUNTIME.json",
    "NATIVE_FIELDMASTER_RUNTIME_TELEMETRY",
    "TRAILER_RUNAWAY_ASSIST",
    "TRAILER_BRAKES_CRITICAL",
    "TRAILER_BRAKE_FADE",
    "TRAILER_BRAKES_HOT",
    "TRAILER_BRAKES_COOLING",
    "DESCENT_ASSIST",
    "HILL_HOLD_ACTIVE",
    "visible_alert_samples",
    "assist_alert_samples",
    "thermal_alert_samples",
    "cooling_alert_samples",
    "build SHA mismatch",
):
    require(token in evaluator, f"0.1.67 evaluator missing: {token}")

require('"evaluate_fieldmaster_hud_runtime.ps1"' in runner, "attested runner must execute HUD evaluator")
require('fieldmaster_hud_runtime -NotePropertyValue "PASS"' in runner, "acceptance summary must record HUD runtime PASS")
require(runner.index("& $HillHaulEvaluator") < runner.index("& $HudEvaluator") < runner.index("& $Attestor"), "HUD evidence must run after hill-haul and before attestation")

for token in (
    'Read-JsonRequired "FIELDMASTER_HUD_RUNTIME.json"',
    'Assert-ExactIdentity $hud "FIELDMASTER_HUD_RUNTIME.json"',
    'fieldmaster_hud_runtime = "PASS"',
    "fieldmaster_hud_telemetry_samples",
    "fieldmaster_hud_visible_alert_samples",
    '"FIELDMASTER_HUD_RUNTIME.json"',
    "FINAL_SHA256SUMS.txt",
):
    require(token in attestor, f"candidate attestation missing HUD runtime binding: {token}")

checkboxes = re.findall(r"^- \[([ xX])\] ", roadmap, flags=re.MULTILINE)
done = sum(1 for value in checkboxes if value.lower() == "x")
require((done, len(checkboxes)) == (125, 130), f"roadmap checkbox math drifted to {done}/{len(checkboxes)}")
require("**125** | **5** | **130** | **96.2%**" in roadmap, "roadmap table must remain 125/130 = 96.2%")
require(roadmap.count("../assets/readme/progress-mini.svg") == 1, "roadmap must embed exactly one mini SVG")
require(not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, re.MULTILINE), "legacy text progress meter returned")

for required_open in (
    "Dedicated native Chaos wheeled tractor movement",
    "Full Unreal compile + packaged Win64 smoke test",
    "Dedicated native Chaos drivetrain/suspension/wheel setup",
    "Authored skeletal trailer wheel assets and final hitch sockets",
    "Full Win64 CI/build runner",
):
    require(f"- [ ] {required_open}" in roadmap, f"runtime/art gate closed without evidence: {required_open}")

for token in ("0.1.67", "FIELDMASTER_HUD_RUNTIME.json", "exact candidate", "human visual review", "does not close"):
    require(token.lower() in playtest.lower(), f"playtest missing: {token}")
    require(token.lower() in changelog.lower(), f"changelog missing: {token}")

print("GTT 0.1.67 Fieldmaster HUD packaged-runtime evidence: source/integration contract OK")
print("Exact candidate attestation now seals authoritative HUD safety-state telemetry evidence")
print("Roadmap truth preserved: 125/130 = 96.2%; five runtime/art gates remain open")
