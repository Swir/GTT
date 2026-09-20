#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.42 packaged garage/workshop recovery evidence."""
from __future__ import annotations
import re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def text(path):
    p=ROOT/path
    if not p.is_file(): raise AssertionError(f"missing required file: {path}")
    return p.read_text(encoding="utf-8")
def require(src,needle,label):
    if needle not in src: raise AssertionError(f"missing {label}: {needle}")
def main():
    evidence_h=text("Source/GTT/Public/Core/GTTFarmCargoWorkshopRecoveryEvidenceSubsystem.h"); evidence_cpp=text("Source/GTT/Private/Core/GTTFarmCargoWorkshopRecoveryEvidenceSubsystem.cpp"); service_h=text("Source/GTT/Public/World/GTTServiceTerminal.h"); service_cpp=text("Source/GTT/Private/World/GTTServiceTerminal.cpp"); garage_h=text("Source/GTT/Public/World/GTTGarageFleetSubsystem.h"); garage_slot=text("Source/GTT/Private/World/GTTGarageSlotTerminal.cpp"); roadside=text("Source/GTT/Private/Vehicles/GTTRoadsideRecoverySubsystem.cpp"); evaluator=text("Scripts/evaluate_farm_cargo_workshop_recovery_runtime.ps1"); promote=text("Scripts/promote_demo_gate_workshop_recovery.ps1"); smoke=text("Scripts/smoke_test_windows.ps1"); win64=text(".github/workflows/win64-package-evidence.yml"); runner=text("Scripts/run_win64_candidate_acceptance.ps1"); attestor=text("Scripts/write_win64_candidate_attestation.ps1"); roadmap=text("Docs/ROADMAP.md"); readme=text("README.md")
    for s,n,l in [(evidence_h,"UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem","workshop runtime subsystem"),(evidence_cpp,'TEXT("GTTFarmCargoWorkshopRecoveryScenario")',"dedicated smoke flag"),(evidence_cpp,"constexpr float StartDelaySeconds = 356.0f","ordered evidence window"),(evidence_cpp,"constexpr float GlobalDeadlineSeconds = 382.0f","bounded evidence deadline"),(evidence_cpp,"FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME_BEGIN version=1","runtime begin marker"),(evidence_cpp,"FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME_COMPLETE","runtime completion marker"),(evidence_cpp,"Roadside->RequestRoadsideTow(NativeMulebox.Get())","production tow request"),(evidence_cpp,"Authority->GetBoundCargoVehicleId() == LoadedVehicleId","exact cargo identity"),(garage_h,"IsVehicleOnWorkshopHold(FName VehicleId) const","fleet workshop-hold API"),(garage_slot,"GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(Snapshot)","garage hold policy"),(service_h,"EGTTServiceType GetServiceType() const","service terminal type query"),(service_cpp,"ApplyNativeWorkshopService()","authoritative native workshop service"),(service_cpp,"GameMode->SaveProgress()","workshop persistence checkpoint"),(evaluator,"gtt.farm-cargo-workshop-recovery-runtime.v1","runtime evidence schema"),(evaluator,"FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json","runtime evidence output"),(promote,"schema=13","technical gate schema 13"),(smoke,"-GTTFarmCargoWorkshopRecoveryScenario","packaged smoke launch flag")]: require(s,n,l)
    if "Economy->AddCash(" in evidence_cpp or "Economy->SpendCash(" in evidence_cpp: raise AssertionError("runtime route must observe production economy mutations")
    m=re.search(r"default:\s*'([0-9]+)\.([0-9]+)\.([0-9]+)'",win64); assert m and tuple(map(int,m.groups()))>=(0,1,42)
    timing=re.search(r'"-MinimumAliveSeconds",\s*(\d+).*?"-LaunchTimeoutSeconds",\s*(\d+)',runner,flags=re.S); assert timing,"could not parse canonical Win64 smoke runtime window"; alive,timeout=map(int,timing.groups()); assert alive>=386 and timeout>=415 and timeout>alive
    for t in ["run_win64_attested_candidate_acceptance.ps1","WIN64_CANDIDATE_ATTESTATION.json"]: require(win64,t,"sealed Win64 delegate")
    for t in ["evaluate_farm_cargo_workshop_recovery_runtime.ps1","promote_demo_gate_workshop_recovery.ps1"]: require(runner,t,"canonical runner")
    require(attestor,"FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME.json","candidate attestor workshop evidence")
    schemas=[int(v) for v in re.findall(r"gate\.schema\s+-ne\s+(\d+)",runner)]; assert schemas and max(schemas)>=13
    require(roadside,"damage_preserved=%s","tow damage marker"); require(roadside,"identity_preserved=%s","tow identity marker"); require(roadside,"serviced=NO destination=WORKSHOP","ordinary tow remains unserviced")
    require(roadmap,"<!-- SWIR-ROADMAP-STANDARD:v1 -->","roadmap marker"); require(roadmap,"../assets/readme/progress-mini.svg","roadmap mini"); require(roadmap,"| **125** | **5** | **130** | **96.2%** |","roadmap count"); require(readme,"<!-- SWIR-README-STANDARD:v2 -->","README marker"); require(readme,"assets/readme/progress-card.svg","README card"); require(readme,"## 🔎 Search Keywords","README keywords")
    block=roadmap.split("<!-- ROADMAP-PROGRESS:START -->",1)[1].split("<!-- ROADMAP-PROGRESS:END -->",1)[0]; assert not re.search(r"[█▓▒░]{4,}|\[[#=\-]{5,}\]",block)
    print(f"GTT 0.1.42 packaged garage/workshop recovery evidence: PASS; sealed runner window={alive}/{timeout}s"); return 0
if __name__=="__main__":
    try: raise SystemExit(main())
    except (AssertionError,IndexError,ValueError) as exc: print(f"GTT 0.1.42 packaged garage/workshop recovery evidence: FAIL: {exc}",file=sys.stderr); raise SystemExit(1)
