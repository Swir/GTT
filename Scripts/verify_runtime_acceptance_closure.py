#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
evaluator = (ROOT / "Scripts/evaluate_authored_trailer_runtime.ps1").read_text(encoding="utf-8")
gate = (ROOT / "Scripts/evaluate_demo_candidate.ps1").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/win64-package-evidence.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.1.18.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.1.18.md").read_text(encoding="utf-8")

errors = []

for token in [
    "NATIVE_TRAILER_RUNTIME.json", "gtt.native-trailer-runtime.v1", "AUTHORED_TRAILER_RUNTIME_EVIDENCE",
    "NATIVE_CHAOS_RUNTIME.json", "NATIVE_DRIVETRAIN_SCENARIO.json", "authored_active_samples",
    "native_tow_samples", "dual_contact_samples", "safe_hitch_samples", "hitch_error_cm", "warning",
    "ExpectedGitSha", "exit 5",
]:
    if token not in evaluator:
        errors.append(f"trailer runtime evaluator missing: {token}")

schema_match = re.search(r"(?m)^\s*schema=(\d+)\s*$", gate)
if not schema_match or int(schema_match.group(1)) < 6:
    errors.append("demo technical gate schema regressed below the 0.1.18 schema-6 floor")
for token in [
    "NATIVE_DRIVETRAIN_SCENARIO.json", "gtt.native-drivetrain-scenario.v1", "NATIVE_TRAILER_RUNTIME.json",
    "gtt.native-trailer-runtime.v1", "deterministic_drivetrain='PASS'", "authored_trailer_runtime='PASS'",
    "trailer_dual_contact_samples", "trailer_safe_hitch_samples",
]:
    if token not in gate:
        errors.append(f"demo technical gate missing closure token: {token}")

required_workflow = [
    "Evaluate deterministic Fieldmaster forward/reverse drivetrain scenario",
    "Evaluate authored trailer rig, hitch and dual-wheel runtime",
    "evaluate_authored_trailer_runtime.ps1", "NATIVE_TRAILER_RUNTIME.json", "DEMO_TECHNICAL_GATE.json",
]
for token in required_workflow:
    if token not in workflow:
        errors.append(f"Win64 evidence workflow missing: {token}")

order = ["evaluate_native_chaos_runtime.ps1", "evaluate_drivetrain_scenario.ps1", "evaluate_authored_trailer_runtime.ps1", "evaluate_demo_candidate.ps1"]
positions = [workflow.find(token) for token in order]
if any(pos < 0 for pos in positions) or positions != sorted(positions):
    errors.append("Win64 runtime gates are not ordered Native Chaos -> drivetrain -> trailer -> demo candidate")

for checkbox in [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Win64 CI/build runner",
]:
    if checkbox not in roadmap:
        errors.append(f"runtime/hardware gate was closed without real Win64 evidence: {checkbox}")

checked = len(re.findall(r"^\s*- \[[xX]\] ", roadmap, flags=re.MULTILINE))
open_items = len(re.findall(r"^\s*- \[ \] ", roadmap, flags=re.MULTILINE))
if (checked, open_items, checked + open_items) != (125, 5, 130):
    errors.append(f"roadmap checkbox drift: {checked}/{checked + open_items}, open={open_items}")

for token in [
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "<!-- ROADMAP-PROGRESS:START -->", "<!-- ROADMAP-PROGRESS:END -->",
    "## 📊 Overall progress", "ROADMAP-96.2%25", "DONE-125%2F130",
    "../assets/readme/progress-mini.svg", "| **125** | **5** | **130** | **96.2%** |", "0.1.18 runtime evidence closure",
]:
    if token not in roadmap:
        errors.append(f"roadmap style/progress/status drift: {token}")
if roadmap.count("../assets/readme/progress-mini.svg") != 1:
    errors.append("roadmap must embed exactly one progress-mini.svg")
if re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}", roadmap, flags=re.MULTILINE):
    errors.append("legacy text/Unicode roadmap progress meter must not return")

for token in ["drivetrain manifest is now mandatory", "authored trailer runtime", "NATIVE_TRAILER_RUNTIME.json", "schema 6", "does not close"]:
    if token.lower() not in changelog.lower():
        errors.append(f"0.1.18 changelog missing: {token}")
for token in ["NATIVE_DRIVETRAIN_SCENARIO.json", "NATIVE_TRAILER_RUNTIME.json", "dual wheel contact", "hitch", "visual acceptance", "must remain open"]:
    if token.lower() not in playtest.lower():
        errors.append(f"0.1.18 playtest missing: {token}")

if errors:
    print("GTT 0.1.18 runtime acceptance closure verification FAILED")
    for error in errors:
        print(" -", error)
    sys.exit(1)

print(f"GTT 0.1.18 runtime acceptance closure verification OK (current additive gate schema={schema_match.group(1)})")
print(" - technical gate still consumes deterministic drivetrain evidence")
print(" - authored trailer rig/contact/hitch evidence remains mandatory")
print(" - five runtime/hardware Roadmap blockers remain open until real UE 5.8 Win64 proof exists")
print(" - roadmap presentation is SVG-only; numeric checklist truth remains 125/130 (96.2%)")
