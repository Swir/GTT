#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

save_h = read("Source/GTT/Public/Save/GTTSaveGame.h")
struct_cpp = read("Source/GTT/Private/Core/GTTStructuralGameMode.cpp")
logistics_h = read("Source/GTT/Public/World/GTTLogisticsReputationSubsystem.h")
logistics_cpp = read("Source/GTT/Private/World/GTTLogisticsReputationSubsystem.cpp")
road_h = read("Source/GTT/Public/Activities/GTTRoadRunDirector.h")
road_cpp = read("Source/GTT/Private/Activities/GTTRoadRunDirector.cpp")
contract_h = read("Source/GTT/Public/World/GTTContractBoardSubsystem.h")
contract_cpp = read("Source/GTT/Private/World/GTTContractBoardSubsystem.cpp")
terminal_cpp = read("Source/GTT/Private/World/GTTContractBoardTerminal.cpp")
workflow = read(".github/workflows/project-sanity.yml")
changelog = read("CHANGELOG.d/0.1.3.md")
playtest = read("Docs/PLAYTEST_0.1.3.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "save schema v8": "SaveVersion = 8" in save_h and "v8 persists rural logistics reputation/history" in save_h,
    "save persists logistics record": all(token in save_h for token in (
        "LogisticsReputation", "LogisticsCleanStreak", "LogisticsCompletedRuns", "LogisticsFailedRuns", "LogisticsLifetimeRevenue")),
    "older save compatibility gates retained": all(token in struct_cpp for token in (
        "StructuralDamageSaveVersion = 5", "FleetDispatchSaveVersion = 6", "MissionLoadoutSaveVersion = 7",
        "LogisticsReputationSaveVersion = 8", "ExtendedSaveVersion = 8")),
    "primary save captures logistics": "Logistics->CaptureToSave(Save)" in struct_cpp,
    "primary load restores logistics": "Logistics->RestoreFromSave" in struct_cpp and "Save->SaveVersion >= LogisticsReputationSaveVersion" in struct_cpp,
    "logistics subsystem exposes persistent progression": all(token in logistics_h for token in (
        "GetReputation", "GetCleanStreak", "GetCompletedRuns", "GetFailedRuns", "GetLifetimeRevenue",
        "RecordCourierSuccess", "RecordCourierFailure", "CaptureToSave", "RestoreFromSave")),
    "courier has real day window": all(token in logistics_cpp for token in (
        "CourierOpenHour = 6.0f", "CourierCloseHour = 21.5f", "Hour >= CourierOpenHour && Hour < CourierCloseHour")),
    "late shift uses existing clock": "LateShiftHour = 18.5f" in logistics_cpp and "GameMode->GetDayNightCycle()" in logistics_cpp and "LATE SHIFT +10%" in logistics_cpp,
    "bounded reputation reward multiplier": all(token in logistics_cpp for token in (
        "FMath::Min(0.20f", "FMath::Min(0.05f", "ShiftMultiplier = IsLateShift() ? 1.10f : 1.0f")),
    "delivery quality drives reputation": all(token in logistics_cpp for token in (
        "ParcelIntegrity >= 0.97f", "!bPoliceIncident", "NativeImpacts <= 0", "ReputationDelta", "CleanStreak")),
    "failed deliveries persist consequence state": "RecordCourierFailure" in logistics_cpp and "Reputation - (bSevereFailure ? 12 : 8)" in logistics_cpp,
    "multi-stop stage exposed": all(token in road_h for token in ("CollectParts", "RelayHillFarm", "DeliverParts", "HillFarmRelayLocation")),
    "real Hill Farm relay marker": all(token in road_cpp for token in (
        "HILL FARM\\nCOURIER RELAY", "SetMarkerState(false, true, false)", "SetMarkerState(false, false, true)", "CompleteRelay")),
    "timer and integrity continue across loaded stages": "Stage != EGTTRoadRunStage::RelayHillFarm && Stage != EGTTRoadRunStage::DeliverParts" in road_cpp and "TimeRemaining = FMath::Max(0.0f, TimeRemaining - DeltaSeconds)" in road_cpp and "UpdateDeliveryRisk" in road_cpp,
    "police incident remembered for run": "bPoliceIncidentDuringRun = true" in road_cpp and "COURIER RELAY BLOCKED" in road_cpp and "COURIER HANDOFF BLOCKED" in road_cpp,
    "schedule enforced before start": "IsRoadCourierWindowOpen()" in road_cpp and "PARTS DEPOT CLOSED" in road_cpp,
    "payout multiplier locked at acceptance": "RewardMultiplierAtStart = Logistics->GetRoadCourierRewardMultiplier()" in road_cpp and "RawReward" in road_cpp and "RewardMultiplierAtStart" in road_cpp,
    "success updates progression before save": "RecordCourierSuccess" in road_cpp and "GameMode->SaveProgress()" in road_cpp,
    "failure updates progression and saves": "RecordCourierFailure" in road_cpp and "Reputation/streak consequence saved" in road_cpp,
    "board offer exposes schedule and reputation": all(token in contract_h for token in (
        "bScheduleOpen", "ScheduleStatus", "LogisticsReputation", "LogisticsTier", "PayoutBonusPercent")),
    "board prices current reputation and shift": "GetRoadCourierRewardMultiplier" in contract_cpp and "Offer.PayoutBonusPercent" in contract_cpp and "Offer.MaximumReward = FMath::RoundToInt" in contract_cpp,
    "board acceptance obeys schedule": "Offer.bCanAcceptNow = bFleetReady && Offer.bScheduleOpen" in contract_cpp and "JobTag == RoadRunJob && !Offer.bScheduleOpen" in contract_cpp,
    "closed shift still allows genuine prep only": "Offer.bNeedsPreparation = !bFleetReady" in contract_cpp and "Vehicle preparation remains available before the next shift" in contract_cpp,
    "ROAD economics reflect added route": "if (JobTag == RoadRunJob) return 310;" in contract_cpp and "if (JobTag == RoadRunJob) return 460;" in contract_cpp,
    "terminal has compact logistics row": "REP %s %d | BONUS %d%% | %s" in terminal_cpp and "CLOSED" in terminal_cpp,
    "terminal does not fake prep when ready but closed": "else if (Offer.bNeedsPreparation)" in terminal_cpp and "else if (!Offer.bScheduleOpen)" in terminal_cpp,
    "sanity wired": "Verify living rural logistics and traffic reputation" in workflow and "verify_living_logistics_reputation.py" in workflow,
    "playtest covers runtime and migration": all(token in playtest.lower() for token in (
        "multi-stop route", "traffic, condition and crash risk", "police incident", "reputation and payout", "save/load schema v8", "win64/demo evidence")),
    "changelog documents milestone and demo honesty": all(token in changelog for token in (
        "GTT 0.1.3", "Living Rural Logistics", "schema v8", "125/130", "No demo Release is authorized")),
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Living logistics/reputation verification failed: " + ", ".join(failed))

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
    raise SystemExit(f"0.1.3 source milestone must not claim build/art gates: {done}/{total} = {percent:.1f}%")

print(f"[OK] GTT 0.1.3 living rural logistics, multi-stop ROAD route, schedule economy and persistent reputation verified ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}%.")
