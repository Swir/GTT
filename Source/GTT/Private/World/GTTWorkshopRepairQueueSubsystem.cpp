#include "World/GTTWorkshopRepairQueueSubsystem.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GTTSaveGame.h"
#include "Save/GTTWorkshopQueueSaveGame.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTGarageServicePolicy.h"
#include "World/GTTServiceTerminal.h"
#include "World/GTTWorkshopHoursPolicy.h"
#include "GTT.h"

namespace
{
    const FString WorkshopQueueSlot(TEXT("GTT_WorkshopQueue_01"));
    const FString PrimaryWorldSlot(TEXT("GTT_Prototype_01"));
    constexpr int32 SaveUserIndex = 0;
    constexpr float QueueTickSeconds = 0.50f;
    constexpr float WorkshopParkingRadius = 900.0f;
    constexpr int32 DefaultWorkshopBaseCost = 75;

    bool NeedsMechanicalRepair(const AGTTRoadVehicleNativePawn* Vehicle)
    {
        return Vehicle && Vehicle->NeedsNativeWorkshopService();
    }

    const AGTTDayNightCycle* FindClock(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AGTTDayNightCycle> It(World); It; ++It)
        {
            if (IsValid(*It)) return *It;
        }
        return nullptr;
    }

    bool IsLaterSlot(int32 DayA, float HourA, int32 DayB, float HourB)
    {
        return DayA > DayB || (DayA == DayB && HourA > HourB + KINDA_SMALL_NUMBER);
    }

    bool IsAtOrAfter(int32 Day, float Hour, int32 TargetDay, float TargetHour)
    {
        return Day > TargetDay || (Day == TargetDay && Hour + KINDA_SMALL_NUMBER >= TargetHour);
    }
}

TStatId UGTTWorkshopRepairQueueSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTWorkshopRepairQueueSubsystem, STATGROUP_Tickables);
}

void UGTTWorkshopRepairQueueSubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;

    TickAccumulator += DeltaSeconds;
    NoticeCooldown = FMath::Max(0.0f, NoticeCooldown - DeltaSeconds);
    if (TickAccumulator < QueueTickSeconds) return;
    TickAccumulator = 0.0f;

    if (!bLoaded) LoadCheckpointOnce();
    if (QueueEntries.Num() > 0) TryExecuteReadyReservations();
}

bool UGTTWorkshopRepairQueueSubsystem::ResolveClock(int32& OutDay, float& OutHour) const
{
    if (const AGTTDayNightCycle* Clock = FindClock(GetWorld()))
    {
        OutDay = Clock->GetDayNumber();
        OutHour = GTTWorkshopHoursPolicy::NormalizeHour(Clock->GetTimeOfDayHours());
        return true;
    }
    return false;
}

bool UGTTWorkshopRepairQueueSubsystem::HasQueuedRepairForVehicle(FName VehicleId) const
{
    return !VehicleId.IsNone() && QueueEntries.ContainsByPredicate(
        [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Entry)
        {
            return Entry.PersistentVehicleId == VehicleId;
        });
}

bool UGTTWorkshopRepairQueueSubsystem::IsVehicleInWorkshopService(FName VehicleId) const
{
    return !VehicleId.IsNone() && QueueEntries.ContainsByPredicate(
        [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Entry)
        {
            return Entry.PersistentVehicleId == VehicleId && Entry.bCheckedIn && !Entry.bReadyForPickup;
        });
}

bool UGTTWorkshopRepairQueueSubsystem::IsVehicleAwaitingPickup(FName VehicleId) const
{
    return !VehicleId.IsNone() && QueueEntries.ContainsByPredicate(
        [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Entry)
        {
            return Entry.PersistentVehicleId == VehicleId && Entry.bReadyForPickup;
        });
}

FName UGTTWorkshopRepairQueueSubsystem::GetQueuedVehicleId() const
{
    return QueueEntries.Num() > 0 ? QueueEntries[0].PersistentVehicleId : NAME_None;
}

int32 UGTTWorkshopRepairQueueSubsystem::GetLockedQuote() const
{
    return QueueEntries.Num() > 0 ? QueueEntries[0].LockedQuote : 0;
}

int32 UGTTWorkshopRepairQueueSubsystem::CalculateUrgentQuote(int32 StandardQuote) const
{
    const int32 SafeQuote = FMath::Max(0, StandardQuote);
    if (SafeQuote <= 0) return 0;
    const int32 Surcharge = FMath::Max(1, FMath::CeilToInt(
        static_cast<float>(SafeQuote) * (static_cast<float>(UrgentQuoteSurchargePercent) / 100.0f)));
    return SafeQuote + Surcharge;
}

int32 UGTTWorkshopRepairQueueSubsystem::GetUrgentQuoteForVehicle(FName VehicleId) const
{
    if (VehicleId.IsNone()) return 0;
    const FGTTWorkshopRepairQueueSnapshot* Entry = QueueEntries.FindByPredicate(
        [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Candidate)
        {
            return Candidate.PersistentVehicleId == VehicleId;
        });
    if (!Entry) return 0;
    return Entry->bUrgent ? Entry->LockedQuote : CalculateUrgentQuote(Entry->LockedQuote);
}

void UGTTWorkshopRepairQueueSubsystem::SortQueueForServiceOrder()
{
    QueueEntries.Sort([](const FGTTWorkshopRepairQueueSnapshot& A, const FGTTWorkshopRepairQueueSnapshot& B)
    {
        if (A.ReadyDay != B.ReadyDay) return A.ReadyDay < B.ReadyDay;
        if (!FMath::IsNearlyEqual(A.ReadyHour, B.ReadyHour)) return A.ReadyHour < B.ReadyHour;
        if (A.bUrgent != B.bUrgent) return A.bUrgent;
        return A.PersistentVehicleId.LexicalLess(B.PersistentVehicleId);
    });
}

