from pathlib import Path
root = Path(__file__).resolve().parents[1]
eval_ps = (root / 'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
roadblock = (root / 'Source/GTT/Private/Police/GTTRoadblock.cpp').read_text(encoding='utf-8')
native = (root / 'Source/GTT/Public/Vehicles/GTTRoadVehicleNativePawn.h').read_text(encoding='utf-8')
checks = {
    'native telemetry path': 'VehiclePath=TEXT("NATIVE_CHAOS")' in roadblock and 'path=%s' in roadblock,
    'native consequence telemetry': 'ROADBLOCK_SPIKE_CONSEQUENCE vehicle=%s' in roadblock and 'tire_before=%.3f' in roadblock and 'tire_after=%.3f' in roadblock and 'tire_delta=%.3f' in roadblock,
    'native spike API': 'ApplyPoliceSpikeDamage' in native,
    'evaluator requires native consequence': 'ROADBLOCK_SPIKE_CONSEQUENCE vehicle=' in eval_ps and 'path=NATIVE_CHAOS' in eval_ps,
    'tire before/after hard gate': 'nativeSpikeAfter -ge $nativeSpikeBefore' in eval_ps and 'nativeSpikeDelta -le 0' in eval_ps,
    'evidence schema': 'gtt.demo-scenario.v8' in eval_ps,
    'evidence fields': 'native_spike_consequence_passed' in eval_ps and 'native_spike_tire_delta' in eval_ps and 'physical_crossing_passed' in eval_ps and 'post_spike_escape_passed' in eval_ps,
    'win64 milestone': "default: '0.0.91'" in workflow and 'post-spike escape dynamics evidence' in workflow,
}
failed = [name for name, ok in checks.items() if not ok]
if failed: raise SystemExit('Native roadblock runtime gate verification failed: ' + ', '.join(failed))
print(f'Native roadblock runtime evidence gate verified ({len(checks)}/{len(checks)} checks).')
