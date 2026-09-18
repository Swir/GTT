from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
eval_ps = (root / 'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
roadblock = (root / 'Source/GTT/Private/Police/GTTRoadblock.cpp').read_text(encoding='utf-8')
native = (root / 'Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h').read_text(encoding='utf-8')

version_match = re.search(r"default:\s*'([0-9]+)\.([0-9]+)\.([0-9]+)'", workflow)
workflow_version = tuple(int(part) for part in version_match.groups()) if version_match else None

checks = {
    'native telemetry path': 'VehiclePath=TEXT("NATIVE_CHAOS")' in roadblock and 'path=%s' in roadblock,
    'native consequence telemetry': 'ROADBLOCK_SPIKE_CONSEQUENCE vehicle=%s' in roadblock and 'tire_before=%.3f' in roadblock and 'tire_after=%.3f' in roadblock and 'tire_delta=%.3f' in roadblock,
    'native spike API': 'ApplyPoliceSpikeDamage' in native,
    'evaluator requires native consequence': 'ROADBLOCK_SPIKE_CONSEQUENCE vehicle=' in eval_ps and 'path=NATIVE_CHAOS' in eval_ps,
    'tire before/after hard gate': 'nativeSpikeAfter -ge $nativeSpikeBefore' in eval_ps and 'nativeSpikeDelta -le 0' in eval_ps,
    'evidence schema': 'gtt.demo-scenario.v11' in eval_ps,
    'evidence fields': all(x in eval_ps for x in ['native_spike_consequence_passed','native_spike_tire_delta','physical_crossing_passed','post_spike_escape_passed','damage_persistence_passed','workshop_recovery_passed','structural_persistence_passed','structural_repair_passed','structural_handling_passed','structural_reload_handling_passed','structural_drive_recovery_passed']),
    'win64 milestone retained or advanced': workflow_version is not None and workflow_version >= (0, 1, 14) and all(x in workflow for x in ['RUNTIME_SMOKE.json','DEMO_TECHNICAL_GATE.json','WIN64_PREFLIGHT.json','BUILD_ATTEMPT.json']),
}
failed = [name for name, ok in checks.items() if not ok]
if failed: raise SystemExit('Native roadblock runtime gate verification failed: ' + ', '.join(failed))
version_label = '.'.join(str(part) for part in workflow_version)
print(f'Native roadblock runtime evidence gate retained under current {version_label} candidate workflow ({len(checks)}/{len(checks)} checks).')
