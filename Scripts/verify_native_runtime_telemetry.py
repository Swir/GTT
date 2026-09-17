from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
header = (ROOT / "Source/GTT/Public/Vehicles/GTTNativeRuntimeTelemetrySubsystem.h").read_text(encoding="utf-8")
source = (ROOT / "Source/GTT/Private/Vehicles/GTTNativeRuntimeTelemetrySubsystem.cpp").read_text(encoding="utf-8")
evaluator = (ROOT / "Scripts/evaluate_native_chaos_runtime.ps1").read_text(encoding="utf-8")
workflow = (ROOT / ".github/workflows/win64-package-evidence.yml").read_text(encoding="utf-8")
demo_gate = (ROOT / "Scripts/evaluate_demo_candidate.ps1").read_text(encoding="utf-8")
project_sanity = (ROOT / ".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
dedicated = (ROOT / ".github/workflows/native-runtime-telemetry-sanity.yml").read_text(encoding="utf-8")
roadmap = (ROOT / "Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest = (ROOT / "Docs/PLAYTEST_0.1.15.md").read_text(encoding="utf-8")
changelog = (ROOT / "CHANGELOG.d/0.1.15.md").read_text(encoding="utf-8")

for token in [
    "UGTTNativeRuntimeTelemetrySubsystem",
    "UTickableWorldSubsystem",
    "SampleFieldmaster",
    "EvidenceSecondsByVehicle",
]:
    assert token in header, f"missing telemetry subsystem header token: {token}"

for token in [
    "NATIVE_FIELDMASTER_RUNTIME_TELEMETRY",
    "UGTTNativeAxleTractionSubsystem",
    "SampleSnapshot",
    "GetWheelState",
    "SlipMagnitude",
    "SlipAngle",
    "GetCurrentGear",
    "GetThrottleInput",
    "GetBrakeInput",
    "GetSteeringInput",
    "ValidWheels",
    "ContactWheels",
    "bSuspensionRuntimeReady",
    "GetTowLoadFactor",
    "IsLegacyTakeoverActive",
    "IsNativeFieldmasterReady",
]:
    assert token in source, f"missing live telemetry token: {token}"

for token in [
    "NATIVE_CHAOS_RUNTIME.json",
    "gtt.native-chaos-runtime.v1",
    "ExpectedGitSha",
    "NATIVE_FIELDMASTER_RUNTIME_TELEMETRY",
    "NATIVE_PHYSICS_EVIDENCE",
    "NATIVE_WHEEL_SETUP_EVIDENCE",
    "NATIVE_PHYSICS_FALLBACK",
    "FIELDMASTER_MOTION",
    "FIELDMASTER_CONTROL",
    "max_valid_wheels",
    "max_contacts",
    "max_suspension_samples",
    "max_speed_kmh",
    "observed_gears",
    "command_samples",
    "result = $result",
]:
    assert token in evaluator, f"missing runtime evaluator contract token: {token}"

for token in [
    "workflow_dispatch:",
    "runs-on: [self-hosted, windows, x64, unreal-5.8]",
    "evaluate_native_chaos_runtime.ps1",
    "NATIVE_CHAOS_RUNTIME.json",
    "Upload verified Win64 evidence",
    "Win64-failure-diagnostics",
]:
    assert token in workflow, f"Win64 workflow missing telemetry evidence token: {token}"

for token in [
    "NATIVE_CHAOS_RUNTIME.json",
    "gtt.native-chaos-runtime.v1",
    "native_chaos_runtime='PASS'",
    "schema=5",
    "max_valid_wheels",
    "max_contacts",
    "max_suspension_samples",
    "max_speed_kmh",
    "command_samples",
]:
    assert token in demo_gate, f"demo technical gate missing telemetry token: {token}"

assert "python Scripts/verify_native_runtime_telemetry.py" in project_sanity
assert "python Scripts/verify_native_runtime_telemetry.py" in dedicated
assert "pull_request:" in dedicated and "push:" in dedicated

# The source-contract milestone must not close hardware/runtime acceptance gates.
for checkbox in [
    "- [ ] Dedicated native Chaos wheeled tractor movement",
    "- [ ] Full Unreal compile + packaged Win64 smoke test",
    "- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup",
    "- [ ] Authored skeletal trailer wheel assets and final hitch sockets",
    "- [ ] Full Win64 CI/build runner",
]:
    assert checkbox in roadmap, f"runtime/hardware gate was closed without packaged evidence: {checkbox}"
assert "<!-- SWIR-ROADMAP-STANDARD:v1 -->" in roadmap
assert "📊 Overall progress" in roadmap
assert "125" in roadmap and "130" in roadmap and "96.2%" in roadmap

for token in [
    "NATIVE_CHAOS_RUNTIME.json",
    "gtt.native-chaos-runtime.v1",
    "four valid",
    "FIELDMASTER_MOTION",
    "FIELDMASTER_CONTROL",
    "visual acceptance",
]:
    assert token.lower() in playtest.lower(), f"0.1.15 playtest missing: {token}"

for token in [
    "UGTTNativeRuntimeTelemetrySubsystem",
    "evaluate_native_chaos_runtime.ps1",
    "NATIVE_CHAOS_RUNTIME.json",
    "schema 5",
    "does **not** claim",
]:
    assert token.lower() in changelog.lower(), f"0.1.15 changelog missing: {token}"

print("GTT 0.1.15 Native Chaos runtime telemetry/drivetrain acceptance contract: OK")