void UGTTWorkshopRepairQueueSubsystem::AdvanceUrgentAppointment(int32 UrgentIndex)
{
    if (!QueueEntries.IsValidIndex(UrgentIndex)) return;

    const FGTTWorkshopRepairQueueSnapshot& Target = QueueEntries[UrgentIndex];
    int32 EarliestStandard = INDEX_NONE;
    for (int32 Index = 0; Index < QueueEntries.Num(); ++Index)
    {
        if (Index == UrgentIndex) continue;
        const FGTTWorkshopRepairQueueSnapshot& Candidate = QueueEntries[Index];
        if (Candidate.bUrgent || Candidate.bCheckedIn || Candidate.bReadyForPickup) continue;
        if (!IsLaterSlot(Target.ReadyDay, Target.ReadyHour, Candidate.ReadyDay, Candidate.ReadyHour)) continue;

        if (EarliestStandard == INDEX_NONE || IsLaterSlot(
            QueueEntries[EarliestStandard].ReadyDay,
            QueueEntries[EarliestStandard].ReadyHour,
            Candidate.ReadyDay,
            Candidate.ReadyHour))
        {
            EarliestStandard = Index;
        }
    }

    if (EarliestStandard != INDEX_NONE)
    {
        Swap(QueueEntries[UrgentIndex].ReadyDay, QueueEntries[EarliestStandard].ReadyDay);
        Swap(QueueEntries[UrgentIndex].ReadyHour, QueueEntries[EarliestStandard].ReadyHour);
    }
    SortQueueForServiceOrder();
}

void UGTTWorkshopRepairQueueSubsystem::ResolveNextAppointment(
    int32 RequestDay, float RequestHour, int32& OutReadyDay, float& OutReadyHour) const
{
    GTTWorkshopHoursPolicy::ResolveNextOpening(RequestDay, RequestHour, OutReadyDay, OutReadyHour);

    for (const FGTTWorkshopRepairQueueSnapshot& Existing : QueueEntries)
    {
        if (!IsLaterSlot(Existing.ReadyDay, Existing.ReadyHour, OutReadyDay, OutReadyHour)) continue;
        OutReadyDay = Existing.ReadyDay;
        OutReadyHour = Existing.ReadyHour + AppointmentSpacingHours;
    }

    if (OutReadyHour + KINDA_SMALL_NUMBER >= GTTWorkshopHoursPolicy::ClosingHour)
    {
        ++OutReadyDay;
        OutReadyHour = GTTWorkshopHoursPolicy::OpeningHour;
    }
}

void UGTTWorkshopRepairQueueSubsystem::ResolveServiceCompletion(
    int32 StartDay, float StartHour, float DurationHours, int32& OutDay, float& OutHour) const
{
    const float AbsoluteHours = FMath::Max(0.0f, StartHour) + FMath::Max(0.10f, DurationHours);
    OutDay = FMath::Max(1, StartDay) + FMath::FloorToInt(AbsoluteHours / 24.0f);
    OutHour = FMath::Fmod(AbsoluteHours, 24.0f);
    if (OutHour < 0.0f) OutHour += 24.0f;
}

float UGTTWorkshopRepairQueueSubsystem::CalculateServiceDurationHours(
    const AGTTRoadVehicleNativePawn* Vehicle, bool bUrgent) const
{
    if (!Vehicle)
    {
        return bUrgent
            ? MinimumServiceDurationHours * UrgentServiceDurationMultiplier
            : MinimumServiceDurationHours;
    }

    const FGTTRoadVehicleMigrationSnapshot Migration = Vehicle->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot Body = Vehicle->GetBodyDamageSnapshot();
    const float ConditionDeficit = 1.0f - FMath::Clamp(Migration.ConditionPercent, 0.0f, 1.0f);
    const float TireDeficit = 1.0f - FMath::Clamp(Migration.TireIntegrity, 0.0f, 1.0f);
    const float FuelCapacity = FMath::Max(1.0f, Vehicle->GetFuelCapacityLiters());
    const float FuelDeficit = 1.0f - FMath::Clamp(Migration.FuelLiters / FuelCapacity, 0.0f, 1.0f);
    const float BodyHealth = FMath::Clamp(
        (Body.FrontHealth + Body.RearHealth + Body.LeftHealth + Body.RightHealth) * 0.25f, 0.0f, 1.0f);
    const float BodyDeficit = 1.0f - BodyHealth;
    const float DetachedPenalty = FMath::Clamp(static_cast<float>(Body.DetachedPanelCount) / 4.0f, 0.0f, 1.0f);
    const float Workload = ConditionDeficit * 0.35f + TireDeficit * 0.20f + FuelDeficit * 0.10f
        + BodyDeficit * 0.25f + DetachedPenalty * 0.10f;
    const float StandardDuration = FMath::Clamp(
        MinimumServiceDurationHours + Workload,
        MinimumServiceDurationHours,
        MaximumServiceDurationHours);
    if (!bUrgent) return StandardDuration;

    return FMath::Clamp(
        StandardDuration * UrgentServiceDurationMultiplier,
        MinimumServiceDurationHours * UrgentServiceDurationMultiplier,
        MaximumServiceDurationHours * UrgentServiceDurationMultiplier);
}

