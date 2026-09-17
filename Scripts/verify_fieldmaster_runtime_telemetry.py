#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
movement_h = (ROOT / 'Source/GTT/Public/Vehicles/GTTFieldmasterChaosMovementComponent.h').read_text(encoding='utf-8')
movement_cpp = (ROOT / 'Source/GTT/Private/Vehicles/GTTFieldmasterChaosMovementComponent.cpp').read_text(encoding='utf-8')
scenario = (ROOT / 'Source/GTT/Private/Core/GTTDemoSmokeScenarioSubsystem.cpp').read_text(encoding='utf-8')
evaluator = (ROOT / 'Scripts/evaluate_fieldmaster_chaos_telemetry.ps1').read_text(encoding='utf-8')
fixture = (ROOT / 'Scripts/test_fieldmaster_chaos_telemetry.ps1').read_text(encoding='utf-8')
demo_gate = (ROOT / 'Scripts/evaluate_demo_candidate.ps1').read_text(encoding='utf-8')
workflow = (ROOT / '.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
dedicated = (ROOT / '.github/workflows/win64-runtime-acceptance-sanity.yml').read_text(encoding='utf-8')
roadmap = (ROOT / 'Docs/ROADMAP.md').read_text(encoding='utf-8')
playtest = (ROOT / 'Docs/PLAYTEST_0.1.15.md').read_text(encoding='utf-8')
changelog = (ROOT / 'CHANGELOG.d/0.1.15.md').read_text(encoding='utf-8')
release_doc = (ROOT / 'Docs/RELEASE_WINDOWS.md').read_text(encoding='utf-8')

for token in [
    'FGTTFieldmasterWheelRuntimeTelemetry', 'FGTTFieldmasterRuntimeTelemetry', 'CaptureRuntimeTelemetry()',
    'EngineRpm', 'EngineMaxRpm', 'CurrentGear', 'TargetGear', 'ValidWheelCount', 'ContactCount',
    'SuspensionSampleCount', 'TotalSpringForce', 'MaxSlipMagnitude', 'TotalDriveTorque', 'TotalBrakeTorque',
    'LastRequestedSignedThrottle', 'LastTerrainGripFactor',
]:
    assert token in movement_h, f'movement telemetry contract missing {token}'

for token in [
    'GetCurrentGear()', 'GetTargetGear()', 'GetEngineRotationSpeed()', 'GetEngineMaxRotationSpeed()', 'GetForwardSpeed()',
    'GetNumWheels()', 'GetWheelState(WheelIndex)', 'Status.bIsValid', 'Status.bInContact', 'Status.bIsSlipping',
    'Status.bIsSkidding', 'Status.NormalizedSuspensionLength', 'Status.SpringForce', 'Status.SlipMagnitude',
    'Status.DriveTorque', 'Status.BrakeTorque',
]:
    assert token in movement_cpp, f'live Chaos telemetry implementation missing {token}'

for token in [
    'ExerciseFieldmasterControls', 'UGTTFieldmasterChaosMovementComponent', 'Movement->ApplyFieldmasterDriveCommand',
    'Movement->CaptureRuntimeTelemetry()', 'FIELDMASTER_CHAOS_TELEMETRY result=OBSERVED',
    'gear=%d target_gear=%d rpm=%.1f max_rpm=%.1f speed_kmh=%.2f',
    'valid_wheels=%d contacts=%d suspension_samples=%d', 'drive_torque=%.1f brake_torque=%.1f',
    'ExerciseFieldmasterControls(*It,Elapsed)',
]:
    assert token in scenario, f'deterministic scenario does not exercise dedicated Fieldmaster telemetry path: {token}'
assert 'ExerciseNativeControls(*It,Elapsed,TEXT("Fieldmaster"))' not in scenario, 'Fieldmaster still bypasses dedicated movement in packaged smoke'

for token in [
    'FIELDMASTER_CHAOS_TELEMETRY', 'gtt.fieldmaster-chaos-telemetry.v1', 'sample_count', 'driven_sample_count',
    'grounded_sample_count', 'torque_sample_count', "valid_wheels -eq 4", 'contacts -ge 2',
    'suspension_samples -eq 4', 'drive_torque -gt 0.1', 'speed_kmh', 'rpm', 'FIELDMASTER_CHAOS_TELEMETRY.json',
]:
    assert token in evaluator, f'telemetry evaluator missing hard runtime gate: {token}'

for token in [
    'fixture-sha', 'drive_torque=428.0', 'drive_torque=391.0',
    "-replace 'drive_torque=428.0','drive_torque=0.0'", 'Negative telemetry fixture unexpectedly passed',
    'FIELDMASTER_CHAOS_TELEMETRY.json',
]:
    assert token in fixture, f'telemetry evaluator fixture missing positive/negative case: {token}'

for token in [
    'FIELDMASTER_CHAOS_TELEMETRY.json', 'gtt.fieldmaster-chaos-telemetry.v1',
    "fieldmaster_native_chaos_telemetry='PASS'", 'fieldmaster_telemetry_samples', 'schema=4',
]:
    assert token in demo_gate, f'demo technical gate does not consume Fieldmaster telemetry: {token}'

for token in [
    "default: '0.1.15'", 'evaluate_fieldmaster_chaos_telemetry.ps1', 'FIELDMASTER_CHAOS_TELEMETRY.json',
    'Evaluate Fieldmaster dedicated Native Chaos telemetry', 'DEMO_TECHNICAL_GATE.json',
]:
    assert token in workflow, f'Win64 package evidence workflow missing telemetry route: {token}'

for token in [
    'verify_fieldmaster_runtime_telemetry.py', 'GTT 0.1.15 Native Chaos runtime telemetry sanity',
    'test_fieldmaster_chaos_telemetry.ps1', 'Exercise Fieldmaster telemetry evaluator with positive and negative fixtures',
]:
    assert token in dedicated, f'dedicated 0.1.15 sanity workflow missing {token}'
for text in (playtest, changelog, release_doc):
    assert 'FIELDMASTER_CHAOS_TELEMETRY.json' in text, 'milestone/release docs missing telemetry evidence manifest'
assert 'GTT 0.1.15' in changelog and '0.1.15' in playtest

assert '<!-- SWIR-ROADMAP-STANDARD:v1 -->' in roadmap
assert '## 📊 Overall progress' in roadmap
checks = re.findall(r'^- \[(x| )\] ', roadmap, flags=re.MULTILINE)
done = sum(mark == 'x' for mark in checks)
total = len(checks)
assert (done, total) == (125, 130), (done, total)
assert 'ROADMAP-96.2%25' in roadmap and 'DONE-125%2F130' in roadmap
assert '███████████████████░ 96.2%' in roadmap
assert '| **125** | **5** | **130** | **96.2%** |' in roadmap
for item in [
    'Dedicated native Chaos wheeled tractor movement', 'Full Unreal compile + packaged Win64 smoke test',
    'Dedicated native Chaos drivetrain/suspension/wheel setup', 'Authored skeletal trailer wheel assets and final hitch sockets',
    'Full Win64 CI/build runner',
]:
    assert f'- [ ] {item}' in roadmap, f'runtime-only roadmap item closed without real runtime proof: {item}'

print('GTT 0.1.15 Fieldmaster Native Chaos runtime telemetry source/evidence contract OK; parser fixtures wired; roadmap remains truthful at 125/130')
