#!/usr/bin/env python3
"""Fail-closed source contract for GTT 0.1.69 audit evidence orchestration/provenance."""

from __future__ import annotations

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

def require(text: str, token: str, scope: str) -> None:
    if token not in text:
        raise SystemExit(f"FAIL: {scope} missing required token: {token}")

def reject(text: str, token: str, scope: str) -> None:
    if token in text:
        raise SystemExit(f"FAIL: {scope} retained forbidden token: {token}")

core_h = read("Source/GTT/Public/Core/GTTDemoSmokeScenarioSubsystem.h")
core_cpp = read("Source/GTT/Private/Core/GTTDemoSmokeScenarioSubsystem.cpp")
service_h = read("Source/GTT/Public/World/GTTServiceTerminal.h")
service_cpp = read("Source/GTT/Private/World/GTTServiceTerminal.cpp")
damage_h = read("Source/GTT/Public/Core/GTTDamageRecoveryEvidenceSubsystem.h")
damage_cpp = read("Source/GTT/Private/Core/GTTDamageRecoveryEvidenceSubsystem.cpp")
struct_h = read("Source/GTT/Public/Core/GTTStructuralDamageEvidenceSubsystem.h")
struct_cpp = read("Source/GTT/Private/Core/GTTStructuralDamageEvidenceSubsystem.cpp")
drive_h = read("Source/GTT/Public/Vehicles/GTTStructuralDriveConsequenceSubsystem.h")
drive_cpp = read("Source/GTT/Private/Vehicles/GTTStructuralDriveConsequenceSubsystem.cpp")
evaluator = read("Scripts/evaluate_demo_scenario.ps1")
candidate = read("Scripts/run_win64_candidate_acceptance.ps1")
attest = read("Scripts/write_win64_candidate_attestation.ps1")
archive = read("Scripts/verify_win64_candidate_archive.ps1")
archive_fixture = read("Scripts/test_win64_candidate_archive_verifier.ps1")
project_sanity = read(".github/workflows/project-sanity.yml")

for token in ("DidCompleteSuccessfully()", "GetProvenRoadblockVehicleId()", "bScenarioPassed", "ProvenRoadblockVehicleId"):
    require(core_h + core_cpp, token, "core demo scenario")
if re.search(r"ProvenRoadblockVehicleId\s*=\s*Vehicle->GetPersistentVehicleId\(\)\s*;", core_cpp) is None:
    raise SystemExit("FAIL: core exact roadblock vehicle handoff missing")
require(core_cpp, "bScenarioPassed=true", "core explicit PASS state")

for token in ("ResolveNativeRoadServiceTarget()", "FindActiveNativeRoadVehicle"):
    require(service_h + service_cpp, token, "production workshop target resolver")
require(service_cpp, "ResolveNativeRoadServiceTarget()", "production workshop interaction")

for header, scope in ((damage_h, "damage recovery"), (struct_h, "structural recovery"), (drive_h, "structural drive")):
    require(header, "DidCompleteSuccessfully()", scope)
    require(header, "GetEvidenceVehicleId()", scope)
    require(header, "ExpectedWorkshopQuote", scope)

reject(damage_cpp, r'\n#include', "damage recovery includes")
for text, scope, fixed in (
    (damage_cpp, "damage recovery", "GetCash() < 250"),
    (struct_cpp, "structural recovery", "GetCash() < 450"),
    (drive_cpp, "structural drive", "GetCash() < 550"),
):
    reject(text, fixed, scope)
    for token in ("GetNativeRoadCheckoutQuote", "ResolveNativeRoadServiceTarget", "NeedsNativeWorkshopService"):
        require(text, token, scope)

for token in ("DidCompleteSuccessfully()", "GetProvenRoadblockVehicleId()", "FindRoadVehicleById"):
    require(damage_cpp, token, "damage recovery predecessor/target binding")
for token in ("DidCompleteSuccessfully()", "GetEvidenceVehicleId()", "FindRoadVehicleById"):
    require(struct_cpp, token, "structural recovery predecessor/target binding")
    require(drive_cpp, token, "structural drive predecessor/target binding")

def require_exact_workshop_debit(text: str, scope: str) -> None:
    require(text, "ExpectedWorkshopQuote", scope)
    require(text, "CashBeforeWorkshop", scope)
    direct = re.search(
        r"(CashBeforeWorkshop\s*-\s*(?:CashAfterWorkshop|CashAfter|Economy->GetCash\(\)))\s*(?:==|!=)\s*ExpectedWorkshopQuote",
        text,
    )
    paid_capture = re.search(
        r"const\s+int32\s+Paid\s*=\s*CashBeforeWorkshop\s*-\s*(?:CashAfterWorkshop|CashAfter|Economy->GetCash\(\))\s*;",
        text,
    )
    paid_gate = re.search(r"Paid\s*(?:==|!=)\s*ExpectedWorkshopQuote", text)
    if not direct and not (paid_capture and paid_gate):
        raise SystemExit(f"FAIL: {scope} does not require exact workshop debit equality")

for text, scope in ((damage_cpp, "damage recovery"), (struct_cpp, "structural recovery"), (drive_cpp, "structural drive")):
    require_exact_workshop_debit(text, scope)

for token in ("expected_quote", "native_spike_vehicle", "structural_paid", "structural_drive_paid"):
    require(evaluator, token, "demo scenario evaluator")
if not re.search(r"nativeSpikeVehicle.*(workshop|structural)", evaluator, re.IGNORECASE | re.DOTALL):
    raise SystemExit("FAIL: evaluator lacks cross-stage exact-vehicle coherence")

for name in ("GTT_Fieldmaster60_Rig.gltf", "GTT_Rattleback82_Rig.gltf", "GTT_Mulebox1200_Rig.gltf"):
    require(candidate, name, "candidate Native rig provenance")
for token in ("source_gltf_bytes", "source_gltf_sha256", "Get-FileHash -Algorithm SHA256", "NATIVE_VEHICLE_RIG_EDITOR_ACCEPTANCE.json"):
    require(candidate, token, "candidate Native rig provenance")

for token in ("NATIVE_VEHICLE_RIG_EDITOR_ACCEPTANCE.json", "gtt.native-vehicle-rig-editor-acceptance.v1", "native_vehicle_rig_sources", "source_gltf_sha256"):
    require(attest, token, "candidate attestation Native rig provenance")
    require(archive, token, "archive Native rig provenance")
require(archive, "hash/byte-bound", "archive Native rig evidence binding")
for token in ('Write-Fixture -Name "bad-rig-provenance"', 'Write-Fixture -Name "missing-rig-binding"'):
    require(archive_fixture, token, "archive provenance fixture")

require(project_sanity, "verify_cdo_physics_safety.py", "project sanity CDO/cook guard")
require(project_sanity, "verify_audit_evidence_orchestration.py", "project sanity audit guard")

print("GTT audit evidence orchestration/provenance source contract: PASS")
