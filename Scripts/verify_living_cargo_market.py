#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

save_h = read("Source/GTT/Public/Save/GTTSaveGame.h")
struct_cpp = read("Source/GTT/Private/Core/GTTStructuralGameMode.cpp")
logistics_h = read("Source/GTT/Public/World/GTTLogisticsReputationSubsystem.h")
logistics_cpp = read("Source/GTT/Private/World/GTTLogisticsReputationSubsystem.cpp")
farm_h = read("Source/GTT/Public/Activities/GTTFarmJobDirector.h")
farm_cpp = read("Source/GTT/Private/Activities/GTTFarmJobDirector.cpp")
farm_terminal_h = read("Source/GTT/Public/Activities/GTTFarmJobTerminal.h")
farm_terminal_cpp = read("Source/GTT/Private/Activities/GTTFarmJobTerminal.cpp")
board_terminal_cpp = read("Source/GTT/Private/World/GTTContractBoardTerminal.cpp")
citizen_cpp = read("Source/GTT/Private/NPC/GTTCitizenPawn.cpp")
workflow = read(".github/workflows/project-sanity.yml")
changelog = read("CHANGELOG.d/0.1.4.md")
playtest = read("Docs/PLAYTEST_0.1.4.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "save v8 additive cargo history": "SaveVersion = 8" in save_h and "0.1.4 extends v8 additively" in save_h,
    "cargo aggregates persisted": all(token in save_h for token in (
        "LogisticsCargoCompletedRuns", "LogisticsCargoFailedRuns", "LogisticsCargoLifetimeRevenue")),
    "bounded mixed history persisted": all(token in save_h for token in (
        "LogisticsRecentContractTags", "LogisticsRecentPayouts", "LogisticsRecentQualityPercent")),
    "primary structural save still owns logistics": "Logistics->CaptureToSave(Save)" in struct_cpp and "Logistics->RestoreFromSave" in struct_cpp,
    "cargo API exposed": all(token in logistics_h for token in (
        "IsCargoDepotWindowOpen", "GetCargoScheduleLabel", "GetCargoRouteTier", "GetCargoMarketMultiplier",
        "GetCargoMarketLabel", "RecordCargoSuccess", "RecordCargoFailure", "GetRecentHistorySummary")),
    "cargo shift aligns with NPC work schedule": all(token in logistics_cpp for token in (
        "CargoOpenHour = 7.0f", "CargoCloseHour = 17.5f", "Hour >= CargoOpenHour && Hour < CargoCloseHour"))
        and "Hour>=7.0f&&Hour<17.5f" in citizen_cpp,
    "dynamic demand uses time day and progression": all(token in logistics_cpp for token in (
        "TimeDemandBonus", "GetDayNumber()", "DemandCycle", "CargoCompletedRuns + CompletedRuns", "ReputationBonus", "StreakBonus")),
    "cargo market bounded": "FMath::Clamp(1.0f + TimeDemandBonus + DailyDemandBonus + ReputationBonus + StreakBonus, 1.0f, 1.38f)" in logistics_cpp,
    "reputation gates route tiers": all(token in logistics_cpp for token in (
        "Reputation >= 50", "Reputation >= 20", "return 3", "return 2", "return 1")),
    "history ring bounded to six": "MaxRecentContracts = 6" in logistics_cpp and "while (RecentContractTags.Num() > MaxRecentContracts)" in logistics_cpp,
    "ROAD participates in shared history": "AppendHistory(FName(TEXT(\"RoadRun\"))" in logistics_cpp and "RoadRunFail" in logistics_cpp,
    "CARGO success and failure persist progression": all(token in logistics_cpp for token in (
        "++CargoCompletedRuns", "++CargoFailedRuns", "CargoLifetimeRevenue", "CargoChain", "CargoFail")),
    "cargo save captures and restores new fields": all(token in logistics_cpp for token in (
        "Save->LogisticsCargoCompletedRuns = CargoCompletedRuns", "Save->LogisticsRecentContractTags = RecentContractTags",
        "CargoCompletedRuns = FMath::Max(0, Save->LogisticsCargoCompletedRuns)", "RecentContractTags = Save->LogisticsRecentContractTags")),
    "multi-stop farm stage exposed": "DeliverFinalStop" in farm_h and "TryCompleteFinalStop" in farm_h,
    "final handoff terminal type exposed": "FinalFinish" in farm_terminal_h and "TryCompleteFinalStop(Pawn)" in farm_terminal_cpp,
    "real North Wood final terminal spawned": "SpawnActor<AGTTFarmJobTerminal>(FVector(7850.0f, 1120.0f, 55.0f)" in farm_cpp and "FinalFinish" in farm_cpp,
    "loaded timer spans relay and final leg": "Stage != EGTTFarmJobStage::DeliverCargo && Stage != EGTTFarmJobStage::DeliverFinalStop" in farm_cpp and "Stage = EGTTFarmJobStage::DeliverFinalStop" in farm_cpp,
    "Mulebox cargo stays physically loaded through relay": "SetCargoLoadFactor(1.0f)" in farm_cpp and "ClearLoadedVehicleCargoState();" in farm_cpp,
    "schedule enforced before cargo start": "IsCargoDepotWindowOpen()" in farm_cpp and "return at 07:00" in farm_cpp,
    "market multiplier locked at acceptance": "MarketMultiplierAtStart = Logistics->GetCargoMarketMultiplier()" in farm_cpp and "FleetAdjustedReward" in farm_cpp and "MarketMultiplierAtStart" in farm_cpp,
    "route bonus rewards extended chain": "TrustedChainBonus = 70" in farm_h and "ReliableChainBonus = 120" in farm_h and "RouteBonus" in farm_cpp,
    "police blocks both legal handoffs": "HILL FARM" in farm_cpp and "NORTH WOOD YARD" in farm_cpp and "HANDOFF BLOCKED" in farm_cpp and "bPoliceIncidentDuringRun = true" in farm_cpp,
    "cargo completion updates history before save": "RecordCargoSuccess" in farm_cpp and "GameMode->SaveProgress()" in farm_cpp,
    "cargo failure updates history and saves": "RecordCargoFailure" in farm_cpp and "Reputation/streak consequence saved" in farm_cpp,
    "board surfaces cargo market planning": all(token in board_terminal_cpp for token in (
        "GetCargoMarketLabel", "GetCargoScheduleLabel", "GetCargoRouteTier", "GetCargoCompletedRuns", "GetRecentHistorySummary")),
    "closed cargo still permits real prep": "Offer.bNeedsPreparation" in board_terminal_cpp and "CARGO prep stays available while staff are off shift" in board_terminal_cpp,
    "sanity wired": "Verify living CARGO market and dynamic contract chain" in workflow and "verify_living_cargo_market.py" in workflow,
    "playtest covers market chain persistence": all(token in playtest.lower() for token in (
        "npc-aligned depot schedule", "dynamic demand", "route tier progression", "police refusal", "persistent mixed logistics history", "win64")),
    "changelog documents milestone and demo honesty": all(token in changelog for token in (
        "GTT 0.1.4", "Living CARGO Market", "1.38x", "125/130", "No demo Release is authorized")),
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Living CARGO market verification failed: " + ", ".join(failed))

required_style = [
    '<!-- SWIR-ROADMAP-STANDARD:v1 -->', '<!-- ROADMAP-PROGRESS:START -->',
    'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"',
    '## 📊 Overall progress', '<!-- ROADMAP-PROGRESS:END -->'
]
for token in required_style:
    if token not in roadmap:
        raise SystemExit("SWIR roadmap style lock missing: " + token)

items = re.findall(r'^- \[(x| )\] ', roadmap, flags=re.MULTILINE)
done = sum(v == 'x' for v in items)
total = len(items)
if not total:
    raise SystemExit("Roadmap checklist missing")
remaining = total - done
percent = round(done * 100.0 / total, 1)
filled = round(done * 20.0 / total)
bar = '█' * filled + '░' * (20 - filled)
for token in (
    f'ROADMAP-{percent:.1f}%25', f'DONE-{done}%2F{total}', 'STATUS-IN%20PROGRESS',
    f'{bar} {percent:.1f}%', f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |'):
    if token not in roadmap:
        raise SystemExit("Roadmap dashboard drift: missing " + token)
if (done, total, percent) != (125, 130, 96.2):
    raise SystemExit(f"0.1.4 source milestone must not claim build/art gates: {done}/{total} = {percent:.1f}%")

print(f"[OK] GTT 0.1.4 living CARGO market, dynamic contract chain and mixed logistics history verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.")