bool UGTTWorkshopRepairQueueSubsystem::TryQueueNearestEligibleNativeRoadVehicle(
    const FVector& Origin, float SearchRadius, FString& OutSummary)
{
    if (!bLoaded) LoadCheckpointOnce();

    int32 Day = 0;
    float Hour = 0.0f;
    if (!ResolveClock(Day, Hour))
    {
        OutSummary = TEXT("Workshop appointments unavailable: world clock is missing; minimal maps stay fail-open.");
        return false;
    }
    if (GTTWorkshopHoursPolicy::IsOpen(Hour))
    {
        OutSummary = TEXT("Workshop is open now; use normal workshop service instead of a deferred appointment.");
        return false;
    }
    if (QueueEntries.Num() >= MaxQueuedRepairs)
    {
        OutSummary = FString::Printf(
            TEXT("Workshop appointment book is full (%d/%d). Collect completed work, cancel a waiting exact reservation or wait for service."),
            QueueEntries.Num(), MaxQueuedRepairs);
        return false;
    }

    UWorld* World = GetWorld();
    if (!World) return false;

    AGTTRoadVehicleNativePawn* Best = nullptr;
    float BestDistanceSquared = FMath::Square(FMath::Max(100.0f, SearchRadius));
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Candidate = *It;
        if (!IsValid(Candidate) || !Candidate->IsLegacyTakeoverActive()) continue;
        const FGTTRoadVehicleMigrationSnapshot State = Candidate->GetMigrationSnapshot();
        const FName CandidateId = Candidate->GetPersistentVehicleId();
        if (!State.bOwnedByPlayer || CandidateId.IsNone()) continue;
        if (!NeedsMechanicalRepair(Candidate)) continue;
        if (HasQueuedRepairForVehicle(CandidateId)) continue;
        if (RequiresHardWorkshopHold(CandidateId)) continue;
        if (!IsCargoVehicleCompatible(CandidateId)) continue;
        if (IsVehicleImpounded(CandidateId)) continue;

        const float DistanceSquared = FVector::DistSquared2D(Origin, Candidate->GetActorLocation());
        if (DistanceSquared <= BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            Best = Candidate;
        }
    }

    if (!Best)
    {
        OutSummary = TEXT("No additional eligible owned damaged/mobile native road vehicle is close enough to reserve. Existing exact-ID appointments are skipped; hard WORKSHOP HOLD vehicles use emergency recovery instead.");
        return false;
    }

    const FName VehicleId = Best->GetPersistentVehicleId();
    UGTTBreakdownDecisionSubsystem* Decision = World->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    const int32 Quote = Decision
        ? Decision->CalculateRepairEstimate(Best, DefaultWorkshopBaseCost)
        : DefaultWorkshopBaseCost + Best->GetBodyDamageRepairSurcharge();
    if (Quote <= 0)
    {
        OutSummary = TEXT("Workshop appointment rejected an invalid repair quote.");
        return false;
    }

    int32 ReadyDay = Day;
    float ReadyHour = GTTWorkshopHoursPolicy::OpeningHour;
    ResolveNextAppointment(Day, Hour, ReadyDay, ReadyHour);

    FGTTWorkshopRepairQueueSnapshot Entry;
    Entry.bQueued = true;
    Entry.PersistentVehicleId = VehicleId;
    Entry.LockedQuote = Quote;
    Entry.RequestedDay = Day;
    Entry.RequestedHour = Hour;
    Entry.ReadyDay = ReadyDay;
    Entry.ReadyHour = ReadyHour;
    Entry.QueuePosition = QueueEntries.Num() + 1;
    Entry.Priority = TEXT("STANDARD");
    Entry.State = TEXT("QUEUED");
    QueueEntries.Add(Entry);

    if (!WriteCheckpoint())
    {
        QueueEntries.Pop();
        OutSummary = TEXT("Workshop appointment could not be saved; no charge or vehicle mutation occurred.");
        return false;
    }

    OutSummary = FString::Printf(
        TEXT("QUEUED %d/%d: %s STANDARD repair | locked quote $%d | appointment day %d %s | exact vehicle ID pinned | check-in starts timed service | no pre-charge."),
        QueueEntries.Num(), MaxQueuedRepairs, *VehicleId.ToString(), Quote, ReadyDay,
        *GTTWorkshopHoursPolicy::FormatHour(ReadyHour));
    UE_LOG(LogGTT, Display,
        TEXT("WORKSHOP_QUEUE_ACCEPTED vehicle=%s priority=STANDARD locked_quote=%d requested_day=%d requested_hour=%.2f ready_day=%d ready_hour=%.2f position=%d capacity=%d charged=NO exact_id=YES lifecycle=WAITING"),
        *VehicleId.ToString(), Quote, Day, Hour, ReadyDay, ReadyHour, QueueEntries.Num(), MaxQueuedRepairs);
    return true;
}

bool UGTTWorkshopRepairQueueSubsystem::CancelQueuedRepair(FName VehicleId, FString& OutSummary)
{
    if (!bLoaded) LoadCheckpointOnce();
    if (QueueEntries.Num() == 0)
    {
        OutSummary = TEXT("Workshop appointment book is empty.");
        return false;
    }
    if (VehicleId.IsNone())
    {
        OutSummary = TEXT("Workshop appointment cancellation requires an exact vehicle ID.");
        return false;
    }

    const int32 Index = QueueEntries.IndexOfByPredicate(
        [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Entry)
        {
            return Entry.PersistentVehicleId == VehicleId;
        });
    if (Index == INDEX_NONE)
    {
        OutSummary = FString::Printf(TEXT("No workshop appointment belongs to %s; exact-ID cancellation rejected."), *VehicleId.ToString());
        return false;
    }
    if (QueueEntries[Index].bCheckedIn || QueueEntries[Index].bReadyForPickup)
    {
        OutSummary = FString::Printf(
            TEXT("%s is already under workshop authority. IN_SERVICE, AWAITING_PAYMENT and READY_FOR_PICKUP work cannot be cancelled; finish or collect it instead."),
            *VehicleId.ToString());
        return false;
    }

    const FString Id = QueueEntries[Index].PersistentVehicleId.ToString();
    RemoveEntryAt(Index, TEXT("PLAYER_CANCELLED"));
    OutSummary = FString::Printf(
        TEXT("Workshop appointment for %s cancelled. No charge was taken; %d/%d appointment slots remain occupied."),
        *Id, QueueEntries.Num(), MaxQueuedRepairs);
    return true;
}

