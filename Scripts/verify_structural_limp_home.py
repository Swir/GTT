#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
sub_h = (root / 'Source/GTT/Public/Vehicles/GTTStructuralDriveConsequenceSubsystem.h').read_text(encoding='utf-8')
sub_cpp = (root / 'Source/GTT/Private/Vehicles/GTTStructuralDriveConsequenceSubsystem.cpp').read_text(encoding='utf-8')
native_h = (root / 'Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h').read_text(encoding='utf-8')
native_cpp = (root / 'Source/GTT/Private/Vehicles/GTTRoadVehicleNativePawn.cpp').read_text(encoding='utf-8')
persist_cpp = (root / 'Source/GTT/Private/Vehicles/GTTRoadVehicleStructuralPersistence.cpp').read_text(encoding='utf-8')
evaluator = (root / 'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
sanity = (root / '.github/workflows/project-sanity.yml').read_text(encoding='utf-8')
playtest = (root / 'Docs/PLAYTEST_0.0.94.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.d/0.0.94.md').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')

runtime = re.search(r'-MinimumAliveSeconds\s+(\d+)\s+-LaunchTimeoutSeconds\s+(\d+)', workflow)
gameplay_runtime = re.search(r'-MinimumRuntimeSeconds\s+(\d+)', workflow)
runtime_window_ok = bool(runtime and gameplay_runtime)
if runtime_window_ok:
    minimum_alive = int(runtime.group(1))
    launch_timeout = int(runtime.group(2))
    gameplay_minimum = int(gameplay_runtime.group(1))
    runtime_window_ok = minimum_alive >= 125 and gameplay_minimum >= minimum_alive and launch_timeout > minimum_alive

version_match = re.search(r"default:\s*'([0-9]+)\.([0-9]+)\.([0-9]+)'", workflow)
candidate_version = tuple(int(part) for part in version_match.groups()) if version_match else None
candidate_version_ok = candidate_version is not None and candidate_version >= (0, 1, 14)

checks = {
    'runtime world subsystem': 'UGTTStructuralDriveConsequenceSubsystem : public UTickableWorldSubsystem' in sub_h,
    'drive-state contract': all(x in sub_h for x in ['FGTTStructuralDriveState', 'DamageSeverity', 'DragRatePerSecond', 'LateralPullRate', 'PowerRetention', 'SteeringRetention', 'bLimpHomeActive']),
    'single authoritative body state': 'GetBodyDamageSnapshot()' in sub_cpp and 'RestorePersistentBodyDamage' not in sub_h,
    'native takeover only': 'IsNativeReady()' in sub_cpp and 'IsLegacyTakeoverActive()' in sub_cpp,
    'physical longitudinal consequence': 'const FVector DragForce' in sub_cpp and 'Mesh->AddForce(DragForce' in sub_cpp,
    'physical asymmetric pull': 'const FVector LateralForce' in sub_cpp and 'State.LateralPullRate' in sub_cpp and 'Mesh->AddForce(LateralForce' in sub_cpp,
    'cooling limp-home consequence': 'State.bLimpHomeActive' in sub_cpp and 'CoolingDrag' in sub_cpp and 'State.CoolingStress >= 0.75f' in sub_cpp,
    'production telemetry': 'NATIVE_STRUCTURAL_DRIVE_STATE' in sub_cpp and 'power_retention=' in sub_cpp and 'steering_retention=' in sub_cpp,
    'existing input consequences preserved': 'DamageThrottleLimit' in native_cpp and 'DamageSteeringLimit' in native_cpp and 'DamageSteeringBias' in native_cpp,
    'existing persistence remains source of truth': 'RestorePersistentBodyDamage' in persist_cpp and 'UpdateDamageConsequences(0.0f)' in persist_cpp,
    'evidence waits for structural 0.0.93 route': 'UGTTStructuralDamageEvidenceSubsystem' in sub_cpp and 'Previous->IsTickable()' in sub_cpp,
    'evidence stages real damage model': 'ApplyScriptedImpactDamage(110.0f, EGTTRoadDamageZone::Front)' in sub_cpp and 'ApplyScriptedImpactDamage(96.0f, EGTTRoadDamageZone::Right)' in sub_cpp,
    'damaged dynamics hard gate': all(x in sub_cpp for x in ['DamageSeverity < 0.30f', 'DragRatePerSecond < 0.28f', 'PowerRetention >= 0.90f', 'SteeringRetention >= 0.94f']),
    'save-load consequence round trip': 'GameMode->SaveProgress()' in sub_cpp and 'GameMode->LoadProgress()' in sub_cpp and 'RestorePersistentBodyDamage(Pristine, 0)' in sub_cpp,
    'reloaded drive equivalence': 'DEMO_SCENARIO_STRUCTURAL_RELOAD_HANDLING' in sub_cpp and 'StateTolerance' in sub_cpp,
    'paid workshop integration': 'Interact_Implementation(PlayerPawn)' in sub_cpp and 'CashBeforeWorkshop' in sub_cpp and 'Paid <= 0' in sub_cpp,
    'workshop removes physics consequence': 'DEMO_SCENARIO_STRUCTURAL_DRIVE_RECOVERY' in sub_cpp and 'Recovered.DragRatePerSecond > 0.01f' in sub_cpp and 'Recovered.bLimpHomeActive' in sub_cpp,
    'runtime PASS evidence': all(x in sub_cpp for x in ['DEMO_SCENARIO_STRUCTURAL_HANDLING', 'DEMO_SCENARIO_STRUCTURAL_RELOAD_HANDLING', 'DEMO_SCENARIO_STRUCTURAL_DRIVE_RECOVERY', 'DEMO_SCENARIO_STRUCTURAL_DRIVE result=PASS']),
    'evaluator schema v11': 'gtt.demo-scenario.v11' in evaluator and 'required_step_count=33' in evaluator,
    'evaluator new hard steps': all(x in evaluator for x in ["step='STRUCTURAL_HANDLING'", "step='STRUCTURAL_RELOAD_HANDLING'", "step='STRUCTURAL_DRIVE_RECOVERY'"]),
    'evaluator new hard gates': all(x in evaluator for x in ['structural_handling_passed', 'structural_reload_handling_passed', 'structural_drive_recovery_passed']),
    'candidate version not regressed below 0.1.14': candidate_version_ok and 'RUNTIME_SMOKE.json' in workflow and 'DEMO_TECHNICAL_GATE.json' in workflow,
    'extended packaged runtime': runtime_window_ok,
    'sanity wired': 'Verify persistent structural limp-home dynamics' in sanity and 'verify_structural_limp_home.py' in sanity,
    'docs': '0.0.94' in playtest and 'STRUCTURAL_HANDLING' in playtest and '0.0.94' in changelog and 'limp-home' in changelog.lower(),
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('Structural limp-home verification failed: ' + ', '.join(failed))

for token in ('<!-- SWIR-ROADMAP-STANDARD:v1 -->','<!-- ROADMAP-PROGRESS:START -->','<!-- ROADMAP-PROGRESS:END -->','## 📊 Overall progress','../assets/readme/progress-mini.svg'):
    if token not in roadmap:
        raise SystemExit('SWIR roadmap style lock missing: ' + token)
items = re.findall(r'^- \[(x| )\] ', roadmap, flags=re.MULTILINE)
done = sum(v == 'x' for v in items)
total = len(items)
remaining = total - done
percent = round(done * 100.0 / total, 1)
for token in (
    f'ROADMAP-{percent:.1f}%25', f'DONE-{done}%2F{total}',
    f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |'
):
    if token not in roadmap:
        raise SystemExit('Roadmap dashboard drift: missing ' + token)
progress_block=roadmap.split('<!-- ROADMAP-PROGRESS:START -->',1)[1].split('<!-- ROADMAP-PROGRESS:END -->',1)[0]
if progress_block.count('../assets/readme/progress-mini.svg') != 1:
    raise SystemExit('Roadmap progress block must embed exactly one canonical progress-mini.svg.')
if re.search(r'[█▓▒░]{3,}',progress_block):
    raise SystemExit('Legacy text/Unicode progress meter must not return to the active Roadmap dashboard.')

label = '.'.join(str(part) for part in candidate_version)
print(f'[OK] Persistent structural limp-home dynamics verified ({len(checks)} checks; candidate {label}; runtime {minimum_alive}s); roadmap {done}/{total} = {percent:.1f}% with SVG-only progress.')
