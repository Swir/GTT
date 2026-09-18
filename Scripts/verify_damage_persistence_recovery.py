#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / 'Source/GTT/Public/Core/GTTDamageRecoveryEvidenceSubsystem.h').read_text(encoding='utf-8')
cpp = (root / 'Source/GTT/Private/Core/GTTDamageRecoveryEvidenceSubsystem.cpp').read_text(encoding='utf-8')
evaluator = (root / 'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
san = (root / '.github/workflows/project-sanity.yml').read_text(encoding='utf-8')
playtest = (root / 'Docs/PLAYTEST_0.0.92.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.d/0.0.92.md').read_text(encoding='utf-8')
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
    'tickable opt-in subsystem': 'UTickableWorldSubsystem' in header and 'GTTDemoSmokeScenario' in cpp,
    'waits for core scenario': 'UGTTDemoSmokeScenarioSubsystem' in cpp and 'CoreScenario->IsTickable()' in cpp,
    'captures damaged native vehicle': all(x in cpp for x in ['FindDamagedNativeRoadVehicle', 'TireIntegrity >= 0.995f', 'DamagedTireIntegrity']),
    'real save path': 'GameMode->SaveProgress()' in cpp and 'GTT_Prototype_01' in cpp and 'OwnedVehicles.FindByPredicate' in cpp,
    'anti-stale mutation': 'Legacy->RepairTires()' in cpp and 'Legacy->RepairVehicle(100000.0f)' in cpp,
    'real load path': 'GameMode->LoadProgress()' in cpp and 'TryActivateLegacyTakeover()' in cpp,
    'persistence evidence': 'DEMO_SCENARIO_DAMAGE_PERSISTENCE' in cpp and 'result=PASS tire_saved=' in cpp,
    'paid workshop path': 'AGTTServiceTerminal' in cpp and 'EGTTServiceType::Workshop' in cpp and 'Interact_Implementation(PlayerPawn)' in cpp,
    'economy charge proof': 'CashAfterWorkshop >= CashBeforeWorkshop' in cpp and 'WORKSHOP_TEST_RESERVE' in cpp,
    'repaired handling proof': all(x in cpp for x in ['RepairedRisk', 'RepairedThrottle', 'RepairedSteering', 'bMeasuredRecovery']),
    'workshop evidence': 'DEMO_SCENARIO_WORKSHOP_RECOVERY' in cpp and 'DEMO_SCENARIO_DAMAGE_RECOVERY result=PASS' in cpp,
    'latest schema retains recovery': 'gtt.demo-scenario.v11' in evaluator and 'required_step_count=33' in evaluator,
    'evaluator persistence hard gate': 'damage_persistence_passed' in evaluator and 'spike damage did not survive the SaveProgress/LoadProgress round-trip' in evaluator,
    'evaluator workshop hard gate': 'workshop_recovery_passed' in evaluator and 'paid workshop did not restore persisted spike damage and handling' in evaluator,
    'candidate version not regressed below 0.1.14': candidate_version_ok and all(x in workflow for x in ['WIN64_PREFLIGHT.json','BUILD_ATTEMPT.json','RUNTIME_SMOKE.json','DEMO_TECHNICAL_GATE.json']),
    'extended packaged runtime': runtime_window_ok,
    'workflow evaluator ordering': workflow.index('Evaluate structural limp-home, persistence and workshop recovery scenario') < workflow.index('Evaluate packaged gameplay smoke'),
    'sanity wired': 'Verify spike damage persistence and workshop recovery' in san and 'verify_damage_persistence_recovery.py' in san,
    'origin docs retained': '0.0.92' in playtest and 'DAMAGE_PERSISTENCE' in playtest and 'WORKSHOP_RECOVERY' in playtest and '0.0.92' in changelog,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('Damage persistence/workshop recovery verification failed: ' + ', '.join(failed))

for token in ('<!-- SWIR-ROADMAP-STANDARD:v1 -->','<!-- ROADMAP-PROGRESS:START -->','<!-- ROADMAP-PROGRESS:END -->','## 📊 Overall progress','../assets/readme/progress-mini.svg'):
    if token not in roadmap:
        raise SystemExit('SWIR roadmap style lock missing: ' + token)
items = re.findall(r'^- \[(x| )\] ', roadmap, flags=re.MULTILINE)
done = sum(v == 'x' for v in items)
total = len(items)
remaining = total - done
percent = round(done * 100.0 / total, 1)
for token in (f'ROADMAP-{percent:.1f}%25', f'DONE-{done}%2F{total}', f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |'):
    if token not in roadmap:
        raise SystemExit('Roadmap dashboard drift: missing ' + token)
progress_block=roadmap.split('<!-- ROADMAP-PROGRESS:START -->',1)[1].split('<!-- ROADMAP-PROGRESS:END -->',1)[0]
if progress_block.count('../assets/readme/progress-mini.svg') != 1:
    raise SystemExit('Roadmap progress block must embed exactly one canonical progress-mini.svg.')
if re.search(r'[█▓▒░]{3,}',progress_block):
    raise SystemExit('Legacy text/Unicode progress meter must not return to the active Roadmap dashboard.')

label = '.'.join(str(part) for part in candidate_version)
print(f'[OK] Spike damage save/load persistence + paid workshop recovery retained ({len(checks)} checks; candidate {label}; runtime {minimum_alive}s); roadmap {done}/{total} = {percent:.1f}% with SVG-only progress.')