bool UGTTWorkshopRepairQueueSubsystem::PromoteQueuedRepairToUrgent(FName VehicleId, FString& OutSummary)
{
    if (!bLoaded) LoadCheckpointOnce();
    if (VehicleId.IsNone())
    {
        OutSummary = TEXT("Urgent workshop promotion requires an exact vehicle ID.");
        return false;
    }

    int32 Day = 0;
    float Hour = 0.0f;
    if (!ResolveClock(Day, Hour))
    {
        OutSummary = TEXT("Urgent workshop promotion unavailable: world clock is missing.");
        return false;
    }
    if (GTTWorkshopHoursPolicy::IsOpen(Hour))
    {
        OutSummary = TEXT("Urgent promotion closes when the workshop opens; check in the exact vehicle instead.");
        return false;
    }

    const int32 Index = QueueEntries.IndexOfByPredicate(
        [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Entry)
        {
            return Entry.PersistentVehicleId == VehicleId;
        });
    if (Index == INDEX_NONE)
    {
        OutSummary = TEXT("No exact workshop appointment is available for urgent promotion.");
        return false;
    }
    if (QueueEntries[Index].bUrgent)
    {
        OutSummary = FString::Printf(TEXT("%s is already URGENT at locked quote $%d."),
            *VehicleId.ToString(), QueueEntries[Index].LockedQuote);
        return false;
    }
    if (QueueEntries[Index].bCheckedIn || QueueEntries[Index].bReadyForPickup)
    {
        OutSummary = TEXT("Priority cannot change after workshop check-in.");
        return false;
    }

    const TArray<FGTTWorkshopRepairQueueSnapshot> Previous = QueueEntries;
    const int32 StandardQuote = QueueEntries[Index].LockedQuote;
    const int32 UrgentQuote = CalculateUrgentQuote(StandardQuote);
    if (UrgentQuote <= StandardQuote)
    {
        OutSummary = TEXT("Urgent workshop promotion rejected an invalid premium quote.");
        return false;
    }

    QueueEntries[Index].bUrgent = true;
    QueueEntries[Index].Priority = TEXT("URGENT");
    QueueEntries[Index].LockedQuote = UrgentQuote;
    AdvanceUrgentAppointment(Index);

    if (!WriteCheckpoint())
    {
        QueueEntries = Previous;
        OutSummary = TEXT("Urgent workshop promotion could not be persisted; STANDARD appointment and quote were restored with no charge.");
        return false;
    }

    const FGTTWorkshopRepairQueueSnapshot* Promoted = QueueEntries.FindByPredicate(
        [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Entry)
        {
            return Entry.PersistentVehicleId == VehicleId;
        });
    const int32 ReadyDay = Promoted ? Promoted->ReadyDay : Day;
    const float ReadyHour = Promoted ? Promoted->ReadyHour : GTTWorkshopHoursPolicy::OpeningHour;
    OutSummary = FString::Printf(
        TEXT("URGENT CONFIRMED: %s | STANDARD $%d -> locked URGENT $%d (+%d%%) | priority slot day %d %s | service timer x%.2f | no pre-charge."),
        *VehicleId.ToString(), StandardQuote, UrgentQuote, UrgentQuoteSurchargePercent,
        ReadyDay, *GTTWorkshopHoursPolicy::FormatHour(ReadyHour), UrgentServiceDurationMultiplier);
    UE_LOG(LogGTT, Display,
        TEXT("WORKSHOP_QUEUE_PRIORITY_UPGRADED vehicle=%s standard_quote=%d urgent_locked_quote=%d surcharge_percent=%d ready_day=%d ready_hour=%.2f service_multiplier=%.2f charged=NO exact_id=YES"),
        *VehicleId.ToString(), StandardQuote, UrgentQuote, UrgentQuoteSurchargePercent,
        ReadyDay, ReadyHour, UrgentServiceDurationMultiplier);
    return true;
}

bool UGTTWorkshopRepairQueueSubsystem::ReleaseCompletedRepairForPickup(FName VehicleId, FString& OutSummary)
{
    if (!bLoaded) LoadCheckpointOnce();
    if (VehicleId.IsNone())
    {
        OutSummary = TEXT("Workshop pickup requires an exact vehicle ID.");
        return false;
    }

    const int32 Index = QueueEntries.IndexOfByPredicate(
        [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Entry)
        {
            return Entry.PersistentVehicleId == VehicleId;
        });
    if (Index == INDEX_NONE)
    {
        OutSummary = TEXT("No completed workshop job belongs to that exact vehicle ID.");
        return false;
    }

    const FGTTWorkshopRepairQueueSnapshot Entry = QueueEntries[Index];
    if (!Entry.bReadyForPickup || Entry.PaidAmount <= 0 || Entry.PaidAmount != Entry.LockedQuote)
    {
        OutSummary = FString::Printf(
            TEXT("%s is not READY_FOR_PICKUP; payment and repair must complete before fleet release."),
            *VehicleId.ToString());
        return false;
    }

    bool bAmbiguous = false;
    AGTTRoadVehicleNativePawn* Vehicle = FindExactQueuedVehicle(VehicleId, bAmbiguous);
    if (bAmbiguous || !Vehicle || !Vehicle->IsLegacyTakeoverActive()
        || !Vehicle->GetMigrationSnapshot().bOwnedByPlayer || !IsVehicleAtWorkshop(Vehicle))
    {
        OutSummary = FString::Printf(
            TEXT("Pickup for %s requires the one exact owned repaired vehicle to remain at the workshop; no queue state changed."),
            *VehicleId.ToString());
        return false;
    }

    const TArray<FGTTWorkshopRepairQueueSnapshot> Previous = QueueEntries;
    QueueEntries.RemoveAt(Index);
    if (!WriteCheckpoint())
    {
        QueueEntries = Previous;
        WriteCheckpoint();
        OutSummary = TEXT("Workshop pickup could not persist fleet release; completed job remains READY_FOR_PICKUP.");
        return false;
    }

    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
        GameMode->SaveProgress();

    OutSummary = FString::Printf(
        TEXT("PICKUP COMPLETE: %s returned to garage/fleet dispatch | paid $%d already | repair/refuel complete | %d/%d workshop slots remain occupied."),
        *VehicleId.ToString(), Entry.PaidAmount, QueueEntries.Num(), MaxQueuedRepairs);
    UE_LOG(LogGTT, Display,
        TEXT("WORKSHOP_QUEUE_PICKUP_RELEASED vehicle=%s paid_amount=%d exact_id=YES repair_complete=YES fleet_return=YES remaining=%d capacity=%d"),
        *VehicleId.ToString(), Entry.PaidAmount, QueueEntries.Num(), MaxQueuedRepairs);
    return true;
}

FGTTWorkshopRepairQueueSnapshot UGTTWorkshopRepairQueueSubsystem::BuildSnapshot(
    const FGTTWorkshopRepairQueueSnapshot& Entry, int32 Position) const
{
    FGTTWorkshopRepairQueueSnapshot Snapshot = Entry;
    Snapshot.bQueued = true;
    Snapshot.QueuePosition = Position;
    Snapshot.Priority = Entry.bUrgent ? TEXT("URGENT") : TEXT("STANDARD");
    Snapshot.State = TEXT("QUEUED");
    Snapshot.HoursUntilReady = 0.0f;
    Snapshot.HoursUntilServiceComplete = 0.0f;

    if (Entry.bReadyForPickup)
    {
        Snapshot.State = TEXT("READY_FOR_PICKUP");
        return Snapshot;
    }

    int32 Day = 0;
    float Hour = 0.0f;
    if (ResolveClock(Day, Hour))
    {
        Snapshot.HoursUntilReady = FMath::Max(0.0f,
            static_cast<float>(Entry.ReadyDay - Day) * 24.0f + Entry.ReadyHour - Hour);
        if (Entry.bCheckedIn)
        {
            Snapshot.HoursUntilServiceComplete = FMath::Max(0.0f,
                static_cast<float>(Entry.ServiceCompleteDay - Day) * 24.0f + Entry.ServiceCompleteHour - Hour);
            Snapshot.State = IsAtOrAfter(Day, Hour, Entry.ServiceCompleteDay, Entry.ServiceCompleteHour)
                ? TEXT("AWAITING_PAYMENT") : TEXT("IN_SERVICE");
            Snapshot.HoursUntilReady = 0.0f;
        }
        else if (IsAtOrAfter(Day, Hour, Entry.ReadyDay, Entry.ReadyHour))
        {
            Snapshot.State = TEXT("READY");
            Snapshot.HoursUntilReady = 0.0f;
        }
    }
    return Snapshot;
}

