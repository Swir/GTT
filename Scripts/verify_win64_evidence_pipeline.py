from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
workflow = (ROOT / ".github/workflows/win64-package-evidence.yml").read_text(encoding="utf-8")
smoke = (ROOT / "Scripts/smoke_test_windows.ps1").read_text(encoding="utf-8")
package = (ROOT / "Scripts/package_windows.ps1").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.0.43.md").read_text(encoding="utf-8")

required_workflow = [
    "workflow_dispatch:",
    "runs-on: [self-hosted, windows, x64, unreal-5.8]",
    "package_windows.ps1",
    "smoke_test_windows.ps1",
    "validate_windows_package.ps1",
    "RUNTIME_SMOKE.json",
    "actions/upload-artifact@v4",
]
for token in required_workflow:
    assert token in workflow, f"missing Win64 evidence workflow token: {token}"

required_smoke = [
    "GTT.exe",
    "Start-Process",
    "MinimumAliveSeconds",
    "RUNTIME_SMOKE.json",
    "result = 'PASS'",
    "visual_acceptance = 'NOT_PERFORMED'",
    "Stop-Process",
]
for token in required_smoke:
    assert token in smoke, f"missing runtime smoke evidence token: {token}"

assert 'string]$Version = "0.0.43"' in package, "package helper must default to current milestone"
assert "BUILD_INFO.json" in package and "SHA256SUMS.txt" in package
assert "- [ ] Full Win64 CI/build runner" in roadmap, "do not close runner acceptance until a real runner executes the workflow"
assert "- [ ] Full Unreal compile + packaged Win64 smoke test" in roadmap, "do not close packaged acceptance without real evidence"
assert "125" in roadmap and "130" in roadmap and "96.2%" in roadmap
assert "visual" in playtest.lower() and "RUNTIME_SMOKE.json" in playtest

print("Win64 package evidence gate: OK")
