#!/usr/bin/env python3
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding="utf-8")

save_h = read("Source/GTT/Public/Save/GTTSaveGame.h")
logistics_h = read("Source/GTT/Public/World/GTTLogisticsReputationSubsystem.h")
logistics_cpp = read("Source/GTT/Private/World/GTTLogisticsReputationSubsystem.cpp")
rel_h = read("Source/GTT/Public/World/GTTDispatcherRelationshipSubsystem.h")
rel_cpp = read("Source/GTT/Private/World/GTTDispatcherRelationshipSubsystem.cpp")
dispatch_cpp = read("Source/GTT/Private/NPC/GTTLogisticsDispatcherPawn.cpp")
farm_cpp = read("Source/GTT/Private/Activities/GTTFarmJobDirector.cpp")
workflow = read(".github/workflows/project-sanity.yml")
changelog = read("CHANGELOG.d/0.1.9.md")
playtest = read("Docs/PLAYTEST_0.1.9.md")
roadmap = read("Docs/ROADMAP.md")

checks = {
    "reservation API is exposed": all(token in logistics_h for token in (
        "GetCargoReservationCount", "GetReservedCargoOrderTier", "GetCargoReservationSummary",
        "ReserveNegotiatedCargoOrder", "ExpireCargoReservations")),
    "queue is hard capped at two": all(token in logistics_cpp for token in (
        "MaxCargoReservations = 2", "FMath::Clamp(MaxQueue, 1, MaxCargoReservations)",
        "CargoReservedOrderTiers.Num() >= QueueLimit")),
    "reservation debits scarce stock immediately": all(token in logistics_cpp for token in (
        "FeedDepotStock -= RequiredStock", "CargoReservedOrderTiers.Add(CandidateTier)",
        "CargoReservedUnits.Add(RequiredStock)", "CargoReservationExpiryHours.Add(ExpiryHour)",
        "STOCK HELD")),
    "job start consumes held stock without second debit": all(token in logistics_cpp for token in (
        "CargoReservedOrderTiers[0] == RouteTier", "OutReservedUnits = CargoReservedUnits",
        "Consumed protected T%d dispatcher load", "without a second stock debit")),
    "expired hold returns stock and creates consequence": all(token in logistics_cpp for token in (
        "FeedDepotStock = FMath::Clamp(FeedDepotStock +", "CargoBacklogPressure = FMath::Clamp(CargoBacklogPressure + 1",
        "Missing pickup returns stock but adds backlog pressure")),
    "reservation owns next cargo order": all(token in logistics_cpp for token in (
        "CargoReservationDay == GetDayNumber()", "return CargoReservedOrderTiers[0]",
        "GetActiveCargoOrderTier()/ReserveCargoContract")),
    "queued order remains acceptable after free stock debit": all(token in logistics_cpp for token in (
        "A queued load already owns real stock", "ReservedTier >= 2 && WoodYardDemand <= 0",
        "return true;")),
    "reservation cannot bypass staffed desk": all(token in logistics_cpp for token in (
        "if (!IsCargoDepotWindowOpen())", "only written while the Feed Depot desk is staffed",
        "FMath::Min(CargoCloseHour, CurrentHour + HoldHours)")),
    "queue persistence stays additive v8": all(token in save_h for token in (
        "SaveVersion = 8", "0.1.9", "CargoReservationDay = 0",
        "CargoReservedOrderTiers", "CargoReservedUnits", "CargoReservationExpiryHours")),
    "queue capture and restore are complete": all(token in logistics_cpp for token in (
        "Save->CargoReservationDay = CargoReservationDay",
        "Save->CargoReservedOrderTiers = CargoReservedOrderTiers",
        "Save->CargoReservedUnits = CargoReservedUnits",
        "Save->CargoReservationExpiryHours = CargoReservationExpiryHours",
        "CargoReservedOrderTiers = Save->CargoReservedOrderTiers",
        "CargoReservationExpiryHours = Save->CargoReservationExpiryHours")),
    "restored queue is aligned bounded and clamped": all(token in logistics_cpp for token in (
        "const int32 ReservationCount = FMath::Min3", "FMath::Min(ReservationCount, MaxCargoReservations)",
        "FMath::Clamp(CargoReservedOrderTiers[Index], 1, 3)",
        "FMath::Clamp(CargoReservationExpiryHours[Index], CargoOpenHour, CargoCloseHour)")),
    "relationship favors are gameplay limits": all(token in rel_h for token in (
        "GetCargoReservationCapacity", "GetCargoReservationHoldMinutes", "GetCargoReservationFavorLabel")) and all(
        token in rel_cpp for token in (
            "return GetFeedDispatcherRelationship() >= 45 ? 2 : 1",
            "if (Relationship >= 70) return 110",
            "if (Relationship >= 45) return 80",
            "if (Relationship >= 25) return 50",
            "return 35")),
    "dispatcher turns negotiation into real reservation": all(token in dispatch_cpp for token in (
        "CycleCargoNegotiatedOrder", "ReserveNegotiatedCargoOrder",
        "GetCargoReservationHoldMinutes", "GetCargoReservationCapacity",
        "GetCargoReservationFavorLabel", "SaveProgress()")),
    "compact existing dispatcher hooks remain": all(token in dispatch_cpp for token in (
        "FEED DISPATCH NEGOTIATION", "E NEGOTIATE | DESK T%d",
        "HILL NEED %d | E STATUS", "WOOD NEED %d | E STATUS")),
    "trusted drivers visibly gain second slot": all(token in rel_cpp + dispatch_cpp for token in (
        "DOUBLE DESK", "Q%d/%d", "queue %d/%d")),
    "existing farm director still consumes authoritative active order": all(token in farm_cpp for token in (
        "RouteTierAtStart = Logistics->GetActiveCargoOrderTier()",
        "ReserveCargoContract(RouteTierAtStart")),
    "existing relationship access remains bounded": all(token in rel_cpp for token in (
        "Relationship >= 70 ? 3", "Relationship >= 35 ? 2 : 1",
        "FMath::Min(RelationshipTier", "Logistics->GetCargoRouteTier()")),
    "docs state honest demo boundary": all(token in changelog for token in (
        "GTT 0.1.9", "stock-backed", "SaveVersion remains 8", "No demo Release")) and all(
        token in playtest for token in (
            "Stock debit", "Queue persistence", "Expiry consequence", "Win64/demo acceptance boundary")),
    "workflow runs reservation verifier": (
        "Verify reserved contract queue and dispatcher favors" in workflow
        and "python Scripts/verify_reserved_contract_queue.py" in workflow),
}