FGTTWorkshopRepairQueueSnapshot UGTTWorkshopRepairQueueSubsystem::GetQueueSnapshot() const
{
    if (QueueEntries.Num() == 0) return FGTTWorkshopRepairQueueSnapshot();
    return BuildSnapshot(QueueEntries[0], 1);
}

TArray<FGTTWorkshopRepairQueueSnapshot> UGTTWorkshopRepairQueueSubsystem::GetQueueSnapshots() const
{
    TArray<FGTTWorkshopRepairQueueSnapshot> Result;
    Result.Reserve(QueueEntries.Num());
    for (int32 Index = 0; Index < QueueEntries.Num(); ++Index)
    {
        Result.Add(BuildSnapshot(QueueEntries[Index], Index + 1));
    }
    return Result;
}

FText UGTTWorkshopRepairQueueSubsystem::GetQueueStatusText() const
{
    if (QueueEntries.Num() == 0) return FText::FromString(TEXT("Workshop appointments: EMPTY"));
    const FGTTWorkshopRepairQueueSnapshot Next = BuildSnapshot(QueueEntries[0], 1);
    if (Next.bReadyForPickup)
    {
        return FText::FromString(FString::Printf(
            TEXT("Workshop appointments: %d/%d | next READY_FOR_PICKUP #%d %s | %s | paid $%d | collect at workshop job board to return it to fleet"),
            QueueEntries.Num(), MaxQueuedRepairs, Next.QueuePosition, *Next.PersistentVehicleId.ToString(),
            *Next.Priority, Next.PaidAmount));
    }
    if (Next.bCheckedIn)
    {
        return FText::FromString(FString::Printf(
            TEXT("Workshop appointments: %d/%d | next %s #%d %s | %s | locked $%d | service ETA %.1f h | checkout only after service | no pre-charge"),
            QueueEntries.Num(), MaxQueuedRepairs, *Next.State, Next.QueuePosition,
            *Next.PersistentVehicleId.ToString(), *Next.Priority, Next.LockedQuote, Next.HoursUntilServiceComplete));
    }
    return FText::FromString(FString::Printf(
        TEXT("Workshop appointments: %d/%d | next %s #%d %s | %s | locked $%d | day %d %s | ETA %.1f h | no pre-charge"),
        QueueEntries.Num(), MaxQueuedRepairs, *Next.State, Next.QueuePosition,
        *Next.PersistentVehicleId.ToString(), *Next.Priority, Next.LockedQuote, Next.ReadyDay,
        *GTTWorkshopHoursPolicy::FormatHour(Next.ReadyHour), Next.HoursUntilReady));
}

void UGTTWorkshopRepairQueueSubsystem::LoadCheckpointOnce()
{
    bLoaded = true;
    QueueEntries.Reset();

    UGTTWorkshopQueueSaveGame* Save = Cast<UGTTWorkshopQueueSaveGame>(
        UGameplayStatics::LoadGameFromSlot(WorkshopQueueSlot, SaveUserIndex));
    if (!Save) return;
    if (Save->SchemaVersion != 1)
    {
        ClearCheckpoint(TEXT("INVALID_SCHEMA"));
        return;
    }

    TArray<FGTTWorkshopQueueSaveEntry> SavedEntries = Save->Appointments;
    bool bMigratedLegacy = false;
    if (SavedEntries.Num() == 0 && Save->bQueued)
    {
        FGTTWorkshopQueueSaveEntry Legacy;
        Legacy.PersistentVehicleId = Save->PersistentVehicleId;
        Legacy.LockedQuote = Save->LockedQuote;
        Legacy.RequestedDay = Save->RequestedDay;
        Legacy.RequestedHour = Save->RequestedHour;
        Legacy.ReadyDay = Save->ReadyDay;
        Legacy.ReadyHour = Save->ReadyHour;
        SavedEntries.Add(Legacy);
        bMigratedLegacy = true;
    }

    bool bNeedsRewrite = bMigratedLegacy || SavedEntries.Num() > MaxQueuedRepairs;
    TSet<FName> SeenIds;
    for (const FGTTWorkshopQueueSaveEntry& Saved : SavedEntries)
    {
        if (QueueEntries.Num() >= MaxQueuedRepairs) break;
        const bool bBasicValid = !Saved.PersistentVehicleId.IsNone() && Saved.LockedQuote > 0
            && Saved.RequestedDay > 0 && Saved.ReadyDay > 0
            && Saved.RequestedHour >= 0.0f && Saved.RequestedHour < 24.0f
            && Saved.ReadyHour >= 0.0f && Saved.ReadyHour < 24.0f;
        if (!bBasicValid || SeenIds.Contains(Saved.PersistentVehicleId)
            || !IsCargoVehicleCompatible(Saved.PersistentVehicleId)
            || IsVehicleImpounded(Saved.PersistentVehicleId))
        {
            bNeedsRewrite = true;
            continue;
        }

        SeenIds.Add(Saved.PersistentVehicleId);
        FGTTWorkshopRepairQueueSnapshot Entry;
        Entry.bQueued = true;
        Entry.PersistentVehicleId = Saved.PersistentVehicleId;
        Entry.LockedQuote = Saved.LockedQuote;
        Entry.RequestedDay = Saved.RequestedDay;
        Entry.RequestedHour = Saved.RequestedHour;
        Entry.ReadyDay = Saved.ReadyDay;
        Entry.ReadyHour = Saved.ReadyHour;
        Entry.bUrgent = Saved.bUrgent;
        Entry.Priority = Saved.bUrgent ? TEXT("URGENT") : TEXT("STANDARD");
        Entry.State = TEXT("QUEUED");

        const bool bLifecycleValid = Saved.bCheckedIn
            && Saved.ServiceStartDay > 0 && Saved.ServiceCompleteDay > 0
            && Saved.ServiceStartHour >= 0.0f && Saved.ServiceStartHour < 24.0f
            && Saved.ServiceCompleteHour >= 0.0f && Saved.ServiceCompleteHour < 24.0f
            && IsAtOrAfter(Saved.ServiceCompleteDay, Saved.ServiceCompleteHour,
                Saved.ServiceStartDay, Saved.ServiceStartHour);
        if (Saved.bCheckedIn && !bLifecycleValid) bNeedsRewrite = true;
        if (bLifecycleValid)
        {
            Entry.bCheckedIn = true;
            Entry.ServiceStartDay = Saved.ServiceStartDay;
            Entry.ServiceStartHour = Saved.ServiceStartHour;
            Entry.ServiceCompleteDay = Saved.ServiceCompleteDay;
            Entry.ServiceCompleteHour = Saved.ServiceCompleteHour;
        }

        const bool bPickupValid = Saved.bReadyForPickup && bLifecycleValid
            && Saved.PaidAmount == Saved.LockedQuote && Saved.PaidAmount > 0
            && Saved.PaidDay > 0 && Saved.PaidHour >= 0.0f && Saved.PaidHour < 24.0f;
        if (Saved.bReadyForPickup && !bPickupValid) bNeedsRewrite = true;
        if (bPickupValid)
        {
            Entry.bReadyForPickup = true;
            Entry.PaidAmount = Saved.PaidAmount;
            Entry.PaidDay = Saved.PaidDay;
            Entry.PaidHour = Saved.PaidHour;
        }
        QueueEntries.Add(Entry);
    }

    SortQueueForServiceOrder();

    if (QueueEntries.Num() == 0)
    {
        ClearCheckpoint(TEXT("NO_VALID_APPOINTMENTS"));
        return;
    }

    const int32 CheckedInCount = QueueEntries.CountByPredicate(
        [](const FGTTWorkshopRepairQueueSnapshot& Entry) { return Entry.bCheckedIn && !Entry.bReadyForPickup; });
    const int32 UrgentCount = QueueEntries.CountByPredicate(
        [](const FGTTWorkshopRepairQueueSnapshot& Entry) { return Entry.bUrgent; });
    const int32 PickupCount = QueueEntries.CountByPredicate(
        [](const FGTTWorkshopRepairQueueSnapshot& Entry) { return Entry.bReadyForPickup; });
    UE_LOG(LogGTT, Display,
        TEXT("WORKSHOP_QUEUE_RESTORED count=%d capacity=%d checked_in=%d urgent=%d pickup=%d first_vehicle=%s first_locked_quote=%d exact_id=YES legacy_migrated=%s"),
        QueueEntries.Num(), MaxQueuedRepairs, CheckedInCount, UrgentCount, PickupCount,
        *QueueEntries[0].PersistentVehicleId.ToString(), QueueEntries[0].LockedQuote,
        bMigratedLegacy ? TEXT("YES") : TEXT("NO"));
    if (bNeedsRewrite) WriteCheckpoint();
}

