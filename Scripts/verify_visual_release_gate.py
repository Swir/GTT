from __future__ import annotations
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(path):
    p=ROOT/path; assert p.exists(),f"missing required file: {path}"; return p.read_text(encoding="utf-8")
header=read("Source/GTT/Public/Core/GTTDemoVisualEvidenceSubsystem.h"); source=read("Source/GTT/Private/Core/GTTDemoVisualEvidenceSubsystem.cpp")
capture=read("Scripts/capture_demo_visual_evidence.ps1"); visual=read("Scripts/evaluate_demo_visual_evidence.ps1"); acceptance=read("Scripts/write_demo_visual_acceptance.ps1"); technical_gate=read("Scripts/evaluate_demo_candidate.ps1")
workflow=read(".github/workflows/win64-package-evidence.yml"); runner=read("Scripts/run_win64_candidate_acceptance.ps1"); attestor=read("Scripts/write_win64_candidate_attestation.ps1"); release_workflow=read(".github/workflows/release-windows.yml"); release_doc=read("Docs/RELEASE_WINDOWS.md"); roadmap=read("Docs/ROADMAP.md")
assert "UTickableWorldSubsystem" in header
for t in ["GTTVisualEvidence","GTTVisualEvidenceDir=","FScreenshotRequest::RequestScreenshot","world_gameplay","law_pressure","native_vehicle","loaded_trailer","hud_overview","DEMO_VISUAL_CAPTURE_PLAN","DEMO_VISUAL_CAPTURE_WRITTEN","DEMO_VISUAL_CAPTURE_COMPLETE","show_ui=1"]: assert t in source
for t in ["-RenderOffscreen","-ResX=1280","-ResY=720","-GTTDemoSmokeScenario","-GTTVisualEvidence","VISUAL_RUNTIME_SMOKE.json","human_visual_acceptance='NOT_PERFORMED'"]: assert t in capture
assert "-nullrhi" not in capture.lower()
for t in ["gtt.demo-visual-evidence.v1","System.Drawing","1280","720","luminance","sampled_color_bins","human_review_required=$true","unique_frame_hashes"]: assert t in visual
for t in ["gtt.demo-visual-acceptance.v1","[switch]$Approved","ExpectedGitSha","Reviewer","Notes","visual_evidence_manifest_sha256","exact_candidate_review=$true"]: assert t in acceptance
for t in ["DEMO_VISUAL_ACCEPTANCE.json","RequireVisual","visual_acceptance"]: assert t in technical_gate
for t in ["runs-on: [self-hosted, windows, x64, unreal-5.8]","run_win64_attested_candidate_acceptance.ps1","DEMO_VISUAL_EVIDENCE.json","DemoVisualEvidence"]: assert t in workflow,f"candidate workflow missing {t}"
for t in ["capture_demo_visual_evidence.ps1","evaluate_demo_visual_evidence.ps1"]: assert t in runner,f"canonical runner missing {t}"
for t in ["DEMO_VISUAL_EVIDENCE.json","VISUAL_RUNTIME_SMOKE.json"]: assert t in attestor,f"attestor missing visual evidence {t}"
for t in ["candidate_run_id","expected_sha","visual_review_passed","visual_review_notes","actions/download-artifact@v4","write_demo_visual_acceptance.ps1","evaluate_demo_candidate.ps1","-RequireVisual","softprops/action-gh-release@v2","DEMO_VISUAL_ACCEPTANCE.json","DemoVisualEvidence"]: assert t in release_workflow
assert "preflight_win64_unreal.ps1" not in release_workflow and "package_windows.ps1" not in release_workflow and "confirm_runtime_smoke" not in release_workflow
lower=release_doc.lower()
for t in ["two-stage","candidate_run_id","exact packaged candidate","human","demo_visual_acceptance.json"]: assert t in lower
assert "rendered" in lower and "visual" in lower and "evidence" in lower
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap and ("125/130" in roadmap or "125**" in roadmap)
print("GTT 0.1.20 visual evidence + exact-candidate release gate sanity OK (sealed runner)")
