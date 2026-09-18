from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
eval_ps=(root/'Scripts/evaluate_packaged_gameplay_smoke.ps1').read_text(encoding='utf-8')
demo=(root/'Scripts/evaluate_demo_candidate.ps1').read_text(encoding='utf-8')
workflow=(root/'.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
scenario=(root/'Scripts/evaluate_demo_scenario.ps1').read_text(encoding='utf-8')

runtime_match=re.search(r'-MinimumAliveSeconds\s+(\d+)\s+-LaunchTimeoutSeconds\s+(\d+)',workflow)
gameplay_match=re.search(r'-MinimumRuntimeSeconds\s+(\d+)',workflow)
runtime_ok=False
minimum_alive=launch_timeout=minimum_gameplay=0
if runtime_match and gameplay_match:
    minimum_alive=int(runtime_match.group(1));launch_timeout=int(runtime_match.group(2));minimum_gameplay=int(gameplay_match.group(1))
    runtime_ok=minimum_alive>=178 and minimum_gameplay>=minimum_alive and launch_timeout>minimum_alive and launch_timeout>=205

# The packaged-smoke contract was first bound to candidate 0.1.14. Additive evidence
# milestones may advance the label while this verifier keeps the original smoke gates intact.
version_match=re.search(r"default:\s*'([0-9]+)\.([0-9]+)\.([0-9]+)'",workflow)
candidate_version=None
candidate_version_ok=False
if version_match:
    candidate_tuple=tuple(int(version_match.group(i)) for i in range(1,4))
    candidate_version='.'.join(version_match.groups())
    candidate_version_ok=candidate_tuple>=(0,1,14)

checks={
    'gameplay schema':'gtt.packaged-gameplay-smoke.v1' in eval_ps,
    'fatal scan':all(x in eval_ps for x in ['Fatal error:','Unhandled Exception:','LowLevelFatalError','Assertion failed:']),
    'fleet hard gate':all(x in eval_ps for x in ['RustyFieldmaster60','Rattleback82','Mulebox1200','NATIVE_CHAOS_SMOKE_READY']),
    'sha binding':'runtime evidence SHA mismatch' in eval_ps,
    'manifest':'GAMEPLAY_SMOKE.json' in eval_ps and 'GAMEPLAY_SMOKE.json' in demo,
    'demo gate requires pass':"gameplay.result -ne 'PASS'" in demo,
    'workflow runs evaluator':'evaluate_packaged_gameplay_smoke.ps1' in workflow,
    'workflow uploads manifest':'\\GAMEPLAY_SMOKE.json' in workflow,
    'current candidate version':candidate_version_ok,
    'extended runtime':runtime_ok,
    'structural scenario before gameplay smoke':'Evaluate structural limp-home, persistence and workshop recovery scenario' in workflow and workflow.index('Evaluate structural limp-home, persistence and workshop recovery scenario')<workflow.index('Evaluate packaged gameplay smoke'),
    'scenario structural gate':'gtt.demo-scenario.v11' in scenario and 'workshop_recovery_passed' in scenario and 'structural_persistence_passed' in scenario and 'structural_repair_passed' in scenario and 'structural_handling_passed' in scenario and 'structural_reload_handling_passed' in scenario and 'structural_drive_recovery_passed' in scenario,
}
failed=[k for k,v in checks.items() if not v]
if failed:raise SystemExit('Packaged gameplay smoke verification failed: '+', '.join(failed))
print(f'Packaged gameplay smoke verification passed ({len(checks)} checks) under current {candidate_version} candidate label; runtime={minimum_alive}s, gameplay={minimum_gameplay}s, timeout={launch_timeout}s.')