bool UGTTWorkshopRepairQueueSubsystem::WriteCheckpoint()
{
    if (QueueEntries.Num() == 0)
    {
        if (UGameplayStatics::DoesSaveGameExist(WorkshopQueueSlot, SaveUserIndex))
            UGameplayStatics::DeleteGameInSlot(WorkshopQueueSlot, SaveUserIndex);
        return true;
    }

    UGTTWorkshopQueueSaveGame* Save = Cast<UGTTWorkshopQueueSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UGTTWorkshopQueueSaveGame::StaticClass()));
    if (!Save) return false;

    Save->Appointments.Reset();
    for (const FGTTWorkshopRepairQueueSnapshot& Entry : QueueEntries)
    {
        FGTTWorkshopQueueSaveEntry Saved;
        Saved.PersistentVehicleId = Entry.PersistentVehicleId;
        Saved.LockedQuote = Entry.LockedQuote;
        Saved.RequestedDay = Entry.RequestedDay;
        Saved.RequestedHour = Entry.RequestedHour;
        Saved.ReadyDay = Entry.ReadyDay;
        Saved.ReadyHour = Entry.ReadyHour;
        Saved.bUrgent = Entry.bUrgent;
        Saved.bCheckedIn = Entry.bCheckedIn;
        Saved.ServiceStartDay = Entry.ServiceStartDay;
        Saved.ServiceStartHour = Entry.ServiceStartHour;
        Saved.ServiceCompleteDay = Entry.ServiceCompleteDay;
        Saved.ServiceCompleteHour = Entry.ServiceCompleteHour;
        Saved.bReadyForPickup = Entry.bReadyForPickup;
        Saved.PaidAmount = Entry.PaidAmount;
        Saved.PaidDay = Entry.PaidDay;
        Saved.PaidHour = Entry.PaidHour;
        Save->Appointments.Add(Saved);
    }

    // Keep the original schema-v1 first-entry mirror valid for legacy saves/evidence consumers.
    const FGTTWorkshopRepairQueueSnapshot& First = QueueEntries[0];
    Save->bQueued = true;
    Save->PersistentVehicleId = First.PersistentVehicleId;
    Save->LockedQuote = First.LockedQuote;
    Save->RequestedDay = First.RequestedDay;
    Save->RequestedHour = First.RequestedHour;
    Save->ReadyDay = First.ReadyDay;
    Save->ReadyHour = First.ReadyHour;
    return UGameplayStatics::SaveGameToSlot(Save, WorkshopQueueSlot, SaveUserIndex);
}

void UGTTWorkshopRepairQueueSubsystem::ClearCheckpoint(const TCHAR* Reason)
{
    if (UGameplayStatics::DoesSaveGameExist(WorkshopQueueSlot, SaveUserIndex))
        UGameplayStatics::DeleteGameInSlot(WorkshopQueueSlot, SaveUserIndex);
    QueueEntries.Reset();
    UE_LOG(LogGTT, VeryVerbose, TEXT("WORKSHOP_QUEUE_CLEARED reason=%s charged=NO"), Reason ? Reason : TEXT("UNKNOWN"));
}

void UGTTWorkshopRepairQueueSubsystem::RemoveEntryAt(int32 Index, const TCHAR* Reason)
{
    if (!QueueEntries.IsValidIndex(Index)) return;
    const FString VehicleId = QueueEntries[Index].PersistentVehicleId.ToString();
    QueueEntries.RemoveAt(Index);
    if (!WriteCheckpoint())
    {
        UE_LOG(LogGTT, Error, TEXT("WORKSHOP_QUEUE_PERSIST_FAILED after_remove=%s reason=%s"),
            *VehicleId, Reason ? Reason : TEXT("UNKNOWN"));
    }
    UE_LOG(LogGTT, VeryVerbose, TEXT("WORKSHOP_QUEUE_ENTRY_REMOVED vehicle=%s reason=%s remaining=%d charged=NO"),
        *VehicleId, Reason ? Reason : TEXT("UNKNOWN"), QueueEntries.Num());
}

