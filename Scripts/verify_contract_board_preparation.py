#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

contract_h = read("Source/GTT/Public/World/GTTContractBoardSubsystem.h")
contract_cpp = read("Source/GTT/Private/World/GTTContractBoardSubsystem.cpp")
terminal_h = read("Source/GTT/Public/World/GTTContractBoardTerminal.h")
terminal_cpp = read("Source/GTT/Private/World/GTTContractBoardTerminal.cpp")
farm_h = read("Source/GTT/Public/Activities/GTTFarmJobDirector.h")
farm_cpp = read("Source/GTT/Private/Activities/GTTFarmJobDirector.cpp")
rural_h = read("Source/GTT/Public/Activities/GTTRuralWorkDirector.h")
rural_cpp = read("Source/GTT/Private/Activities/GTTRuralWorkDirector.cpp")
heavy_cpp = read("Source/GTT/Private/Activities/GTTHeavyHaulDirector.cpp")
fleet_h = read("Source/GTT/Public/World/GTTGarageFleetSubsystem.h")
fleet_cpp = read("Source/GTT/Private/World/GTTGarageFleetSubsystem.cpp")
sanity = read(".github/workflows/project-sanity.yml")
playtest = read("Docs/PLAYTEST_0.1.1.md")
changelog = read("CHANGELOG.d/0.1.1.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "offer contract exposed": all(token in contract_h for token in [
        "FGTTContractBoardOffer", "BuildOffer", "TryPrepareContract", "TryAcceptContract",
        "PreparationEstimate", "MaximumNetReward", "IsAnyLegalContractActive"]),
    "four legal offers spawn beside garage": all(token in contract_cpp for token in [
        'FarmCargoJob', 'HeavyHaulJob', 'TimberHaulJob', 'FieldMowingJob',
        'SpawnActor<AGTTContractBoardTerminal>', 'Board->Configure(Entry.JobTag)']),
    "offer uses authoritative fleet readiness": all(token in contract_cpp for token in [
        'AssessJobReadiness(JobTag)', 'GetSlotSnapshot', 'RecommendedRoleForJob', 'MaximumNetReward']),
    "board surfaces readiness label": 'MissionReadinessLabel' in terminal_cpp,
    "real reward ranges documented in runtime catalog": all(token in contract_cpp for token in [
        'return 220;', 'return 355;', 'return 900;', 'return 1150;', 'return 340;', 'return 450;', 'return 390;', 'return 470;']),
    "prep charges real economy": 'SpendCash(Offer.PreparationEstimate' in contract_cpp and 'Fleet preparation refund' in contract_cpp,
    "prep services native road authority": all(token in contract_cpp for token in [
        'ApplyNativeWorkshopService()', 'RepairNativeTires()', 'RefuelNativeVehicle', 'FlushNativePersistenceMirror()']),
    "prep synchronizes native fieldmaster": all(token in contract_cpp for token in [
        'ImportLegacyGameplayState(Legacy, Summary)', 'RecallToTransform(Destination)', 'RustyFieldmaster60']),
    "prep stages native Chaos safely": all(token in contract_cpp for token in [
        'SetThrottleInput(0.0f)', 'SetSteeringInput(0.0f)', 'SetBrakeInput(1.0f)',
        'SetPhysicsLinearVelocity(FVector::ZeroVector)', 'SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector)']),
    "prep commits active and role loadout": 'SetPreferredVehicleId(Snapshot.VehicleId)' in contract_cpp and 'SetRoleLoadoutVehicleId(Snapshot.Role, Snapshot.VehicleId)' in contract_cpp,
    "prep saves progress": 'GameMode->SaveProgress()' in contract_cpp,
    "crime locks protect board": 'GetWantedLevel() > 0' in contract_cpp and 'GetWildlifeAlertLevel() > 0' in contract_cpp,
    "overlapping legal contracts blocked": all(token in contract_cpp for token in [
        'IsJobActive()', 'IsActive()', 'IsWorkActive()', 'Finish the active legal contract']),
    "board starts existing directors": all(token in contract_cpp for token in [
        'TryStartJob(PlayerPawn)', 'TryStartContract(PlayerPawn)', 'TryStartTimber(PlayerPawn)', 'TryStartMowing(PlayerPawn)']),
    "world board shows economics and readiness": all(token in terminal_cpp for token in [
        'CONTRACT BOARD', 'PAY $%d-$%d', 'PREP $%d | NET MAX $%d', 'E - ACCEPT', 'E - PREP']),
    "terminal routes prep then accept": 'TryPrepareContract' in terminal_cpp and 'TryAcceptContract' in terminal_cpp and 'BuildOffer(JobTag)' in terminal_cpp,
    "farm stores fleet payout risk": 'FleetPayoutMultiplier' in farm_h and 'FleetPayoutMultiplier = 0.90f' in farm_cpp and 'FleetPayoutMultiplier = 0.75f' in farm_cpp,
    "farm applies visible payout penalty": 'RawReward * FleetPayoutMultiplier' in farm_cpp and 'UNPREPARED FLEET PENALTY' in farm_cpp and 'PREP PENALTY' in farm_cpp,
    "timber stores fleet payout risk": 'FleetPayoutMultiplier' in rural_h and 'FleetPayoutMultiplier = 0.90f' in rural_cpp and 'FleetPayoutMultiplier = 0.75f' in rural_cpp,
    "timber applies visible payout penalty": '(IntegrityPay + Bonus) * FleetPayoutMultiplier' in rural_cpp and 'UNPREPARED FLEET PENALTY' in rural_cpp,
    "hard tractor gates retained": 'HEAVY HAUL BLOCKED' in heavy_cpp and 'FIELD WORK BLOCKED' in rural_cpp,
    "0.1.0 fleet readiness remains source of truth": all(token in fleet_h + fleet_cpp for token in [
        'FGTTFleetMissionAssessment', 'AssessJobReadiness', 'GetRoleLoadoutVehicleId', 'PrepEstimate']),
    "playtest covers complete player route": all(token in playtest.lower() for token in [
        '0.1.1', 'unified board', 'paid preparation', 'save/load', 'sandbox bypass', 'win64']),
    "changelog documents milestone": all(token in changelog.lower() for token in [
        '0.1.1', 'contract board', 'fleet preparation', '125/130', '96.2%']),
    "sanity wired": 'Verify contract board and fleet preparation economy' in sanity and 'verify_contract_board_preparation.py' in sanity,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Contract board/fleet preparation verification failed: " + ", ".join(failed))

