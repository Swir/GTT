#!/usr/bin/env python3
from pathlib import Path
import re,sys
ROOT=Path(__file__).resolve().parents[1]
movement=(ROOT/"Source/GTT/Private/Vehicles/GTTFieldmasterChaosMovementComponent.cpp").read_text(encoding="utf-8")
authority_h=(ROOT/"Source/GTT/Public/Vehicles/GTTNativeDriveDynamicsSubsystem.h").read_text(encoding="utf-8")
authority_cpp=(ROOT/"Source/GTT/Private/Vehicles/GTTNativeDriveDynamicsSubsystem.cpp").read_text(encoding="utf-8")
powertrain=(ROOT/"Source/GTT/Private/Vehicles/GTTChaosPowertrainSetupLibrary.cpp").read_text(encoding="utf-8")
telemetry=(ROOT/"Source/GTT/Private/Vehicles/GTTNativeRuntimeTelemetrySubsystem.cpp").read_text(encoding="utf-8")
evaluator=(ROOT/"Scripts/evaluate_native_chaos_runtime.ps1").read_text(encoding="utf-8")
workflow=(ROOT/".github/workflows/win64-package-evidence.yml").read_text(encoding="utf-8")
runner=(ROOT/"Scripts/run_win64_candidate_acceptance.ps1").read_text(encoding="utf-8")
attestor=(ROOT/"Scripts/write_win64_candidate_attestation.ps1").read_text(encoding="utf-8")
project=(ROOT/".github/workflows/project-sanity.yml").read_text(encoding="utf-8")
dedicated=(ROOT/".github/workflows/automatic-drivetrain-sanity.yml").read_text(encoding="utf-8")
roadmap=(ROOT/"Docs/ROADMAP.md").read_text(encoding="utf-8")
playtest=(ROOT/"Docs/PLAYTEST_0.1.16.md").read_text(encoding="utf-8")
changelog=(ROOT/"CHANGELOG.d/0.1.16.md").read_text(encoding="utf-8")
errors=[]
if "SetTargetGear(" in movement: errors.append("Fieldmaster movement still writes target gears instead of delegating direction authority")
for t in ["UGTTNativeDriveDynamicsSubsystem","SetThrottleInput(EffectiveThrottle)","SetSteeringInput(EffectiveSteering)","SetBrakeInput","automatic forward gears"]:
    if t not in movement: errors.append(f"Fieldmaster movement missing 0.1.16 delegation token: {t}")
for t in ["LastObservedGear","AutomaticForwardGearChangeCount","DirectionShiftCommitCount","GearCommandCount"]:
    if t not in authority_h: errors.append(f"drivetrain authority state missing: {t}")
for t in ["DirectionShiftReleaseSpeedKmh = 3.5f","GearMatchesDirection","NATIVE_AUTOMATIC_GEAR_SHIFT","NATIVE_AUTOMATIC_GEARBOX_EVIDENCE","NATIVE_DIRECTION_SHIFT_COMMIT","bDirectionInterlock = true","FinalThrottle = 0.0f","DirectionInterlockBrakeMin","Movement->SetTargetGear(Authority.StableDirection, true)","bDirectionShiftCommitted || !GearMatchesDirection"]:
    if t not in authority_cpp: errors.append(f"shared drivetrain implementation missing: {t}")
if authority_cpp.count("Movement->SetTargetGear(Authority.StableDirection, true)")!=1: errors.append("shared drivetrain must have exactly one centralized target-gear write")
for t in ["bUseAutomaticGears = true","bUseAutoReverse = false","ForwardGearRatios","ChangeUpRPM","ChangeDownRPM"]:
    if t not in powertrain: errors.append(f"Chaos powertrain automatic gearbox contract missing: {t}")
for t in ["signed_speed_kmh","automatic_gears=%s","forward_gears=%d","TransmissionSetup.bUseAutomaticGears","TransmissionSetup.ForwardGearRatios.Num()"]:
    if t not in telemetry: errors.append(f"runtime telemetry missing automatic drivetrain evidence: {t}")
for t in ["automatic_gear_samples","configured_forward_gears","max_forward_gear_observed","unsafe_direction_shift_commits","max_direction_shift_commit_speed_kmh","automatic_gears' 'YES","forward_gears","NATIVE_DIRECTION_SHIFT_COMMIT","3.75"]:
    if t not in evaluator: errors.append(f"native runtime evaluator missing drivetrain safety token: {t}")
for t in ["runs-on: [self-hosted, windows, x64, unreal-5.8]","run_win64_attested_candidate_acceptance.ps1"]:
    if t not in workflow: errors.append(f"Win64 evidence workflow no longer delegates sealed candidate: {t}")
if "evaluate_native_chaos_runtime.ps1" not in runner: errors.append("canonical candidate runner no longer runs Native Chaos runtime evaluator")
if "NATIVE_CHAOS_RUNTIME.json" not in attestor: errors.append("candidate attestor no longer seals Native Chaos runtime evidence")
if "python Scripts/verify_automatic_drivetrain_runtime.py" not in project: errors.append("Project sanity does not execute 0.1.16 verifier")
if "python Scripts/verify_automatic_drivetrain_runtime.py" not in dedicated: errors.append("dedicated 0.1.16 workflow does not execute its verifier")
for t in ["verify_native_drivetrain_authority.py","verify_fieldmaster_dedicated_chaos_movement.py","verify_native_runtime_telemetry.py"]:
    if t not in dedicated: errors.append(f"dedicated 0.1.16 workflow missing regression verifier: {t}")
for c in ["- [ ] Dedicated native Chaos wheeled tractor movement","- [ ] Full Unreal compile + packaged Win64 smoke test","- [ ] Dedicated native Chaos drivetrain/suspension/wheel setup","- [ ] Authored skeletal trailer wheel assets and final hitch sockets","- [ ] Full Win64 CI/build runner"]:
    if c not in roadmap: errors.append(f"runtime/hardware gate was closed without packaged evidence: {c}")
checked=len(re.findall(r"^\s*- \[x\] ",roadmap,flags=re.M|re.I)); open_=len(re.findall(r"^\s*- \[ \] ",roadmap,flags=re.M)); total=checked+open_
if (checked,open_,total)!=(125,5,130): errors.append(f"roadmap changed unexpectedly: {checked}/{total}")
if roadmap.count("../assets/readme/progress-mini.svg")!=1 or re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}",roadmap,flags=re.M): errors.append("roadmap SVG-only presentation regressed")
for t in ["Automatic upshift freedom","Forward-to-reverse interlock","Safe reverse commit","Rattleback parity","Mulebox parity","unsafe_direction_shift_commits","visual acceptance"]:
    if t.lower() not in playtest.lower(): errors.append(f"0.1.16 playtest missing: {t}")
for t in ["GTT 0.1.16","automatic-transmission conflict","NATIVE_AUTOMATIC_GEARBOX_EVIDENCE","does **not** claim","125/130 (96.2%)"]:
    if t.lower() not in changelog.lower(): errors.append(f"0.1.16 changelog missing: {t}")
if errors:
    print("GTT 0.1.16 automatic drivetrain verification FAILED"); [print(" -",e) for e in errors]; sys.exit(1)
print(f"GTT 0.1.16 automatic drivetrain verification OK; sealed Win64 delegate verified; roadmap={checked}/{total}")
