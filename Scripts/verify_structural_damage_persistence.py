#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
save_h = (root / 'Source/GTT/Public/Save/GTTSaveGame.h').read_text(encoding='utf-8')
gm_h = (root / 'Source/GTT/Public/Core/GTTGameMode.h').read_text(encoding='utf-8')
struct_gm_h = (root / 'Source/GTT/Public/Core/GTTStructuralGameMode.h').read_text(encoding='utf-8')
struct_gm_cpp = (root / 'Source/GTT/Private/Core/GTTStructuralGameMode.cpp').read_text(encoding='utf-8')
native_h = (root / 'Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h').read_text(encoding='utf-8')
native_persist = (root / 'Source/GTT/Private/Vehicles/GTTRoadVehicleStructuralPersistence.cpp').read_text(encoding='utf-8')
evidence_h = (root / 'Source/GTT/Public/Core/GTTStructuralDamageEvidenceSubsystem.h').read_text(encoding='utf-8')
evidence_cpp = (root / 'Source/GTT/Private/Core/GTTStructuralDamageEvidenceSubsystem.cpp').read_text(encoding='utf-8')
unified = (root / 'Source/GTT/Private/Save/GTTUnifiedSaveSubsystem.cpp').read_text(encoding='utf-8')
engine_ini = (root / 'Config/DefaultEngine.ini').read_text(encoding='utf-8')
evaluator = (root / 'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
sanity = (root / '.github/workflows/project-sanity.yml').read_text(encoding='utf-8')
playtest = (root / 'Docs/PLAYTEST_0.0.93.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.d/0.0.93.md').read_text(encoding='utf-8')
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
    'save schema retains structural v5 data under v8+': 'SaveVersion = 8' in save_h and 'FGTTStoredRoadStructuralDamageData' in save_h and 'RoadStructuralDamage' in save_h,
    'exact structural fields': all(x in save_h for x in ['FrontHealth', 'RearHealth', 'LeftHealth', 'RightHealth', 'CoolingStress', 'DetachedPanelMask']),
    'base save hooks virtual': 'virtual bool SaveProgress();' in gm_h and 'virtual bool LoadProgress();' in gm_h,
    'structural game mode overrides': 'AGTTStructuralGameMode' in struct_gm_h and 'SaveProgress() override' in struct_gm_h and 'LoadProgress() override' in struct_gm_h,
    'structural game mode active': 'GlobalDefaultGameMode=/Script/GTT.GTTStructuralGameMode' in engine_ini,
    'native mirror flushed before save': 'FlushNativePersistenceMirror' in struct_gm_cpp and 'Super::SaveProgress()' in struct_gm_cpp,
    'primary save structural capture': 'RoadStructuralDamage.Reset()' in struct_gm_cpp and 'GetBodyDamageSnapshot()' in struct_gm_cpp and 'GetDetachedPanelMask()' in struct_gm_cpp,
    'safe load takeover cycle': 'DeactivateLegacyTakeover()' in struct_gm_cpp and 'Super::LoadProgress()' in struct_gm_cpp and 'RestorePersistentBodyDamage' in struct_gm_cpp and 'TryActivateLegacyTakeover()' in struct_gm_cpp,
    'v5 structural compatibility survives newer schema': 'StructuralDamageSaveVersion = 5' in struct_gm_cpp and 'Save->SaveVersion >= StructuralDamageSaveVersion' in struct_gm_cpp,
    'unified save never downgrades': 'Primary->SaveVersion = FMath::Max(Primary->SaveVersion, 4);' in unified,
    'native structural API': all(x in native_h for x in ['ApplyScriptedImpactDamage', 'RestorePersistentBodyDamage', 'FlushNativePersistenceMirror', 'GetDetachedPanelMask']),
    'panel mask persisted visually': all(x in native_persist for x in ['FrontPanelBit', 'RearPanelBit', 'LeftPanelBit', 'RightPanelBit', 'SetSimulatePhysics(true)', 'NATIVE_ROAD_STRUCTURAL_RESTORE']),
    'scripted crash uses real damage model': 'ApplyNativeImpactDamage(ImpactSpeedKmh, Zone' in native_persist and 'NATIVE_ROAD_SCRIPTED_IMPACT' in native_persist,
    'evidence subsystem': 'UTickableWorldSubsystem' in evidence_h and 'GTTDemoSmokeScenario' in evidence_cpp,
    'evidence waits for 0.0.92 recovery': 'UGTTDamageRecoveryEvidenceSubsystem' in evidence_cpp and 'Previous->IsTickable()' in evidence_cpp,
    'structural damage is nontrivial': all(x in evidence_cpp for x in ['ApplyScriptedImpactDamage(110.0f, EGTTRoadDamageZone::Left)', 'ApplyScriptedImpactDamage(70.0f, EGTTRoadDamageZone::Front)', 'SavedPanelMask == 0', 'SavedBody.CoolingStress <= 0.05f']),
    'real SaveProgress LoadProgress round trip': 'GameMode->SaveProgress()' in evidence_cpp and 'GameMode->LoadProgress()' in evidence_cpp and 'anti-stale structural mutation' in evidence_cpp,
    'primary save verification': 'Saved->SaveVersion < 5' in evidence_cpp and 'RoadStructuralDamage.FindByPredicate' in evidence_cpp,
    'paid workshop surcharge proof': 'SavedRepairSurcharge' in evidence_cpp and 'Paid > SavedRepairSurcharge' in evidence_cpp and 'Interact_Implementation(PlayerPawn)' in evidence_cpp,
    'runtime PASS evidence': all(x in evidence_cpp for x in ['DEMO_SCENARIO_STRUCTURAL_PERSISTENCE', 'DEMO_SCENARIO_STRUCTURAL_REPAIR', 'DEMO_SCENARIO_STRUCTURAL_RECOVERY result=PASS']),
    'evaluator retains structural gates': 'gtt.demo-scenario.v11' in evaluator and 'required_step_count=33' in evaluator and "step='STRUCTURAL_PERSISTENCE'" in evaluator and "step='STRUCTURAL_REPAIR'" in evaluator,
    'evaluator structural hard gates': 'structural_persistence_passed' in evaluator and 'structural_repair_passed' in evaluator and 'structural_recovery_complete' in evaluator,
    'candidate version not regressed below 0.1.14': candidate_version_ok and all(x in workflow for x in ['WIN64_PREFLIGHT.json','BUILD_ATTEMPT.json','RUNTIME_SMOKE.json','DEMO_TECHNICAL_GATE.json']),
    'extended packaged runtime': runtime_window_ok,
    'sanity wired': 'Verify persistent Native structural damage' in sanity and 'verify_structural_damage_persistence.py' in sanity,
    'origin docs retained': '0.0.93' in playtest and 'STRUCTURAL_PERSISTENCE' in playtest and 'STRUCTURAL_REPAIR' in playtest and '0.0.93' in changelog,
}
failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit('Persistent structural damage verification failed: ' + ', '.join(failed))

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
print(f'[OK] Persistent Native structural damage + workshop recovery retained under save v8 ({len(checks)} checks; candidate {label}; runtime {minimum_alive}s); roadmap {done}/{total} = {percent:.1f}% with SVG-only progress.')