required_style = [
    '<!-- SWIR-ROADMAP-STANDARD:v1 -->', '<!-- ROADMAP-PROGRESS:START -->', '<!-- ROADMAP-PROGRESS:END -->',
    'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"',
    '## 📊 Overall progress', '../assets/readme/progress-mini.svg'
]
for token in required_style:
    if token not in roadmap:
        raise SystemExit("SWIR roadmap style lock missing: " + token)

items = re.findall(r'^- \[(x| )\] ', roadmap, flags=re.MULTILINE | re.IGNORECASE)
done = sum(v.lower() == 'x' for v in items)
total = len(items)
if not total:
    raise SystemExit("Roadmap checklist missing")
remaining = total - done
percent = round(done * 100.0 / total, 1)
for token in (
    f'ROADMAP-{percent:.1f}%25', f'DONE-{done}%2F{total}', 'STATUS-IN%20PROGRESS',
    f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |'):
    if token not in roadmap:
        raise SystemExit("Roadmap dashboard drift: missing " + token)
if roadmap.count('../assets/readme/progress-mini.svg') != 1:
    raise SystemExit("Roadmap dashboard must embed exactly one progress-mini.svg")
if re.search(r'^[\s>*`-]*[█▓▒░▰▱■□▪▫▮▯]{5,}', roadmap, flags=re.MULTILINE):
    raise SystemExit("Legacy text/Unicode roadmap progress meter must not return")
if (done, total, percent) != (125, 130, 96.2):
    raise SystemExit(f"0.1.1 source milestone must not claim build/art gates: {done}/{total} = {percent:.1f}%")

print(f"[OK] GTT 0.1.1 contract board, paid fleet preparation and bypass economics verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}% with SVG-only presentation.")
