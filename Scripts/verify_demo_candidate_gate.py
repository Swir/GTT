#!/usr/bin/env python3
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
gate=(ROOT/"Scripts/evaluate_demo_candidate.ps1").read_text(encoding="utf-8")
workflow=(ROOT/".github/workflows/win64-package-evidence.yml").read_text(encoding="utf-8")
runner=(ROOT/"Scripts/run_win64_candidate_acceptance.ps1").read_text(encoding="utf-8")
attestor=(ROOT/"Scripts/write_win64_candidate_attestation.ps1").read_text(encoding="utf-8")
release_verify=(ROOT/"Scripts/verify_release_pipeline.py").read_text(encoding="utf-8")
roadmap=(ROOT/"Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest=(ROOT/"Docs/PLAYTEST_0.0.94.md").read_text(encoding="utf-8")
changelog=(ROOT/"CHANGELOG.d/0.0.94.md").read_text(encoding="utf-8")
for t in ["BUILD_INFO.json","RUNTIME_SMOKE.json","DEMO_SCENARIO.json","NATIVE_CHAOS_RUNTIME.json","scenario.result -ne 'PASS'","native.result -ne 'PASS'","ExpectedGitSha","Fieldmaster","Rattleback82","Mulebox1200","NATIVE_CHAOS_SMOKE_READY","gtt.native-chaos-runtime.v1","native_chaos_runtime","DEMO_TECHNICAL_GATE.json","DEMO_VISUAL_ACCEPTANCE.json","RequireVisual","scenario.schema -ne 'gtt.demo-scenario.v11'","required_step_count -ne 33","structural_handling_passed","structural_reload_handling_passed","structural_drive_recovery_passed","structural_drive_complete","structural_limp_home"]:
    assert t in gate,f"missing demo evidence gate token: {t}"
schema_match=re.search(r"(?m)^\s*schema=(\d+)\s*$",gate); assert schema_match and int(schema_match.group(1))>=5
for t in ["runs-on: [self-hosted, windows, x64, unreal-5.8]","run_win64_attested_candidate_acceptance.ps1","actions/upload-artifact@v4"]: assert t in workflow,f"missing sealed Win64 workflow token: {t}"
for t in ["evaluate_demo_scenario.ps1","evaluate_native_chaos_runtime.ps1","evaluate_demo_candidate.ps1","capture_demo_visual_evidence.ps1","evaluate_demo_visual_evidence.ps1"]: assert t in runner,f"canonical exact-candidate runner missing: {t}"
for t in ["DEMO_SCENARIO.json","NATIVE_CHAOS_RUNTIME.json","DEMO_TECHNICAL_GATE.json","GTT_RUNTIME.log"]: assert t in attestor,f"candidate attestor missing sealed evidence: {t}"
version_match=re.search(r"(?m)^\s+default:\s*'([0-9]+\.[0-9]+\.[0-9]+)'\s*$",workflow); assert version_match
candidate_version=tuple(map(int,version_match.group(1).split('.'))); assert candidate_version>=(0,1,14)
runtime_match=re.search(r'"-MinimumAliveSeconds",\s*(\d+).*?"-LaunchTimeoutSeconds",\s*(\d+)',runner,flags=re.S); assert runtime_match,"canonical runner smoke lifetime/timeout is not parseable"
minimum_alive,launch_timeout=map(int,runtime_match.groups()); assert minimum_alive>=125 and launch_timeout>minimum_alive
assert "verify_demo_candidate_gate.py" in release_verify and "0.0.94" in playtest and "0.0.94" in changelog
for t in ["<!-- SWIR-ROADMAP-STANDARD:v1 -->","<!-- ROADMAP-PROGRESS:START -->","<!-- ROADMAP-PROGRESS:END -->","## 📊 Overall progress","../assets/readme/progress-mini.svg","ROADMAP-96.2%25","DONE-125%2F130","| **125** | **5** | **130** | **96.2%** |"]: assert t in roadmap
checks=re.findall(r"^- \[[x ]\] ",roadmap,flags=re.M|re.I); done=len(re.findall(r"^- \[x\] ",roadmap,flags=re.M|re.I)); open_=len(re.findall(r"^- \[ \] ",roadmap,flags=re.M)); assert (len(checks),done,open_)==(130,125,5)
assert roadmap.count("../assets/readme/progress-mini.svg")==1 and not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}",roadmap,flags=re.M)
scenario=(ROOT/"Scripts/evaluate_demo_scenario.ps1").read_text(encoding="utf-8")
for t in ["ROADBLOCK_SPIKE_CONSEQUENCE","NATIVE_CHAOS","tire_before","tire_after","tire_delta","gtt.demo-scenario.v11","physical_crossing_passed","wheel_risk_after","POST_SPIKE_ESCAPE","damage_persistence_passed","workshop_recovery_passed","structural_persistence_passed","structural_repair_passed","structural_handling_passed","structural_reload_handling_passed","structural_drive_recovery_passed","structural_drive_complete","required_step_count=33"]: assert t in scenario
print(f"[OK] exact-candidate demo gate verified via sealed runner; schema={schema_match.group(1)}, runtime={minimum_alive}/{launch_timeout}s, roadmap 125/130")