checks["fresh negotiation tier cannot bypass dispatcher trust behind queued active tier"] = all(token in dispatch_cpp for token in (
    "GetNegotiatedCargoOrderTier()", "GetActiveCargoOrderTier() <= AccessTier", "NegotiatedTier <= AccessTier",
    "GetNegotiatedCargoOrderTier() > AccessTier"))

failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(f"[{'OK' if ok else 'FAIL'}] {name}")
if failed:
    raise SystemExit("Reserved contract queue verification failed: " + "; ".join(failed))

checkboxes = re.findall(r"^\s*-\s*\[([x ])\]", roadmap, flags=re.MULTILINE | re.IGNORECASE)
done = sum(1 for value in checkboxes if value.lower() == "x")
total = len(checkboxes)
remaining = total - done
progress = round(done * 100.0 / total, 1) if total else 0.0

required_style = (
    "<!-- SWIR-ROADMAP-STANDARD:v1 -->", "<!-- ROADMAP-PROGRESS:START -->", "<!-- ROADMAP-PROGRESS:END -->",
    'alt="CI"', 'alt="Roadmap progress"', 'alt="Completed"', 'alt="Status"',
    "## 📊 Overall progress", "../assets/readme/progress-mini.svg", f"ROADMAP-{progress:.1f}%25", f"DONE-{done}%2F{total}",
    "STATUS-IN%20PROGRESS", "| ✅ Completed | ⏳ Remaining | 📦 Total | 🎯 Progress |",
    f"| **{done}** | **{remaining}** | **{total}** | **{progress:.1f}%** |",
)
for token in required_style:
    if token not in roadmap:
        raise SystemExit("SWIR roadmap dashboard drift: missing " + token)
if (done, remaining, total, progress) != (125, 5, 130, 96.2):
    raise SystemExit(f"0.1.9 source milestone must not claim build/art gates: {done}/{total} = {progress:.1f}%")
progress_block = roadmap.split("<!-- ROADMAP-PROGRESS:START -->", 1)[1].split("<!-- ROADMAP-PROGRESS:END -->", 1)[0]
if progress_block.count("../assets/readme/progress-mini.svg") != 1:
    raise SystemExit("Roadmap progress block must embed exactly one canonical progress-mini.svg.")
if re.search(r"[█▓▒░]{3,}", progress_block):
    raise SystemExit("Legacy text/Unicode progress meter must not return to the active Roadmap dashboard.")

print(f"[OK] GTT 0.1.9 stock-backed reservation queue and dispatcher favors verified; roadmap {done}/{total} = {progress:.1f}% with SVG-only progress.")
