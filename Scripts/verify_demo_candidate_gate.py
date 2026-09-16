#!/usr/bin/env python3
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
gate=(ROOT/'Scripts/evaluate_demo_candidate.ps1').read_text(encoding='utf-8');workflow=(ROOT/'.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8');release_verify=(ROOT/'Scripts/verify_release_pipeline.py').read_text(encoding='utf-8');roadmap=(ROOT/'Docs/ROADMAP.md').read_text(encoding='utf-8');playtest=(ROOT/'Docs/PLAYTEST_0.0.90.md').read_text(encoding='utf-8');changelog=(ROOT/'CHANGELOG.d/0.0.90.md').read_text(encoding='utf-8')
for token in ('BUILD_INFO.json','RUNTIME_SMOKE.json','DEMO_SCENARIO.json',"scenario.result -ne 'PASS'",'ExpectedGitSha','Fieldmaster','Rattleback82','Mulebox1200','NATIVE_CHAOS_SMOKE_READY','DEMO_TECHNICAL_GATE.json','DEMO_VISUAL_ACCEPTANCE.json','RequireVisual'):assert token in gate,f'missing demo evidence gate token: {token}'
for token in ("default: '0.0.90'",'runs-on: [self-hosted, windows, x64, unreal-5.8]','evaluate_demo_scenario.ps1','evaluate_demo_candidate.ps1','DEMO_SCENARIO.json','DEMO_TECHNICAL_GATE.json','GTT_RUNTIME.log','actions/upload-artifact@v4'):assert token in workflow,f'missing Win64 candidate workflow token: {token}'
assert 'verify_demo_candidate_gate.py' in release_verify;assert '0.0.90' in playtest and '0.0.90' in changelog
for token in ('<!-- SWIR-ROADMAP-STANDARD:v1 -->','## 📊 Overall progress','ROADMAP-96.2%25','DONE-125%2F130','███████████████████░ 96.2%'):assert token in roadmap,f'roadmap Style Lock/progress mismatch: {token}'
checks=re.findall(r'^- \[[x ]\] ',roadmap,flags=re.MULTILINE);done=len(re.findall(r'^- \[x\] ',roadmap,flags=re.MULTILINE));assert len(checks)==130 and done==125
assert '- [ ] Full Win64 CI/build runner' in roadmap and '- [ ] Full Unreal compile + packaged Win64 smoke test' in roadmap
for token in ('ROADBLOCK_SPIKE_CONSEQUENCE','NATIVE_CHAOS','tire_before','tire_after','tire_delta','gtt.demo-scenario.v7','physical_crossing_passed','wheel_risk_after'):assert token in ((ROOT/'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')),f'missing 0.0.90 physical roadblock evidence token: {token}'
print('[OK] Win64 physical Native roadblock crossing candidate gate verified; workflow default 0.0.90; roadmap 125/130.')
