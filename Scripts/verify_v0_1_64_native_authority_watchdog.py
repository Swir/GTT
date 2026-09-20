#!/usr/bin/env python3
"""Source/integration verifier for GTT 0.1.64 Native Chaos authority watchdog."""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
telemetry = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeRuntimeTelemetrySubsystem.cpp").read_text(encoding="utf-8")
pawn = (ROOT / "Source/GTT/Private/Vehicles/GTTFieldmasterNativePawn.cpp").read_text(encoding="utf-8")
runtime_gate = (ROOT / "Scripts/evaluate_native_authority_runtime.ps1").read_text(encoding="utf-8")
native_gate = (ROOT / "Scripts/evaluate_native_chaos_runtime.ps1").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST-0.1.64-NATIVE-AUTHORITY-WATCHDOG.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.1.64-native-authority-watchdog.md").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/gtt-v0.1.64-native-authority-watchdog.yml").read_text(encoding="utf-8")

for token in [
    "FindLegacyFieldmasterMirror",
    "ValidateNativeAuthority",
    "WRONG_MOVEMENT_CLASS",
    "MOVEMENT_INACTIVE",
    "PHYSICS_ASSET_MISSING",
    "LEGACY_MIRROR_VISIBLE",
    "LEGACY_MIRROR_COLLISION_ENABLED",
    "LEGACY_MIRROR_TICK_ENABLED",
    "NATIVE_VEHICLE_HIDDEN",
    "NATIVE_COLLISION_DISABLED",
    "NATIVE_FIELDMASTER_AUTHORITY_FAULT",
    "action=RESTORE_LEGACY",
    "Vehicle->DeactivateLegacyTakeover()",
    "authority=NATIVE_CHAOS",
    "takeover_integrity=PASS",
    "movement_class=%s",
    "legacy_mirror=QUIESCENT",
    "legacy_collision=NO",
    "legacy_tick=NO",
    "native_collision=YES",
    "physics_asset=YES",
]:
    assert token in telemetry, f"missing 0.1.64 authority-watchdog source contract: {token}"

# The existing production takeover must still quiesce the legacy actor before native authority starts.
for token in [
    "LegacyVehicle->SetActorHiddenInGame(true)",
    "LegacyVehicle->SetActorEnableCollision(false)",
    "LegacyVehicle->SetActorTickEnabled(false)",
    "SetActorHiddenInGame(false)",
    "SetActorEnableCollision(true)",
    "bTakeoverActive = true",
]:
    assert token in pawn, f"production takeover regression: {token}"

for token in [
    "gtt.native-authority-runtime.v1",
    "NATIVE_FIELDMASTER_RUNTIME_TELEMETRY",
    "authority=NATIVE_CHAOS",
    "takeover_integrity=PASS",
    "movement_class=GTTFieldmasterChaosMovementComponent",
    "legacy_mirror=QUIESCENT",
    "NATIVE_FIELDMASTER_AUTHORITY_FAULT",
    "authority_samples",
    "authority_faults",
    "packaged runtime smoke did not PASS",
    "build SHA mismatch",
]:
    assert token in runtime_gate, f"runtime authority evaluator missing: {token}"

# Preserve the already-strong packaged Native Chaos gate instead of replacing it with the new watchdog.
for token in [
    "max_valid_wheels",
    "max_contacts",
    "max_suspension_samples",
    "max_speed_kmh",
    "observed_gears",
    "automatic_gear_samples",
    "command_samples",
    "physics_fallback_observed",
]:
    assert token in native_gate, f"existing Native Chaos runtime gate regressed: {token}"

open_blockers = [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Win64 CI/build runner",
]
for checkbox in open_blockers:
    assert checkbox in roadmap, f"0.1.64 must not close runtime/art blocker without packaged proof: {checkbox}"

checked = len(re.findall(r"^- \[x\] ", roadmap, flags=re.MULTILINE | re.IGNORECASE))
open_items = len(re.findall(r"^- \[ \] ", roadmap, flags=re.MULTILINE))
assert (checked, open_items, checked + open_items) == (125, 5, 130)
assert round(checked * 100.0 / (checked + open_items), 1) == 96.2
assert roadmap.count("../assets/readme/progress-mini.svg") == 1
assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, re.MULTILINE)

for token in [
    "split authority",
    "RESTORE_LEGACY",
    "NATIVE_AUTHORITY_RUNTIME.json",
    "packaged Win64",
    "does not close",
]:
    assert token.lower() in playtest.lower(), f"0.1.64 playtest missing {token}"

for token in [
    "Native Chaos authority watchdog",
    "NATIVE_FIELDMASTER_AUTHORITY_FAULT",
    "NATIVE_AUTHORITY_RUNTIME.json",
    "does not close",
]:
    assert token.lower() in changelog.lower(), f"0.1.64 changelog missing {token}"

for token in [
    "Verify 0.1.64 Native Chaos authority watchdog",
    "verify_native_runtime_telemetry.py",
    "verify_fieldmaster_dedicated_chaos_movement.py",
    "verify_v0_1_63_trailer_brake_thermal_control.py",
    "verify_v0_1_62_fieldmaster_hill_haul_control.py",
]:
    assert token in workflow, f"0.1.64 CI regression coverage missing {token}"

print("GTT 0.1.64 Native Chaos authority watchdog: source/integration contract OK")
print("Fail-closed split-authority recovery and packaged runtime evidence evaluator are wired")
print("Roadmap truth preserved: 125/130 = 96.2%; five runtime/art gates remain open")
