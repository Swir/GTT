from pathlib import Path
root=Path(__file__).resolve().parents[1]
eval_ps=(root/'Scripts/evaluate_packaged_gameplay_smoke.ps1').read_text(encoding='utf-8');demo=(root/'Scripts/evaluate_demo_candidate.ps1').read_text(encoding='utf-8');workflow=(root/'.github/workflows/win64-package-evidence.yml').read_text(encoding='utf-8')
checks={'gameplay schema':'gtt.packaged-gameplay-smoke.v1' in eval_ps,'fatal scan':all(x in eval_ps for x in ['Fatal error:','Unhandled Exception:','LowLevelFatalError','Assertion failed:']),'fleet hard gate':all(x in eval_ps for x in ['RustyFieldmaster60','Rattleback82','Mulebox1200','NATIVE_CHAOS_SMOKE_READY']),'sha binding':'runtime evidence SHA mismatch' in eval_ps,'manifest':'GAMEPLAY_SMOKE.json' in eval_ps and 'GAMEPLAY_SMOKE.json' in demo,'demo gate requires pass':"gameplay.result -ne 'PASS'" in demo,'workflow runs evaluator':'evaluate_packaged_gameplay_smoke.ps1' in workflow,'workflow uploads manifest':'\\GAMEPLAY_SMOKE.json' in workflow,'version':'0.0.86' in workflow}
failed=[k for k,v in checks.items() if not v]
if failed:raise SystemExit('Packaged gameplay smoke verification failed: '+', '.join(failed))
print(f'Packaged gameplay smoke verification passed ({len(checks)} checks).')
