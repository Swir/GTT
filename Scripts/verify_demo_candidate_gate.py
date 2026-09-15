#!/usr/bin/env python3
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
gate = (ROOT / 'Scripts/evaluate_demo_candidate.ps1').read_text(encoding='utf-8')
workflow = (ROOT / '.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
release_verify = (ROOT / 'Scripts/verify_release_pipeline.py').read_text(encoding='utf-8')
roadmap = (ROOT / 'Docs/ROADMAP.md').read_text(encoding='utf-8')
playtest = (ROOT / 'Docs/PLAYTEST_0.0.80.md').read_text(encoding='utf-8')
changelog = (ROOT / 'CHANGELOG.d/0.0.80.md').read_text(encoding='utf-8')

for token in ('BUILD_INFO.json','RUNTIME_SMOKE.json',"result -ne 'PASS'",'ExpectedGitSha','Fieldmaster','Rattleback82','Mulebox1200','NATIVE_CHAOS_SMOKE_READY','DEMO_TECHNICAL_GATE.json','DEMO_VISUAL_ACCEPTANCE.json','RequireVisual'):
    assert token in gate, f'missing demo evidence gate token: {token}'
for token in ("default: '0.0.80'",'runs-on: [self-hosted, windows, x64, unreal-5.8]','evaluate_demo_candidate.ps1','DEMO_TECHNICAL_GATE.json','GTT_RUNTIME.log','actions/upload-artifact@v4'):
    assert token in workflow, f'missing Win64 candidate workflow token: {token}'
assert 'verify_demo_candidate_gate.py' in release_verify, 'release sanity does not execute 0.0.80 gate verifier'
assert '0.0.80' in playtest and '0.0.80' in changelog
for token in ('<!-- SWIR-ROADMAP-STANDARD:v1 -->','## 📊 Overall progress','ROADMAP-96.2%25','DONE-125%2F130','███████████████████░ 96.2%'):
    assert token in roadmap, f'roadmap Style Lock/progress mismatch: {token}'
checks = re.findall(r'^- \[[x ]\] ', roadmap, flags=re.MULTILINE)
done = len(re.findall(r'^- \[x\] ', roadmap, flags=re.MULTILINE))
assert len(checks) == 130 and done == 125, f'roadmap checklist changed unexpectedly: {done}/{len(checks)}'
assert '- [ ] Full Win64 CI/build runner' in roadmap
assert '- [ ] Full Unreal compile + packaged Win64 smoke test' in roadmap
print('[OK] Win64 demo candidate evidence gate verified')
print(' - package SHA + process smoke + fleet Native Chaos evidence are bound together')
print(' - visual acceptance remains a separate mandatory public-demo gate')
print(' - roadmap remains honest at 125/130 (96.2%)')
