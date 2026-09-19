from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
workflow = (ROOT / ".github/workflows/win64-package-evidence.yml").read_text(encoding="utf-8")
release_workflow = (ROOT / ".github/workflows/release-windows.yml").read_text(encoding="utf-8")
dedicated_workflow = (ROOT / ".github/workflows/win64-runtime-acceptance-sanity.yml").read_text(encoding="utf-8")
preflight = (ROOT / "Scripts/preflight_win64_unreal.ps1").read_text(encoding="utf-8")
smoke = (ROOT / "Scripts/smoke_test_windows.ps1").read_text(encoding="utf-8")
package = (ROOT / "Scripts/package_windows.ps1").read_text(encoding="utf-8")
validator = (ROOT / "Scripts/validate_windows_package.ps1").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.1.14.md").read_text(encoding="utf-8")
release_doc = (ROOT / "Docs/RELEASE_WINDOWS.md").read_text(encoding="utf-8")
uproject = (ROOT / "GTT.uproject").read_text(encoding="utf-8")

required_preflight = [
    'evidence_schema = 2',
    'GTT_WIN64_UNREAL_PREFLIGHT',
    'windows-host',
    'run-uat',
    'unreal-editor-cmd',
    'unreal-build-tool',
    'engine-version-5.8',
    'project-engine-association',
    'chaos-vehicles-plugin',
    'gtt-runtime-module',
    'git-lfs',
    'msvc-toolchain',
    'windows-sdk',
    'free-disk',
    'exit 2',
]
for token in required_preflight:
    assert token in preflight, f"missing Win64 preflight contract token: {token}"

required_workflow = [
    "workflow_dispatch:",
    "runs-on: [self-hosted, windows, x64, unreal-5.8]",
    "preflight_win64_unreal.ps1",
    "package_windows.ps1",
    "smoke_test_windows.ps1",
    "validate_windows_package.ps1",
    "evaluate_workshop_capacity_runtime.ps1",
    "promote_demo_gate_workshop_capacity.ps1",
    "WORKSHOP_CAPACITY_RUNTIME.json",
    "schema -ne 16",
    "workshop_capacity_runtime -ne 'PASS'",
    "MinimumAliveSeconds 472",
    "WIN64_PREFLIGHT.json",
    "BUILD_ATTEMPT.json",
    "RUNTIME_SMOKE.json",
    "DEMO_SCENARIO.json",
    "GAMEPLAY_SMOKE.json",
    "DEMO_TECHNICAL_GATE.json",
    "actions/upload-artifact@v4",
    "if: failure()",
    "Win64-failure-diagnostics",
]
for token in required_workflow:
    assert token in workflow, f"missing Win64 evidence workflow token: {token}"

required_release = [
    "candidate_run_id",
    "expected_sha",
    "visual_review_passed",
    "actions/download-artifact@v4",
    "run-id:",
    "DEMO_VISUAL_ACCEPTANCE.json",
    "evaluate_demo_candidate.ps1",
    "-RequireVisual",
    "softprops/action-gh-release@v2",
]
for token in required_release:
    assert token in release_workflow, f"missing exact-candidate release token: {token}"
assert "preflight_win64_unreal.ps1" not in release_workflow, "publication stage must not rerun build preflight"
assert "package_windows.ps1" not in release_workflow, "publication stage must not rebuild the reviewed candidate"
assert "BuildCookRun" not in release_workflow, "publication stage must not invoke UAT build/cook/package"

required_package = [
    'string]$Version = "0.1.14"',
    "preflight_win64_unreal.ps1",
    "WIN64_PREFLIGHT.json",
    "BUILD_ATTEMPT.json",
    "GTT_WIN64_BUILD_ATTEMPT",
    'result = "RUNNING"',
    'attempt.result = "FAIL"',
    'attempt.result = "PASS"',
    "BUILD_INFO.json",
    "SHA256SUMS.txt",
]
for token in required_package:
    assert token in package, f"missing package helper token: {token}"

required_validator = [
    "WIN64_PREFLIGHT.json",
    "BUILD_ATTEMPT.json",
    'preflight.result -ne "PASS"',
    'attempt.result -ne "PASS"',
    "uat_exit_code",
    "PACKAGE_VALIDATION.json",
]
for token in required_validator:
    assert token in validator, f"missing package validator evidence token: {token}"

required_smoke = [
    "GTT.exe",
    "Start-Process",
    "MinimumAliveSeconds",
    "GTTWorkshopCapacityRuntimeScenario",
    "workshop_capacity_runtime_scenario = $true",
    "RUNTIME_SMOKE.json",
    "result = 'PASS'",
    "visual_acceptance = 'NOT_PERFORMED'",
    "Stop-Process",
]
for token in required_smoke:
    assert token in smoke, f"missing runtime smoke evidence token: {token}"

assert "ChaosVehiclesPlugin" in uproject and '"EngineAssociation": "5.8"' in uproject
assert "python Scripts/verify_win64_evidence_pipeline.py" in dedicated_workflow
assert "pull_request:" in dedicated_workflow and "push:" in dedicated_workflow

assert "- [ ] Full Win64 CI/build runner" in roadmap
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap
assert "- [ ] Dedicated native Chaos wheeled tractor movement" in roadmap
assert "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup" in roadmap
assert "125" in roadmap and "130" in roadmap and "96.2%" in roadmap
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap

for required_doc_token in [
    "WIN64_PREFLIGHT.json",
    "BUILD_ATTEMPT.json",
    "RUNTIME_SMOKE.json",
    "visual acceptance",
    "UE 5.8",
]:
    assert required_doc_token.lower() in playtest.lower(), f"playtest missing: {required_doc_token}"
    assert required_doc_token.lower() in release_doc.lower(), f"release doc missing: {required_doc_token}"

assert "candidate_run_id" in release_doc
assert "does not rebuild" in release_doc.lower() or "never" in release_doc.lower()
print("Win64 runtime acceptance/evidence gate: OK (two-stage exact-candidate release architecture; schema-16 workshop capacity wired)")
