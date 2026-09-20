#!/usr/bin/env python3
"""Source-contract verifier for GTT 0.1.52 packaged workshop priority/pickup evidence."""
from __future__ import annotations
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(rel):
    p=ROOT/rel
    if not p.is_file(): raise AssertionError(f"missing required file: {rel}")
    return p.read_text(encoding="utf-8")
def require(text,needles,label):
    missing=[n for n in needles if n not in text]
    if missing: raise AssertionError(f"{label}: missing {missing}")
def main():
    runtime_h=read("Source/GTT/Public/Core/GTTWorkshopPriorityPickupRuntimeEvidenceSubsystem.h"); runtime_cpp=read("Source/GTT/Private/Core/GTTWorkshopPriorityPickupRuntimeEvidenceSubsystem.cpp"); bridge_h=read("Source/GTT/Public/Core/GTTWorkshopLegacyEvidencePickupBridgeSubsystem.h"); bridge_cpp=read("Source/GTT/Private/Core/GTTWorkshopLegacyEvidencePickupBridgeSubsystem.cpp"); queue_h=read("Source/GTT/Public/World/GTTWorkshopRepairQueueSubsystem.h"); queue_cpp=read("Source/GTT/Private/World/GTTWorkshopRepairQueueSubsystem.cpp"); smoke=read("Scripts/smoke_test_windows.ps1"); evaluator=read("Scripts/evaluate_workshop_priority_pickup_runtime.ps1"); promoter=read("Scripts/promote_demo_gate_workshop_priority_pickup.ps1"); win64=read(".github/workflows/win64-package-evidence.yml"); runner=read("Scripts/run_win64_candidate_acceptance.ps1"); attestor=read("Scripts/write_win64_candidate_attestation.ps1"); playtest=read("Docs/PLAYTEST_0.1.52.md"); changelog=read("CHANGELOG.d/0.1.52.md"); roadmap=read("Docs/ROADMAP.md")
    require(runtime_h,["UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem","PreparePriority","AwaitCheckIn","AwaitCheckout","VerifyPickup","bWrongIdRejected","bNoSecondCharge"],"runtime header")
    require(runtime_cpp,["GTTWorkshopPriorityPickupRuntimeScenario","WORKSHOP_PRIORITY_PICKUP_RUNTIME_BEGIN","WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=PRIORITY","WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=CHECKIN","WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=CHECKOUT","WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=PICKUP","WORKSHOP_PRIORITY_PICKUP_RUNTIME_COMPLETE","TryQueueNearestEligibleNativeRoadVehicle","PromoteQueuedRepairToUrgent","ReleaseCompletedRepairForPickup","VerifyPriorityCheckpoint(false)","VerifyPriorityCheckpoint(true)","ExpectedUrgentSurchargePercent = 20","ExpectedUrgentMultiplier = 0.80f","CashBeforePickup","VerifyPrimaryCargoContinuity"],"runtime implementation")
    require(queue_h,["UrgentQuoteSurchargePercent = 20","UrgentServiceDurationMultiplier = 0.80f","PromoteQueuedRepairToUrgent","ReleaseCompletedRepairForPickup","IsVehicleAwaitingPickup"],"production queue API")
    require(queue_cpp,["WORKSHOP_QUEUE_PRIORITY_UPGRADED","WORKSHOP_QUEUE_CHECKED_IN","WORKSHOP_QUEUE_READY_FOR_PICKUP","WORKSHOP_QUEUE_PICKUP_RELEASED"],"production queue markers")
    require(bridge_h,["bPriorityPickupEvidence","Elapsed"],"legacy evidence isolation header")
    require(bridge_cpp,["GTTWorkshopPriorityPickupRuntimeScenario","PriorityPickupEvidenceStartSeconds = 451.0f","LegacyBridgeStopMarginSeconds = 2.0f","priority_pickup_isolation"],"legacy evidence isolation")
    require(smoke,["-GTTWorkshopPriorityPickupRuntimeScenario","workshop_priority_pickup_runtime_scenario = $true"],"packaged smoke route")
    require(evaluator,["gtt.workshop-priority-pickup-runtime.v1","WORKSHOP_PRIORITY_PICKUP_RUNTIME.json","urgent_surcharge_percent=20","urgent_service_multiplier=0.80","exact_id_priority_promotion=$true","single_locked_quote_debit=$true","ready_for_pickup_persisted=$true","wrong_id_pickup_rejected=$true","exact_id_pickup_release=$true","pickup_has_no_second_charge=$true"],"runtime evaluator")
    require(promoter,["schema -ne 16","$gate.schema=17","workshop_capacity_runtime","workshop_priority_pickup_runtime='PASS'","WORKSHOP_PRIORITY_PICKUP_RUNTIME.json"],"schema-17 promoter")
    m=re.search(r"default:\s*['\"]([0-9]+)\.([0-9]+)\.([0-9]+)['\"]",win64); assert m and tuple(map(int,m.groups()))>=(0,1,52)
    require(win64,["run_win64_attested_candidate_acceptance.ps1","WIN64_CANDIDATE_ATTESTATION.json"],"sealed Win64 delegate")
    require(runner,["evaluate_workshop_priority_pickup_runtime.ps1","promote_demo_gate_workshop_priority_pickup.ps1"],"canonical runner")
    require(attestor,["WORKSHOP_PRIORITY_PICKUP_RUNTIME.json"],"candidate attestor")
    require(playtest,["72-case matrix","STANDARD","URGENT","+20%","x0.80","READY_FOR_PICKUP","wrong-ID","single debit","No packaged Win64 proof is claimed"],"0.1.52 playtest")
    require(changelog,["0.1.52","WORKSHOP_PRIORITY_PICKUP_RUNTIME.json","schema 17","+20%","x0.80","READY_FOR_PICKUP","125 / 130 (96.2%)","Win64"],"0.1.52 changelog")
    done=len(re.findall(r"^- \[x\] ",roadmap,flags=re.M|re.I)); open_=len(re.findall(r"^- \[ \] ",roadmap,flags=re.M)); assert (done,open_)==(125,5)
    require(roadmap,["<!-- SWIR-ROADMAP-STANDARD:v1 -->","../assets/readme/progress-mini.svg","| **125** | **5** | **130** | **96.2%** |"],"roadmap presentation")
    assert not re.search(r"[█▓▒░]{4,}|(?:\[[#=\-]{6,}\])",roadmap)
    print(f"GTT 0.1.52 workshop priority/pickup source contract: PASS; sealed runner; roadmap {done}/{done+open_}"); return 0
if __name__=="__main__": raise SystemExit(main())