bool UGTTWorkshopRepairQueueSubsystem::IsCargoVehicleCompatible(FName VehicleId) const
{
    const UGTTSaveGame* Primary = Cast<UGTTSaveGame>(
        UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex));
    if (!Primary || !Primary->bFarmCargoContractActive || Primary->FarmCargoBoundVehicleId.IsNone()) return true;
    return Primary->FarmCargoBoundVehicleId == VehicleId;
}

bool UGTTWorkshopRepairQueueSubsystem::IsVehicleImpounded(FName VehicleId) const
{
    const UGTTSaveGame* Primary = Cast<UGTTSaveGame>(
        UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex));
    return Primary && !Primary->ImpoundedVehicleId.IsNone() && Primary->ImpoundedVehicleId == VehicleId;
}

bool UGTTWorkshopRepairQueueSubsystem::RequiresHardWorkshopHold(FName VehicleId) const
{
    UWorld* World = GetWorld();
    const UGTTGarageFleetSubsystem* Fleet = World ? World->GetSubsystem<UGTTGarageFleetSubsystem>() : nullptr;
    if (!Fleet || VehicleId.IsNone()) return false;
    const TArray<FGTTGarageFleetSnapshot> Vehicles = Fleet->BuildFleetSnapshot(8);
    if (const FGTTGarageFleetSnapshot* Snapshot = Vehicles.FindByPredicate(
        [VehicleId](const FGTTGarageFleetSnapshot& Candidate) { return Candidate.VehicleId == VehicleId; }))
    {
        return GTTGarageServicePolicy::RequiresWorkshopBeforeDispatch(*Snapshot);
    }
    return false;
}

AGTTRoadVehicleNativePawn* UGTTWorkshopRepairQueueSubsystem::FindExactQueuedVehicle(FName VehicleId, bool& bAmbiguous) const
{
    bAmbiguous = false;
    UWorld* World = GetWorld();
    if (!World || VehicleId.IsNone()) return nullptr;

    AGTTRoadVehicleNativePawn* Found = nullptr;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Candidate = *It;
        if (!IsValid(Candidate) || Candidate->GetPersistentVehicleId() != VehicleId) continue;
        if (Found)
        {
            bAmbiguous = true;
            return nullptr;
        }
        Found = Candidate;
    }
    return Found;
}

bool UGTTWorkshopRepairQueueSubsystem::IsVehicleAtWorkshop(const AGTTRoadVehicleNativePawn* Vehicle) const
{
    UWorld* World = GetWorld();
    if (!World || !Vehicle) return false;

    for (TActorIterator<AGTTServiceTerminal> It(World); It; ++It)
    {
        const AGTTServiceTerminal* Terminal = *It;
        if (!IsValid(Terminal) || Terminal->GetServiceType() != EGTTServiceType::Workshop) continue;
        if (FVector::DistSquared(Vehicle->GetActorLocation(), Terminal->GetActorLocation()) <= FMath::Square(WorkshopParkingRadius))
            return true;
    }
    return false;
}

