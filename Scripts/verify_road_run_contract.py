#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

header = read("Source/GTT/Public/Activities/GTTRoadRunDirector.h")
director = read("Source/GTT/Private/Activities/GTTRoadRunDirector.cpp")
contracts = read("Source/GTT/Private/World/GTTContractBoardSubsystem.cpp")
fleet = read("Source/GTT/Private/World/GTTGarageFleetSubsystem.cpp")
hud = read("Source/GTT/Private/UI/GTTGameHUD.cpp")
workflow = read(".github/workflows/project-sanity.yml")
changelog = read("CHANGELOG.d/0.1.2.md")
playtest = read("Docs/PLAYTEST_0.1.2.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "director retains complete courier stages": all(token in header for token in ("Idle", "CollectParts", "RelayHillFarm", "DeliverParts", "TryStartContract", "GetParcelIntegrity", "GetObjectiveText")),
    "ROAD role maps to Rattleback": 'if (JobTag == RoadRunJob) return RattlebackId;' in fleet and 'if (JobTag == RoadRunJob) return EGTTGarageFleetRole::Road;' in fleet,
    "ROAD role has real readiness thresholds": "OutCondition = 0.55f;" in fleet and "OutFuel = 0.35f;" in fleet and "OutTires = 0.55f;" in fleet and "OutBody = 0.60f;" in fleet,
    "contract board owns RoadRun integration": 'const FName RoadRunJob(TEXT("RoadRun"));' in contracts and 'Activities/GTTRoadRunDirector.h' in contracts,
    "contract board spawns missing director": "bHasRoadRunDirector" in contracts and "SpawnActor<AGTTRoadRunDirector>" in contracts,
    "fifth contract board is spawned": "{ RoadRunJob, FVector(-2050.0f, -470.0f, 55.0f) }" in contracts,
    "road courier retains player-facing title and meaningful payout": 'return TEXT("Village Parts Courier")' in contracts and "if (JobTag == RoadRunJob) return 310;" in contracts and "if (JobTag == RoadRunJob) return 460;" in contracts,
    "road courier participates in legal overlap lock": "TActorIterator<AGTTRoadRunDirector>" in contracts and "It->IsActive()" in contracts,
    "board starts real RoadRun director": "bStarted = It->TryStartContract(PlayerPawn)" in contracts,
    "prep summary reports net value": "Updated.MaximumNetReward" in contracts,
    "director enforces Rattleback identity": 'const FName RattlebackId(TEXT("Rattleback82"));' in director and "GetPersistentVehicleId() == RattlebackId" in director,
    "director supports native takeover and legacy fallback": "AGTTRoadVehicleNativePawn" in director and "AGTTVehicleBase" in director and "IsLegacyTakeoverActive()" in director,
    "pickup and drop markers remain compact gameplay guidance": all(token in director for token in ("PARTS DEPOT\\nCOURIER PICKUP", "NORTH WOOD YARD\\nCOURIER DROP", "SetMarkerState(true, false, false)", "SetMarkerState(false, false, true)")),
    "production HUD surfaces active courier objective": 'Activities/GTTRoadRunDirector.h' in hud and "RoadRun->IsActive()" in hud and "RoadRun->GetObjectiveText()" in hud,
    "overspeed damages shipment": "SpeedKmh > SafeCruiseSpeedKmh" in director and "Overspeed * 0.0065f * DeltaSeconds" in director,
    "mechanical fleet state damages shipment": all(token in director for token in ("Assessment.ConditionPercent", "Assessment.TireIntegrity", "Assessment.BodyHealth", "MechanicalRisk")),
    "native impacts damage shipment": "GetNativeImpactCount()" in director and "NewImpacts * 0.085f" in director and "COURIER IMPACT" in director,
    "wanted blocks legal handoff while timer runs": "COURIER HANDOFF BLOCKED" in director and "Wanted->GetWantedLevel()" in director and "TimeRemaining = FMath::Max(0.0f, TimeRemaining - DeltaSeconds)" in director,
    "courier payout consumes integrity and bonuses": all(token in director for token in ("DamagePenalty", "FastDeliveryBonus", "CleanRunBonus", "BaseReward - DamagePenalty + FastBonus + CleanBonus")),
    "courier pays economy and saves": "Economy->AddCash(TotalReward" in director and "GameMode->SaveProgress()" in director,
    "playtest covers original ROAD loop": all(token in playtest for token in ("Village Parts Courier", "Paid ROAD preparation", "Speed, condition and crash risk", "Police consequence", "Win64/demo evidence")),
    "changelog records 0.1.2 milestone and demo honesty": "GTT 0.1.2" in changelog and "Traffic-Risk Economy" in changelog and "No demo Release is authorized" in changelog,
    "workflow runs RoadRun verifier": "Verify Rattleback road courier and traffic-risk economy" in workflow and "verify_road_run_contract.py" in workflow,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    raise SystemExit("Road courier verification failed: " + ", ".join(failed))

required_style = [
    '<!-- SWIR-ROADMAP-STANDARD:v1 -->', '<!-- ROADMAP-PROGRESS:START -->',
    'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"',
    '## 📊 Overall progress', '../assets/readme/progress-mini.svg', '<!-- ROADMAP-PROGRESS:END -->'
]
for token in required_style:
    if token not in roadmap:
        raise SystemExit("SWIR roadmap style lock missing: " + token)
items = re.findall(r'^- \[(x| )\] ', roadmap, flags=re.MULTILINE)
done = sum(v == 'x' for v in items)
total = len(items)
remaining = total - done
percent = round(done * 100.0 / total, 1) if total else 0.0
for token in (f'ROADMAP-{percent:.1f}%25', f'DONE-{done}%2F{total}', 'STATUS-IN%20PROGRESS', f'| **{done}** | **{remaining}** | **{total}** | **{percent:.1f}%** |'):
    if token not in roadmap:
        raise SystemExit("Roadmap dashboard drift: missing " + token)
if (done, total, percent) != (125, 130, 96.2):
    raise SystemExit(f"0.1.2 must not claim Unreal/Win64/art gates: {done}/{total} = {percent:.1f}%")
progress_block = roadmap.split('<!-- ROADMAP-PROGRESS:START -->', 1)[1].split('<!-- ROADMAP-PROGRESS:END -->', 1)[0]
if progress_block.count('../assets/readme/progress-mini.svg') != 1:
    raise SystemExit('Roadmap progress block must embed exactly one canonical progress-mini.svg.')
if re.search(r'[█▓▒░]{3,}', progress_block):
    raise SystemExit('Legacy text/Unicode progress meter must not return to the active Roadmap dashboard.')

print(f"[OK] GTT 0.1.2 ROAD courier guarantees retained under the expanded logistics route ({len(checks)} checks); roadmap {done}/{total} = {percent:.1f}% with SVG-only progress.")
