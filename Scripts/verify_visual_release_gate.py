from __future__ import annotations
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(path:str)->str:
    p=ROOT/path
    assert p.exists(),f"missing required file: {path}"
    return p.read_text(encoding='utf-8')
header=read('Source/GTT/Public/Core/GTTDemoVisualEvidenceSubsystem.h');source=read('Source/GTT/Private/Core/GTTDemoVisualEvidenceSubsystem.cpp');capture=read('Scripts/capture_demo_visual_evidence.ps1');visual=read('Scripts/evaluate_demo_visual_evidence.ps1');acceptance=read('Scripts/write_demo_visual_acceptance.ps1');technical_gate=read('Scripts/evaluate_demo_candidate.ps1');candidate_workflow=read('.github/workflows/win64-package-evidence.yml');release_workflow=read('.github/workflows/release-windows.yml');release_doc=read('Docs/RELEASE_WINDOWS.md');roadmap=read('Docs/ROADMAP.md')
assert 'UTickableWorldSubsystem' in header
for token in ('GTTVisualEvidence','GTTVisualEvidenceDir=','FScreenshotRequest::RequestScreenshot','world_gameplay','law_pressure','native_vehicle','loaded_trailer','hud_overview','DEMO_VISUAL_CAPTURE_PLAN','DEMO_VISUAL_CAPTURE_WRITTEN','DEMO_VISUAL_CAPTURE_COMPLETE','show_ui=1'): assert token in source,f'visual evidence subsystem missing {token}'
for token in ('-RenderOffscreen','-ResX=1280','-ResY=720','-GTTDemoSmokeScenario','-GTTVisualEvidence','VISUAL_RUNTIME_SMOKE.json',"human_visual_acceptance='NOT_PERFORMED'"): assert token in capture,f'visual capture harness missing {token}'
assert '-nullrhi' not in capture.lower(),'rendered visual run must not use NullRHI'
for token in ('gtt.demo-visual-evidence.v1','System.Drawing','1280','720','luminance','sampled_color_bins','human_review_required=$true','unique_frame_hashes'): assert token in visual,f'visual evaluator missing {token}'
for token in ('gtt.demo-visual-acceptance.v1','[switch]$Approved','ExpectedGitSha','Reviewer','Notes','visual_evidence_manifest_sha256','exact_candidate_review=$true'): assert token in acceptance,f'visual acceptance writer missing {token}'
for token in ('DEMO_VISUAL_ACCEPTANCE.json','RequireVisual','visual_acceptance'): assert token in technical_gate,f'technical gate missing visual acceptance requirement {token}'
for token in ('capture_demo_visual_evidence.ps1','evaluate_demo_visual_evidence.ps1','DEMO_VISUAL_EVIDENCE.json','VISUAL_RUNTIME_SMOKE.json','DemoVisualEvidence'): assert token in candidate_workflow,f'candidate workflow missing {token}'
for token in ('candidate_run_id','expected_sha','visual_review_passed','visual_review_notes','actions/download-artifact@v4','write_demo_visual_acceptance.ps1','evaluate_demo_candidate.ps1','-RequireVisual','softprops/action-gh-release@v2','DEMO_VISUAL_ACCEPTANCE.json','DemoVisualEvidence'): assert token in release_workflow,f'release workflow missing {token}'
assert 'preflight_win64_unreal.ps1' not in release_workflow
assert 'package_windows.ps1' not in release_workflow
assert 'confirm_runtime_smoke' not in release_workflow
for token in ('two-stage','candidate_run_id','rendered visual evidence','exact packaged candidate','human','DEMO_VISUAL_ACCEPTANCE.json'): assert token.lower() in release_doc.lower(),f'release docs missing {token}'
assert '<!-- SWIR-ROADMAP-STANDARD:v1 -->' in roadmap
assert '125/130' in roadmap or '125**' in roadmap
print('GTT 0.1.20 visual evidence + exact-candidate release gate sanity OK')
