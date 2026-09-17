from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
workflow = (ROOT / ".github/workflows/win64-package-evidence.yml").read_text(encoding="utf-8")
release_workflow = (ROOT / ".github/workflows/release-windows.yml").read_text(encoding="utf-8")
dedicated_workflow = (ROOT / ".github/workflows/win64-runtime-acceptance-sanity.yml").read_text(encoding="utf-8")
preflight = (ROOT / "Scripts/preflight_win64_unreal.ps1").read_text(encoding="utf-8")
smoke = (ROOT / "Scripts/smoke_test_windows.ps1").read_text(encoding="utf-8")
package = (ROOT / "Scripts/package_windows.ps1").read_text(encoding="utf-8")
validator = (ROOT / "Scripts/validate_windows_package.ps1").read_text(encoding="utf-8")
telemetry = (ROOT / "Scripts/evaluate_fieldmaster_chaos_telemetry.ps1").read_text(encoding="utf-8")
demo_gate = (ROOT / "Scripts/evaluate_demo_candidate.ps1").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.1.15.md").read_text(encoding="utf-8")
release_doc = (ROOT / "Docs/RELEASE_WINDOWS.md").read_text(encoding="utf-8")
uproject = (ROOT / "GTT.uproject").read_text(encoding="utf-8")

required_preflight = [
    'evidence_schema = 2', 'GTT_WIN64_UNREAL_PREFLIGHT', 'windows-host', 'run-uat', 'unreal-editor-cmd',
    'unreal-build-tool', 'engine-version-5.8', 'project-engine-association', 'chaos-vehicles-plugin',
    'gtt-runtime-module', 'git-lfs', 'msvc-toolchain', 'windows-sdk', 'free-disk', 'exit 2',
]
for token in required_preflight:
    assert token in preflight, f"missing Win64 preflight contract token: {token}"

required_workflow = [
    "workflow_dispatch:", "default: '0.1.15'", "runs-on: [self-hosted, windows, x64, unreal-5.8]",
    "preflight_win64_unreal.ps1", "package_windows.ps1", "smoke_test_windows.ps1",
    "evaluate_fieldmaster_chaos_telemetry.ps1", "FIELDMASTER_CHAOS_TELEMETRY.json",
    "validate_windows_package.ps1", "WIN64_PREFLIGHT.json", "BUILD_ATTEMPT.json", "RUNTIME_SMOKE.json",
    "DEMO_SCENARIO.json", "GAMEPLAY_SMOKE.json", "DEMO_TECHNICAL_GATE.json", "actions/upload-artifact@v4",
    "if: failure()", "Win64-failure-diagnostics",
]
for token in required_workflow:
    assert token in workflow, f"missing Win64 evidence workflow token: {token}"

required_release = [
    "default: '0.1.15'", "preflight_win64_unreal.ps1", "verify_fieldmaster_runtime_telemetry.py",
    "BUILD_ATTEMPT.json", "WIN64_PREFLIGHT.json", "PACKAGE_VALIDATION.json", "SHA256SUMS.txt", "if: failure()",
]
for token in required_release:
    assert token in release_workflow, f"missing release workflow evidence token: {token}"

required_package = [
    'string]$Version = "0.1.15"', "preflight_win64_unreal.ps1", "WIN64_PREFLIGHT.json", "BUILD_ATTEMPT.json",
    "GTT_WIN64_BUILD_ATTEMPT", 'result = "RUNNING"', 'attempt.result = "FAIL"', 'attempt.result = "PASS"',
    "BUILD_INFO.json", "SHA256SUMS.txt",
]
for token in required_package:
    assert token in package, f"missing package helper token: {token}"

required_validator = [
    "WIN64_PREFLIGHT.json", "BUILD_ATTEMPT.json", 'preflight.result -ne "PASS"', 'attempt.result -ne "PASS"',
    "uat_exit_code", "PACKAGE_VALIDATION.json",
]
for token in required_validator:
    assert token in validator, f"missing package validator evidence token: {token}"

required_smoke = [
    "GTT.exe", "Start-Process", "MinimumAliveSeconds", "RUNTIME_SMOKE.json", "result = 'PASS'",
    "visual_acceptance = 'NOT_PERFORMED'", "Stop-Process",
]
for token in required_smoke:
    assert token in smoke, f"missing runtime smoke evidence token: {token}"

for token in [
    'gtt.fieldmaster-chaos-telemetry.v1', 'FIELDMASTER_CHAOS_TELEMETRY', 'FIELDMASTER_CHAOS_TELEMETRY.json',
    'driven_sample_count', 'grounded_sample_count', 'torque_sample_count', 'valid_wheels -eq 4',
    'contacts -ge 2', 'drive_torque -gt 0.1',
]:
    assert token in telemetry, f"missing Fieldmaster runtime telemetry evidence token: {token}"
for token in ['FIELDMASTER_CHAOS_TELEMETRY.json', 'gtt.fieldmaster-chaos-telemetry.v1', "fieldmaster_native_chaos_telemetry='PASS'"]:
    assert token in demo_gate, f"demo gate does not consume Fieldmaster telemetry: {token}"

assert "ChaosVehiclesPlugin" in uproject and '"EngineAssociation": "5.8"' in uproject
assert "python Scripts/verify_win64_evidence_pipeline.py" in dedicated_workflow
assert "python Scripts/verify_fieldmaster_runtime_telemetry.py" in dedicated_workflow
assert "pull_request:" in dedicated_workflow and "push:" in dedicated_workflow

# Acceptance honesty: source/contract work must never close hardware/runtime gates.
assert "- [ ] Full Win64 CI/build runner" in roadmap
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
assert "125" in roadmap and "130" in roadmap and "96.2%" in roadmap
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap

for required_doc_token in [
    "WIN64_PREFLIGHT.json", "BUILD_ATTEMPT.json", "RUNTIME_SMOKE.json", "FIELDMASTER_CHAOS_TELEMETRY.json",
    "visual acceptance", "UE 5.8",
]:
    assert required_doc_token.lower() in playtest.lower(), f"playtest missing: {required_doc_token}"
    assert required_doc_token.lower() in release_doc.lower(), f"release doc missing: {required_doc_token}"

print("Win64 runtime acceptance/evidence gate: OK (0.1.15 telemetry-hardened candidate path)")
