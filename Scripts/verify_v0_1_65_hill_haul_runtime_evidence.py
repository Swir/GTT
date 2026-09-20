#!/usr/bin/env python3
"""Source/integration verifier for GTT 0.1.65 packaged hill-haul runtime evidence."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

telemetry = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeRuntimeTelemetrySubsystem.cpp").read_text(encoding="utf-8")
movement = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterChaosMovementComponent.cpp").read_text(encoding="utf-8")
evaluator = (ROOT / "Scripts/evaluate_fieldmaster_hill_haul_runtime.ps1").read_text(encoding="utf-8")
acceptance = (ROOT / "Scripts/run_win64_attested_candidate_acceptance.ps1").read_text(encoding="utf-8")
attestation = (ROOT / "Scripts/write_win64_candidate_attestation.ps1").read_text(encoding="utf-8")
package_workflow = (ROOT / ".github/workflows/win64-package-evidence.yml").read_text(encoding="utf-8")
release_workflow = (ROOT / ".github/workflows/release-windows.yml").read_text(encoding="utf-8")
config = (ROOT / "Config/DefaultGame.ini").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST-0.1.65-HILL-HAUL-RUNTIME-EVIDENCE.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.1.65-hill-haul-runtime-evidence.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/gtt-v0.1.65-hill-haul-runtime-evidence.yml").read_text(encoding="utf-8")

for token in [
    "GetTerrainThrottleAuthority()",
    "GetTravelGradeDegrees()",
    "GetHillHaulBrake()",
    "IsHillHoldActive()",
    "IsDownhillTowBrakeActive()",
    "GetTrailerBrakeHeat01()",
    "GetTrailerBrakeAuthority()",
    "GetTrailerBrakeThermalState()",
    "IsTrailerBrakeFadeActive()",
    "IsTrailerBrakeCoolingActive()",
    "IsTrailerRunawayMitigationActive()",
    "GetTrailerRunawaySafetyBrake()",
    "terrain_throttle_authority=%.3f",
    "travel_grade_deg=%.2f",
    "hill_haul_brake=%.3f",
    "hill_hold=%s",
    "downhill_tow_brake=%s",
    "trailer_brake_heat=%.3f",
    "trailer_brake_authority=%.3f",
    "trailer_brake_state=%s",
    "trailer_brake_fade=%s",
    "trailer_brake_cooling=%s",
    "runaway_mitigation=%s",
    "runaway_brake=%.3f",
]:
    assert token in telemetry, f"0.1.65 telemetry contract missing: {token}"

for token in [
    "TrailerBrakeFadeStartHeat = 0.62f",
    "TrailerBrakeCriticalEnterHeat = 0.88f",
    "RunawayMinimumTowLoad = 0.50f",
    "RunawayMinimumGradeDegrees = 8.0f",
    "RunawayMinimumSpeedKmh = 24.0f",
]:
    assert token in movement, f"0.1.65 runtime evidence thresholds drifted from production movement: {token}"

for token in [
    "gtt.fieldmaster-hill-haul-runtime.v1",
    "BUILD_INFO.json",
    "RUNTIME_SMOKE.json",
    "NATIVE_AUTHORITY_RUNTIME.json",
    "NATIVE_FIELDMASTER_RUNTIME_TELEMETRY",
    "trailer=ATTACHED",
    "loaded_trailer_samples",
    "assist_samples",
    "thermal_samples",
    "runaway_samples",
    "FIELDMASTER_HILL_HAUL_RUNTIME.json",
    "fewer than two loaded-trailer telemetry samples",
    "no hill-hold/downhill-assist sample was captured",
    "no trailer-brake thermal behavior sample was captured",
    "build SHA mismatch",
]:
    assert token in evaluator, f"0.1.65 packaged evaluator missing: {token}"

assert '"evaluate_fieldmaster_hill_haul_runtime.ps1"' in acceptance
assert 'fieldmaster_hill_haul_runtime -NotePropertyValue "PASS"' in acceptance
assert acceptance.index("& $BaseRunner") < acceptance.index("& $HillHaulEvaluator") < acceptance.index("& $Attestor")

for token in [
    'Read-JsonRequired "FIELDMASTER_HILL_HAUL_RUNTIME.json"',
    'Assert-ExactIdentity $hillHaul "FIELDMASTER_HILL_HAUL_RUNTIME.json"',
    "loaded_trailer_samples",
    "assist_samples",
    "thermal_samples",
    '"FIELDMASTER_HILL_HAUL_RUNTIME.json"',
    'fieldmaster_hill_haul_runtime = "PASS"',
    "FINAL_SHA256SUMS.txt",
]:
    assert token in attestation, f"0.1.65 attestation wiring missing: {token}"

for token in [
    "run_win64_attested_candidate_acceptance.ps1",
    "WIN64_CANDIDATE_ATTESTATION.json",
    "FIELDMASTER_HILL_HAUL_RUNTIME.json",
    "NATIVE_AUTHORITY_RUNTIME.json",
    "FINAL_SHA256SUMS.txt",
    "Upload sealed Win64 evidence",
]:
    assert token in package_workflow, f"0.1.65 package workflow is not sealed end-to-end: {token}"
assert "Compress-Archive" not in package_workflow, "Package workflow must not rebuild an archive outside the attestor"

for token in [
    "Verify sealed exact-candidate attestation and integrity manifest",
    "WIN64_CANDIDATE_ATTESTATION.json",
    "FIELDMASTER_HILL_HAUL_RUNTIME.json",
    "FINAL_SHA256SUMS.txt",
    "gtt.win64-candidate-attestation.v1",
    "hill_haul_loaded_samples",
    "hill_haul_assist_samples",
    "hill_haul_thermal_samples",
]:
    assert token in release_workflow, f"0.1.65 reviewed release gate missing sealed evidence check: {token}"

assert re.search(r"(?m)^ProjectVersion=0\.1\.65$", config), "ProjectVersion must identify the exact 0.1.65 candidate"

open_blockers = [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Win64 CI/build runner",
]
for checkbox in open_blockers:
    assert checkbox in roadmap, f"0.1.65 source work must not close runtime/art blocker: {checkbox}"

checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
assert (checked, open_items, checked + open_items) == (125, 5, 130)
assert round(checked * 100.0 / (checked + open_items), 1) == 96.2
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, re.MULTILINE)

for token in [
    "0.1.65",
    "FIELDMASTER_HILL_HAUL_RUNTIME.json",
    "loaded trailer",
    "hill hold",
    "downhill",
    "thermal",
    "exact candidate",
    "does not close",
]:
    assert token.lower() in playtest.lower(), f"0.1.65 playtest missing: {token}"
    assert token.lower() in changelog.lower(), f"0.1.65 changelog missing: {token}"

for token in [
    "Verify 0.1.65 packaged hill-haul evidence contract",
    "verify_v0_1_64_native_authority_watchdog.py",
    "verify_v0_1_64_trailer_brake_runaway_safety.py",
    "verify_v0_1_63_trailer_brake_thermal_control.py",
    "verify_v0_1_62_fieldmaster_hill_haul_control.py",
    "verify_v0_1_61_win64_candidate_pipeline.py",
    "verify_v0_1_61_candidate_attestation.py",
    "verify_progress_presentation.py",
    "verify_project.py",
]:
    assert token in workflow, f"0.1.65 workflow regression coverage missing: {token}"

print("GTT 0.1.65 packaged hill-haul runtime evidence: source/integration contract OK")
print("Package lane now delegates to the sealed exact-candidate runner and release rechecks attestation")
print("Roadmap truth preserved: 125/130 = 96.2%; five runtime/art gates remain open")
