#!/usr/bin/env python3
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
header=(root/'Source/GTT/Public/Core/GTTTrailerEvidenceScenarioSubsystem.h').read_text(encoding='utf-8'); cpp=(root/'Source/GTT/Private/Core/GTTTrailerEvidenceScenarioSubsystem.cpp').read_text(encoding='utf-8'); evaluator=(root/'Scripts/evaluate_authored_trailer_runtime.ps1').read_text(encoding='utf-8'); workflow=(root/'.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8'); runner=(root/'Scripts/run_win64_candidate_acceptance.ps1').read_text(encoding='utf-8'); attestor=(root/'Scripts/write_win64_candidate_attestation.ps1').read_text(encoding='utf-8'); roadmap=(root/'Docs/ROADMAP.md').read_text(encoding='utf-8'); playtest=(root/'Docs/PLAYTEST_0.1.19.md').read_text(encoding='utf-8'); changelog=(root/'CHANGELOG.d/0.1.19.md').read_text(encoding='utf-8'); authoring=(root/'Docs/NATIVE_TRAILER_AUTHORING.md').read_text(encoding='utf-8')
required=[(header+cpp,'UGTTTrailerEvidenceScenarioSubsystem'),(cpp,'StartDelaySeconds = 126.0f'),(cpp,'GlobalDeadlineSeconds = 172.0f'),(cpp,'AttachToNativeFieldmaster'),(cpp,'SetCargoLoaded(true)'),(cpp,'GTT.AuthoredTrailerRig'),(cpp,'tow_eye'),(cpp,'GetRuntimeSnapshot'),(cpp,'MovingDualContactSamples'),(cpp,'MinimumTowDistanceCm = 900.0f'),(cpp,'NATIVE_TRAILER_SCENARIO_SAMPLE'),(cpp,'NATIVE_TRAILER_SCENARIO_COMPLETE'),(cpp,'phase=CONTROLLED_STOP'),(evaluator,'NATIVE_TRAILER_SCENARIO_COMPLETE'),(evaluator,'safe_loaded_motion_samples'),(evaluator,'deterministic_loaded_tow'),(evaluator,'fewer than eight safe moving loaded trailer samples'),(playtest,'loaded authored-trailer motion'),(changelog,'0.1.19'),(authoring,'motion-under-load')]
missing=[t for txt,t in required if t not in txt]
if missing: raise SystemExit('GTT 0.1.19 trailer runtime exercise missing tokens: '+', '.join(missing))
smoke=re.search(r'"-MinimumAliveSeconds",\s*(\d+).*?"-LaunchTimeoutSeconds",\s*(\d+)',runner,flags=re.S); gameplay=re.search(r'"-MinimumRuntimeSeconds",\s*(\d+)',runner)
if not smoke or not gameplay: raise SystemExit('Canonical exact-candidate runner no longer exposes deterministic runtime duration arguments.')
minimum_alive,launch_timeout=map(int,smoke.groups()); minimum_gameplay=int(gameplay.group(1))
if minimum_alive<178 or launch_timeout<=minimum_alive or launch_timeout<205 or minimum_gameplay<minimum_alive: raise SystemExit(f'Runtime window regression: alive={minimum_alive} timeout={launch_timeout} gameplay={minimum_gameplay}')
for t in ['AUTHORED_TRAILER_RUNTIME_EVIDENCE','gtt.native-trailer-runtime.v1','NATIVE_CHAOS_RUNTIME.json','NATIVE_DRIVETRAIN_SCENARIO.json','exit 5']:
    if t not in evaluator: raise SystemExit('Trailer evaluator regression: missing '+t)
for t in ['runs-on: [self-hosted, windows, x64, unreal-5.8]','run_win64_attested_candidate_acceptance.ps1']:
    if t not in workflow: raise SystemExit('Sealed Win64 workflow regression: missing '+t)
if runner.index('evaluate_drivetrain_scenario.ps1')>runner.index('evaluate_authored_trailer_runtime.ps1') or runner.index('evaluate_authored_trailer_runtime.ps1')>runner.index('evaluate_demo_candidate.ps1'): raise SystemExit('Canonical runtime gate order drift')
if 'NATIVE_TRAILER_RUNTIME.json' not in attestor: raise SystemExit('Candidate attestor no longer seals authored trailer runtime evidence')
checks=re.findall(r'^- \[(x|X| )\]',roadmap,flags=re.M); done=sum(1 for v in checks if v.lower()=='x'); total=len(checks); remaining=total-done
if (done,total,remaining)!=(125,130,5): raise SystemExit(f'Roadmap checkbox drift: {done}/{total}/{remaining}')
for t in ['<!-- SWIR-ROADMAP-STANDARD:v1 -->','<!-- ROADMAP-PROGRESS:START -->','<!-- ROADMAP-PROGRESS:END -->','ROADMAP-96.2%25','DONE-125%2F130','📊 Overall progress','../assets/readme/progress-mini.svg','| **125** | **5** | **130** | **96.2%** |']:
    if t not in roadmap: raise SystemExit('Roadmap dashboard drift: missing '+t)
if roadmap.count('../assets/readme/progress-mini.svg')!=1 or re.search(r'^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}',roadmap,flags=re.M): raise SystemExit('Roadmap SVG-only presentation regressed')
for item in ['- [ ] Dedicated native Chaos wheeled tractor movement','- [ ] Full Unreal compile + packaged Win64 smoke test','- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup','- [ ] Authored skeletal trailer wheel assets and final hitch sockets','- [ ] Full Win64 CI/build runner']:
    if item not in roadmap: raise SystemExit('Runtime/hardware blocker closed without real UE evidence: '+item)
print(f'[OK] GTT 0.1.19 deterministic loaded authored-trailer exercise preserved via sealed runner: {minimum_alive}/{launch_timeout}s / gameplay {minimum_gameplay}s; roadmap {done}/{total}.')