void UGTTWorkshopRepairQueueSubsystem::TryExecuteReadyReservations()
{
    int32 Day = 0;
    float Hour = 0.0f;
    if (!ResolveClock(Day, Hour) || !GTTWorkshopHoursPolicy::IsOpen(Hour)) return;

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    UGTTPlayerEconomyComponent* Economy = PlayerPawn
        ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn)
        : nullptr;
    if (!Economy) return;

    int32 Index = 0;
    while (Index < QueueEntries.Num())
    {
        FGTTWorkshopRepairQueueSnapshot Entry = QueueEntries[Index];
        if (Entry.bReadyForPickup)
        {
            ++Index;
            continue;
        }

        const bool bDue = IsAtOrAfter(Day, Hour, Entry.ReadyDay, Entry.ReadyHour);
        if (!bDue)
        {
            ++Index;
            continue;
        }

        if (!IsCargoVehicleCompatible(Entry.PersistentVehicleId) || IsVehicleImpounded(Entry.PersistentVehicleId))
        {
            RemoveEntryAt(Index, TEXT("AUTHORITY_CONFLICT_AT_EXECUTION"));
            continue;
        }
        if (RequiresHardWorkshopHold(Entry.PersistentVehicleId))
        {
            RemoveEntryAt(Index, TEXT("HARD_HOLD_TAKES_PRIORITY"));
            continue;
        }

        bool bAmbiguous = false;
        AGTTRoadVehicleNativePawn* Vehicle = FindExactQueuedVehicle(Entry.PersistentVehicleId, bAmbiguous);
        if (bAmbiguous)
        {
            RemoveEntryAt(Index, TEXT("AMBIGUOUS_PERSISTENT_ID"));
            continue;
        }
        if (!Vehicle || !Vehicle->IsLegacyTakeoverActive() || !Vehicle->GetMigrationSnapshot().bOwnedByPlayer)
        {
            ++Index;
            continue;
        }
        if (!NeedsMechanicalRepair(Vehicle))
        {
            RemoveEntryAt(Index, TEXT("NO_LONGER_NEEDS_REPAIR"));
            continue;
        }

        if (Entry.bCheckedIn && !IsVehicleAtWorkshop(Vehicle))
        {
            QueueEntries[Index].bCheckedIn = false;
            QueueEntries[Index].ServiceStartDay = 0;
            QueueEntries[Index].ServiceStartHour = 0.0f;
            QueueEntries[Index].ServiceCompleteDay = 0;
            QueueEntries[Index].ServiceCompleteHour = 0.0f;
            WriteCheckpoint();
            if (NoticeCooldown <= 0.0f)
            {
                Economy->PushMessage(FString::Printf(
                    TEXT("Workshop service paused for %s: vehicle left the service area before completion. Appointment remains READY with the same %s locked quote and no charge."),
                    *Entry.PersistentVehicleId.ToString(), Entry.bUrgent ? TEXT("URGENT") : TEXT("STANDARD")), 7.0f);
                NoticeCooldown = 8.0f;
            }
            UE_LOG(LogGTT, Display,
                TEXT("WORKSHOP_QUEUE_SERVICE_PAUSED vehicle=%s priority=%s reason=LEFT_SERVICE_AREA locked_quote=%d charged=NO appointment_preserved=YES"),
                *Entry.PersistentVehicleId.ToString(), Entry.bUrgent ? TEXT("URGENT") : TEXT("STANDARD"), Entry.LockedQuote);
            ++Index;
            continue;
        }

        if (!Entry.bCheckedIn)
        {
            if (!IsVehicleAtWorkshop(Vehicle))
            {
                ++Index;
                continue;
            }

            const float DurationHours = CalculateServiceDurationHours(Vehicle, Entry.bUrgent);
            FGTTWorkshopRepairQueueSnapshot& MutableEntry = QueueEntries[Index];
            MutableEntry.bCheckedIn = true;
            MutableEntry.ServiceStartDay = Day;
            MutableEntry.ServiceStartHour = Hour;
            ResolveServiceCompletion(Day, Hour, DurationHours,
                MutableEntry.ServiceCompleteDay, MutableEntry.ServiceCompleteHour);
            if (!WriteCheckpoint())
            {
                MutableEntry.bCheckedIn = false;
                MutableEntry.ServiceStartDay = 0;
                MutableEntry.ServiceStartHour = 0.0f;
                MutableEntry.ServiceCompleteDay = 0;
                MutableEntry.ServiceCompleteHour = 0.0f;
                UE_LOG(LogGTT, Error,
                    TEXT("WORKSHOP_QUEUE_CHECKIN_FAILED vehicle=%s priority=%s reason=PERSISTENCE charged=NO mutation=NO"),
                    *Entry.PersistentVehicleId.ToString(), Entry.bUrgent ? TEXT("URGENT") : TEXT("STANDARD"));
                ++Index;
                continue;
            }

            Economy->PushMessage(FString::Printf(
                TEXT("Workshop check-in: %s | %s locked quote $%d | service time %.1f h | checkout day %d %s | no charge until completion."),
                *Entry.PersistentVehicleId.ToString(), Entry.bUrgent ? TEXT("URGENT") : TEXT("STANDARD"),
                Entry.LockedQuote, DurationHours, MutableEntry.ServiceCompleteDay,
                *GTTWorkshopHoursPolicy::FormatHour(MutableEntry.ServiceCompleteHour)), 8.0f);
            UE_LOG(LogGTT, Display,
                TEXT("WORKSHOP_QUEUE_CHECKED_IN vehicle=%s priority=%s locked_quote=%d duration_hours=%.2f start_day=%d start_hour=%.2f complete_day=%d complete_hour=%.2f charged=NO mutation=NO exact_id=YES"),
                *Entry.PersistentVehicleId.ToString(), Entry.bUrgent ? TEXT("URGENT") : TEXT("STANDARD"),
                Entry.LockedQuote, DurationHours, MutableEntry.ServiceStartDay, MutableEntry.ServiceStartHour,
                MutableEntry.ServiceCompleteDay, MutableEntry.ServiceCompleteHour);
            ++Index;
            continue;
        }

        if (!IsAtOrAfter(Day, Hour, Entry.ServiceCompleteDay, Entry.ServiceCompleteHour))
        {
            ++Index;
            continue;
        }

        const int32 LockedQuote = Entry.LockedQuote;
        if (!Economy->SpendCash(LockedQuote, FString::Printf(
            TEXT("%s queued workshop repair %s - $%d"),
            Entry.bUrgent ? TEXT("Urgent") : TEXT("Standard"),
            *Entry.PersistentVehicleId.ToString(), LockedQuote)))
        {
            if (NoticeCooldown <= 0.0f)
            {
                Economy->PushMessage(FString::Printf(
                    TEXT("Workshop service finished for %s but checkout needs $%d. It stays AWAITING PAYMENT; later due appointments can still proceed."),
                    *Entry.PersistentVehicleId.ToString(), LockedQuote), 6.0f);
                NoticeCooldown = 8.0f;
            }
            ++Index;
            continue;
        }

        if (!Vehicle->ApplyNativeWorkshopService())
        {
            Economy->AddCash(LockedQuote, TEXT("Queued workshop repair rollback"));
            Economy->PushMessage(TEXT("Queued workshop repair could not be applied; payment returned and only that appointment was cleared."), 6.0f);
            RemoveEntryAt(Index, TEXT("SERVICE_MUTATION_FAILED"));
            continue;
        }

        FGTTWorkshopRepairQueueSnapshot& MutableEntry = QueueEntries[Index];
        MutableEntry.bReadyForPickup = true;
        MutableEntry.PaidAmount = LockedQuote;
        MutableEntry.PaidDay = Day;
        MutableEntry.PaidHour = Hour;
        MutableEntry.State = TEXT("READY_FOR_PICKUP");

        if (!WriteCheckpoint())
        {
            const FString VehicleIdText = Entry.PersistentVehicleId.ToString();
            QueueEntries.RemoveAt(Index);
            WriteCheckpoint();
            if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
                GameMode->SaveProgress();
            Economy->PushMessage(FString::Printf(
                TEXT("Workshop checkout completed for %s, but pickup checkpoint persistence failed. Vehicle was released to fleet automatically rather than stranded; charged $%d once."),
                *VehicleIdText, LockedQuote), 9.0f);
            UE_LOG(LogGTT, Error,
                TEXT("WORKSHOP_QUEUE_PICKUP_CHECKPOINT_FAILED vehicle=%s charged=%d repair_complete=YES auto_release=YES"),
                *VehicleIdText, LockedQuote);
            continue;
        }

        if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
            GameMode->SaveProgress();

        Economy->PushMessage(FString::Printf(
            TEXT("Workshop service complete: %s | %s | charged locked quote $%d exactly once | repair/refuel complete | READY FOR PICKUP at the job board."),
            *Entry.PersistentVehicleId.ToString(), Entry.bUrgent ? TEXT("URGENT") : TEXT("STANDARD"), LockedQuote), 9.0f);
        UE_LOG(LogGTT, Display,
            TEXT("WORKSHOP_QUEUE_READY_FOR_PICKUP vehicle=%s priority=%s charged=%d locked_quote_match=YES exact_id=YES timed_service=YES saved=YES fleet_release=PENDING"),
            *Entry.PersistentVehicleId.ToString(), Entry.bUrgent ? TEXT("URGENT") : TEXT("STANDARD"), LockedQuote);
        ++Index;
    }
}
