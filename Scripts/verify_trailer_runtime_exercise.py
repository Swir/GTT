#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
header = (root / 'Source/GTT/Public/Core/GTTTrailerEvidenceScenarioSubsystem.h').read_text(encoding='utf-8')
cpp = (root / 'Source/GTT/Private/Core/GTTTrailerEvidenceScenarioSubsystem.cpp').read_text(encoding='utf-8')
evaluator = (root / 'Scripts/evaluate_authored_trailer_runtime.ps1').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
roadmap = (root / 'Docs/ROADMAP.md').read_text(encoding='utf-8')
playtest = (root / 'Docs/PLAYTEST_0.1.19.md').read_text(encoding='utf-8')
changelog = (root / 'CHANGELOG.d/0.1.19.md').read_text(encoding='utf-8')
authoring = (root / 'Docs/NATIVE_TRAILER_AUTHORING.md').read_text(encoding='utf-8')

required = [
    (header + cpp, 'UGTTTrailerEvidenceScenarioSubsystem'), (cpp, 'StartDelaySeconds = 126.0f'),
    (cpp, 'GlobalDeadlineSeconds = 172.0f'), (cpp, 'AttachToNativeFieldmaster'), (cpp, 'SetCargoLoaded(true)'),
    (cpp, 'GTT.AuthoredTrailerRig'), (cpp, 'tow_eye'), (cpp, 'GetRuntimeSnapshot'),
    (cpp, 'MovingDualContactSamples'), (cpp, 'MinimumTowDistanceCm = 900.0f'),
    (cpp, 'NATIVE_TRAILER_SCENARIO_SAMPLE'), (cpp, 'NATIVE_TRAILER_SCENARIO_COMPLETE'),
    (cpp, 'phase=CONTROLLED_STOP'), (evaluator, 'NATIVE_TRAILER_SCENARIO_COMPLETE'),
    (evaluator, 'safe_loaded_motion_samples'), (evaluator, 'deterministic_loaded_tow'),
    (evaluator, 'fewer than eight safe moving loaded trailer samples'),
    (playtest, 'loaded authored-trailer motion'), (changelog, '0.1.19'), (authoring, 'motion-under-load'),
]
missing = [token for text, token in required if token not in text]
if missing:
    raise SystemExit('GTT 0.1.19 trailer runtime exercise missing tokens: ' + ', '.join(missing))

smoke_match = re.search(
    r'smoke_test_windows\.ps1[^\n]*-MinimumAliveSeconds\s+(\d+)[^\n]*-LaunchTimeoutSeconds\s+(\d+)', workflow,
)
gameplay_match = re.search(r'evaluate_packaged_gameplay_smoke\.ps1[^\n]*-MinimumRuntimeSeconds\s+(\d+)', workflow)
if not smoke_match or not gameplay_match:
    raise SystemExit('Win64 evidence workflow no longer exposes deterministic runtime duration arguments.')
minimum_alive = int(smoke_match.group(1))
launch_timeout = int(smoke_match.group(2))
minimum_gameplay = int(gameplay_match.group(1))
if minimum_alive < 178:
    raise SystemExit(f'Win64 runtime window regressed below 0.1.19 floor: {minimum_alive}s < 178s')
if launch_timeout <= minimum_alive or launch_timeout < 205:
    raise SystemExit(f'Win64 launch timeout no longer safely contains the evidence window: alive={minimum_alive}s timeout={launch_timeout}s')
if minimum_gameplay < minimum_alive:
    raise SystemExit(f'Packaged gameplay evidence window is shorter than smoke survival: gameplay={minimum_gameplay}s alive={minimum_alive}s')

for token in ['AUTHORED_TRAILER_RUNTIME_EVIDENCE', 'gtt.native-trailer-runtime.v1', 'NATIVE_CHAOS_RUNTIME.json', 'NATIVE_DRIVETRAIN_SCENARIO.json', 'exit 5']:
    if token not in evaluator:
        raise SystemExit('Trailer evaluator regression: missing ' + token)

if workflow.index('evaluate_drivetrain_scenario.ps1') > workflow.index('evaluate_authored_trailer_runtime.ps1'):
    raise SystemExit('Win64 evidence order drift: drivetrain must be evaluated before loaded trailer acceptance.')
if workflow.index('evaluate_authored_trailer_runtime.ps1') > workflow.index('evaluate_demo_candidate.ps1'):
    raise SystemExit('Win64 evidence order drift: trailer acceptance must precede the demo technical gate.')

checks = re.findall(r'^- \[(x|X| )\]', roadmap, flags=re.MULTILINE)
done = sum(1 for value in checks if value.lower() == 'x')
total = len(checks)
remaining = total - done
if (done, total, remaining) != (125, 130, 5):
    raise SystemExit(f'Roadmap checkbox drift: done={done} total={total} remaining={remaining}; expected 125/130/5')

for token in [
    '<!-- SWIR-ROADMAP-STANDARD:v1 -->', '<!-- ROADMAP-PROGRESS:START -->', '<!-- ROADMAP-PROGRESS:END -->',
    'ROADMAP-96.2%25', 'DONE-125%2F130', '📊 Overall progress', '../assets/readme/progress-mini.svg',
    '| **125** | **5** | **130** | **96.2%** |',
]:
    if token not in roadmap:
        raise SystemExit('Roadmap dashboard drift: missing ' + token)
if roadmap.count('../assets/readme/progress-mini.svg') != 1:
    raise SystemExit('Roadmap dashboard must embed exactly one progress-mini.svg')
if re.search(r'^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}', roadmap, flags=re.MULTILINE):
    raise SystemExit('Legacy text/Unicode roadmap progress meter must not return')

for open_item in [
    '- [ ] Dedicated native Chaos wheeled tractor movement', '- [ ] Full Unreal compile + packaged Win64 smoke test',
    '- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup',
    '- [ ] Authored skeletal trailer wheel assets and final hitch sockets', '- [ ] Full Win64 CI/build runner',
]:
    if open_item not in roadmap:
        raise SystemExit('Runtime/hardware blocker was closed without real UE 5.8 evidence: ' + open_item)

print(f'[OK] GTT 0.1.19 deterministic loaded authored-trailer exercise preserved with runtime window {minimum_alive}s / timeout {launch_timeout}s / gameplay {minimum_gameplay}s.')
print('[OK] Authored trailer evidence gate and Roadmap lock verified with SVG-only progress presentation.')
