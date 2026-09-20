#!/usr/bin/env python3
from pathlib import Path
import re,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
def read(rel):
    p=ROOT/rel
    if not p.is_file(): raise AssertionError(f"missing required file: {rel}")
    return p.read_text(encoding="utf-8")
def require(rel,*tokens):
    d=read(rel)
    for t in tokens:
        if t not in d: raise AssertionError(f"{rel} missing token: {t}")
    return d
def main():
    base=require("Source/GTT/Public/Vehicles/GTTVehicleBase.h","AssignPersistentVehicleIdForInstance","NewPersistentVehicleId.IsNone() || bOwnedByPlayer || bOccupied","PersistentVehicleId = NewPersistentVehicleId")
    identity_h=require("Source/GTT/Public/Vehicles/GTTVehicleIdentitySubsystem.h","UGTTVehicleIdentitySubsystem","UTickableWorldSubsystem","RefreshFleetIdentity","CollisionRepairCount")
    identity_cpp=require("Source/GTT/Private/Vehicles/GTTVehicleIdentitySubsystem.cpp","VEHICLE_IDENTITY event=OWNED_COLLISION result=UNRESOLVED","persisted_id_is_immutable","BuildUniqueInstanceId","ObservedVehicles","AssignPersistentVehicleIdForInstance","VEHICLE_IDENTITY event=ASSIGN result=PASS")
    scenario_h=require("Source/GTT/Public/Core/GTTFarmCargoRecoveryEvidenceSubsystem.h","UGTTFarmCargoRecoveryEvidenceSubsystem","SaveLoaded","ReloadLoaded","WrongVehicleAfterReload","SaveRelay","ReloadRelay","CompletionReload")
    scenario=require("Source/GTT/Private/Core/GTTFarmCargoRecoveryEvidenceSubsystem.cpp",'FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRecoveryScenario"))',"StartDelaySeconds = 200.0f","GlobalDeadlineSeconds = 224.0f","GTTFarmCargoRecoveryPrimary_0_1_31","GTTFarmCargoRecoveryDecoy_0_1_31","AssignPersistentVehicleIdForInstance","VehicleIdentity->RefreshFleetIdentity()","GameMode && GameMode->SaveProgress()","GameMode && GameMode->LoadProgress()","SpawnedPickupVan->Destroy()","DisturbCargoRuntimeState()","wrong-vehicle-after-reload-was-not-rejected","FARM_CARGO_RECOVERY_RUNTIME phase=RELOAD_LOADED","FARM_CARGO_RECOVERY_RUNTIME phase=RELOAD_RELAY","FARM_CARGO_RECOVERY_RUNTIME phase=COMPLETION_RELOAD","FARM_CARGO_RECOVERY_RUNTIME_COMPLETE result=%s route=feed-hill-wood")
    if "AddCash(" in scenario or "SettleCargoContract(" in scenario: raise AssertionError("recovery harness must observe authoritative payout")
    recovery=require("Scripts/evaluate_farm_cargo_recovery_runtime.ps1","gtt.farm-cargo-recovery-runtime.v1","FARM_CARGO_RECOVERY_RUNTIME.json","-GTTFarmCargoRecoveryScenario","RELOAD_LOADED","WRONG_VEHICLE_AFTER_RELOAD","RELOAD_RELAY","COMPLETION_RELOAD","Require-IntegerField","stable_vehicle_id","diagnostic_failure_count")
    smoke=require("Scripts/smoke_test_windows.ps1","-GTTFarmCargoRecoveryScenario","farm_cargo_recovery_runtime_scenario = $true")
    workflow=require(".github/workflows/win64-package-evidence.yml","run_win64_attested_candidate_acceptance.ps1","WIN64_CANDIDATE_ATTESTATION.json")
    runner=require("Scripts/run_win64_candidate_acceptance.ps1","evaluate_farm_cargo_runtime.ps1","evaluate_farm_cargo_recovery_runtime.ps1","evaluate_demo_candidate.ps1","smoke_test_windows.ps1")
    timing=re.search(r'"-MinimumAliveSeconds",\s*(\d+).*?"-LaunchTimeoutSeconds",\s*(\d+)',runner,flags=re.S); gameplay=re.search(r'"-MinimumRuntimeSeconds",\s*(\d+)',runner)
    if not timing or not gameplay: raise AssertionError("canonical exact-candidate runner does not expose recovery evidence timing")
    alive,timeout=map(int,timing.groups()); game=int(gameplay.group(1)); assert alive>=228 and timeout>alive and timeout>=250 and game>=alive
    assert runner.index("evaluate_farm_cargo_runtime.ps1")<runner.index("evaluate_farm_cargo_recovery_runtime.ps1")<runner.index("evaluate_demo_candidate.ps1")
    candidate=require("Scripts/evaluate_demo_candidate.ps1","FARM_CARGO_RECOVERY_RUNTIME.json","gtt.farm-cargo-recovery-runtime.v1","farm_cargo_recovery_runtime='PASS'","farm_cargo_recovery_loaded_rebind","farm_cargo_recovery_relay_rebind","farm_cargo_recovery_completion_reload")
    schema=re.search(r"(?m)^\s*schema=(\d+)\s*$",candidate); assert schema and int(schema.group(1))>=8
    persistence=require("Source/GTT/Private/Activities/GTTFarmJobPersistence.cpp","RestoreActiveCargoFromSave","AdoptRestoredCargoVehicle","FARM_CARGO_RECOVERY event=RESTORE result=PASS"); authority=require("Source/GTT/Private/Activities/GTTFarmCargoAuthoritySubsystem.cpp","ResolveVehicleByPersistentId(BoundCargoVehicleId)","FARM_CARGO_RECOVERY event=REBIND result=PASS","Director->AdoptRestoredCargoVehicle(Resolved)")
    playtest=read("Docs/PLAYTEST_0.1.31.md"); changelog=read("CHANGELOG.d/0.1.31.md")
    for t in ["actor recreation","wrong vehicle","loaded checkpoint","relay checkpoint","completion reload","Win64"]: assert t.lower() in playtest.lower()
    for t in ["GTT 0.1.31","Fleet Identity","FARM_CARGO_RECOVERY_RUNTIME.json","does not claim"]: assert t.lower() in changelog.lower()
    roadmap=read("Docs/ROADMAP.md"); checks=re.findall(r"^- \[([ xX])\]",roadmap,flags=re.M); done=sum(1 for m in checks if m.lower()=="x"); total=len(checks); assert (done,total)==(125,130); assert roadmap.count("../assets/readme/progress-mini.svg")==1; assert not re.search(r"^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}",roadmap,flags=re.M)
    readme=require("README.md","<!-- SWIR-README-STANDARD:v2 -->","## 🔎 Search Keywords","assets/readme/progress-card.svg","0.1.31","FARM_CARGO_RECOVERY_RUNTIME.json","No public demo release is available yet."); assert "release readiness remains not ready" in readme.lower()
    subprocess.run([sys.executable,str(ROOT/"Scripts/generate_progress_svg.py"),"--check"],check=True)
    print(f"[OK] GTT 0.1.31 Farm Cargo recovery evidence via sealed runner: runtime={alive}/{timeout}s; roadmap={done}/{total}")
if __name__=="__main__": main()
