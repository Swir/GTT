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
    if (bQueued) TryExecuteReadyReservation();
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

bool UGTTWorkshopRepairQueueSubsystem::TryQueueNearestEligibleNativeRoadVehicle(
    const FVector& Origin, float SearchRadius, FString& OutSummary)
{
    if (!bLoaded) LoadCheckpointOnce();

    int32 Day = 0;
    float Hour = 0.0f;
    if (!ResolveClock(Day, Hour))
    {
        OutSummary = TEXT("Workshop queue unavailable: world clock is missing; minimal maps stay fail-open.");
        return false;
    }
    if (GTTWorkshopHoursPolicy::IsOpen(Hour))
    {
        OutSummary = TEXT("Workshop is open now; use normal workshop service instead of a deferred reservation.");
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
        if (!State.bOwnedByPlayer || Candidate->GetPersistentVehicleId().IsNone()) continue;
        if (!NeedsMechanicalRepair(Candidate)) continue;
        if (RequiresHardWorkshopHold(Candidate->GetPersistentVehicleId())) continue;
        if (!IsCargoVehicleCompatible(Candidate->GetPersistentVehicleId())) continue;
        if (IsVehicleImpounded(Candidate->GetPersistentVehicleId())) continue;

        const float DistanceSquared = FVector::DistSquared2D(Origin, Candidate->GetActorLocation());
        if (DistanceSquared <= BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            Best = Candidate;
        }
    }

    if (!Best)
    {
        OutSummary = TEXT("No eligible owned damaged/mobile native road vehicle is close enough to reserve. Hard WORKSHOP HOLD vehicles use emergency recovery instead.");
        return false;
    }

    const FName VehicleId = Best->GetPersistentVehicleId();
    if (bQueued)
    {
        if (QueuedVehicleId == VehicleId)
        {
            OutSummary = FString::Printf(
                TEXT("Workshop reservation already QUEUED for %s | locked $%d | ready day %d %s | no pre-charge."),
                *VehicleId.ToString(), LockedQuote, ReadyDay, *GTTWorkshopHoursPolicy::FormatHour(ReadyHour));
            return true;
        }
        OutSummary = FString::Printf(
            TEXT("Workshop queue already belongs to %s. Cancel that exact reservation before booking another vehicle."),
            *QueuedVehicleId.ToString());
        return false;
    }

    UGTTBreakdownDecisionSubsystem* Decision = World->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    const int32 Quote = Decision
        ? Decision->CalculateRepairEstimate(Best, DefaultWorkshopBaseCost)
        : DefaultWorkshopBaseCost + Best->GetBodyDamageRepairSurcharge();
    if (Quote <= 0)
    {
        OutSummary = TEXT("Workshop queue rejected an invalid repair quote.");
        return false;
    }

    int32 NextDay = Day;
    float NextHour = GTTWorkshopHoursPolicy::OpeningHour;
    GTTWorkshopHoursPolicy::ResolveNextOpening(Day, Hour, NextDay, NextHour);

    bQueued = true;
    QueuedVehicleId = VehicleId;
    LockedQuote = Quote;
    RequestedDay = Day;
    RequestedHour = Hour;
    ReadyDay = NextDay;
    ReadyHour = NextHour;

    if (!WriteCheckpoint())
    {
        bQueued = false;
        QueuedVehicleId = NAME_None;
        LockedQuote = 0;
        OutSummary = TEXT("Workshop reservation could not be saved; no charge or vehicle mutation occurred.");
        return false;
    }

    OutSummary = FString::Printf(
        TEXT("QUEUED: %s repair | locked quote $%d | ready day %d %s | exact vehicle ID pinned | no pre-charge. Park that same vehicle at the workshop before opening."),
        *QueuedVehicleId.ToString(), LockedQuote, ReadyDay, *GTTWorkshopHoursPolicy::FormatHour(ReadyHour));
    UE_LOG(LogGTT, Display,
        TEXT("WORKSHOP_QUEUE_ACCEPTED vehicle=%s locked_quote=%d requested_day=%d requested_hour=%.2f ready_day=%d ready_hour=%.2f charged=NO exact_id=YES"),
        *QueuedVehicleId.ToString(), LockedQuote, RequestedDay, RequestedHour, ReadyDay, ReadyHour);
    return true;
}

bool UGTTWorkshopRepairQueueSubsystem::CancelQueuedRepair(FName VehicleId, FString& OutSummary)
{
    if (!bLoaded) LoadCheckpointOnce();
    if (!bQueued)
    {
        OutSummary = TEXT("Workshop queue is empty.");
        return false;
    }
    if (VehicleId.IsNone() || VehicleId != QueuedVehicleId)
    {
        OutSummary = FString::Printf(TEXT("Workshop reservation belongs to %s; exact-ID cancellation rejected."), *QueuedVehicleId.ToString());
        return false;
    }
    const FString Id = QueuedVehicleId.ToString();
    ClearCheckpoint(TEXT("PLAYER_CANCELLED"));
    OutSummary = FString::Printf(TEXT("Workshop reservation for %s cancelled. No charge was taken."), *Id);
    return true;
}

FGTTWorkshopRepairQueueSnapshot UGTTWorkshopRepairQueueSubsystem::GetQueueSnapshot() const
{
    FGTTWorkshopRepairQueueSnapshot Snapshot;
    Snapshot.bQueued = bQueued;
    Snapshot.PersistentVehicleId = QueuedVehicleId;
    Snapshot.LockedQuote = LockedQuote;
    Snapshot.RequestedDay = RequestedDay;
    Snapshot.RequestedHour = RequestedHour;
    Snapshot.ReadyDay = ReadyDay;
    Snapshot.ReadyHour = ReadyHour;
    Snapshot.State = bQueued ? TEXT("QUEUED") : TEXT("EMPTY");

    if (bQueued)
    {
        int32 Day = 0;
        float Hour = 0.0f;
        if (ResolveClock(Day, Hour))
        {
            Snapshot.HoursUntilReady = FMath::Max(0.0f,
                static_cast<float>(ReadyDay - Day) * 24.0f + ReadyHour - Hour);
            if (Day > ReadyDay || (Day == ReadyDay && Hour + KINDA_SMALL_NUMBER >= ReadyHour))
            {
                Snapshot.State = TEXT("READY");
                Snapshot.HoursUntilReady = 0.0f;
            }
        }
    }
    return Snapshot;
}

FText UGTTWorkshopRepairQueueSubsystem::GetQueueStatusText() const
{
    if (!bQueued) return FText::FromString(TEXT("Workshop queue: EMPTY"));
    const FGTTWorkshopRepairQueueSnapshot Snapshot = GetQueueSnapshot();
    return FText::FromString(FString::Printf(
        TEXT("Workshop queue: %s | %s | locked $%d | day %d %s | ETA %.1f h | no pre-charge"),
        *Snapshot.State, *QueuedVehicleId.ToString(), LockedQuote,
        ReadyDay, *GTTWorkshopHoursPolicy::FormatHour(ReadyHour), Snapshot.HoursUntilReady));
}

void UGTTWorkshopRepairQueueSubsystem::LoadCheckpointOnce()
{
    bLoaded = true;
    UGTTWorkshopQueueSaveGame* Save = Cast<UGTTWorkshopQueueSaveGame>(
        UGameplayStatics::LoadGameFromSlot(WorkshopQueueSlot, SaveUserIndex));
    if (!Save) return;

    if (Save->SchemaVersion != 1 || !Save->bQueued || Save->PersistentVehicleId.IsNone()
        || Save->LockedQuote <= 0 || Save->RequestedDay <= 0 || Save->ReadyDay <= 0
        || Save->ReadyHour < 0.0f || Save->ReadyHour >= 24.0f)
    {
        ClearCheckpoint(TEXT("INVALID_CHECKPOINT"));
        return;
    }

    if (!IsCargoVehicleCompatible(Save->PersistentVehicleId) || IsVehicleImpounded(Save->PersistentVehicleId))
    {
        ClearCheckpoint(TEXT("AUTHORITY_CONFLICT"));
        return;
    }

    bQueued = true;
    QueuedVehicleId = Save->PersistentVehicleId;
    LockedQuote = Save->LockedQuote;
    RequestedDay = Save->RequestedDay;
    RequestedHour = Save->RequestedHour;
    ReadyDay = Save->ReadyDay;
    ReadyHour = Save->ReadyHour;
    UE_LOG(LogGTT, Display,
        TEXT("WORKSHOP_QUEUE_RESTORED vehicle=%s locked_quote=%d ready_day=%d ready_hour=%.2f charged=NO exact_id=YES"),
        *QueuedVehicleId.ToString(), LockedQuote, ReadyDay, ReadyHour);
}

bool UGTTWorkshopRepairQueueSubsystem::WriteCheckpoint()
{
    UGTTWorkshopQueueSaveGame* Save = Cast<UGTTWorkshopQueueSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UGTTWorkshopQueueSaveGame::StaticClass()));
    if (!Save) return false;
    Save->bQueued = bQueued;
    Save->PersistentVehicleId = QueuedVehicleId;
    Save->LockedQuote = LockedQuote;
    Save->RequestedDay = RequestedDay;
    Save->RequestedHour = RequestedHour;
    Save->ReadyDay = ReadyDay;
    Save->ReadyHour = ReadyHour;
    return UGameplayStatics::SaveGameToSlot(Save, WorkshopQueueSlot, SaveUserIndex);
}

void UGTTWorkshopRepairQueueSubsystem::ClearCheckpoint(const TCHAR* Reason)
{
    if (UGameplayStatics::DoesSaveGameExist(WorkshopQueueSlot, SaveUserIndex))
        UGameplayStatics::DeleteGameInSlot(WorkshopQueueSlot, SaveUserIndex);

    bQueued = false;
    QueuedVehicleId = NAME_None;
    LockedQuote = 0;
    RequestedDay = 0;
    RequestedHour = 0.0f;
    ReadyDay = 0;
    ReadyHour = GTTWorkshopHoursPolicy::OpeningHour;
    UE_LOG(LogGTT, VeryVerbose, TEXT("WORKSHOP_QUEUE_CLEARED reason=%s charged=NO"), Reason ? Reason : TEXT("UNKNOWN"));
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

AGTTRoadVehicleNativePawn* UGTTWorkshopRepairQueueSubsystem::FindExactQueuedVehicle(bool& bAmbiguous) const
{
    bAmbiguous = false;
    UWorld* World = GetWorld();
    if (!World || QueuedVehicleId.IsNone()) return nullptr;

    AGTTRoadVehicleNativePawn* Found = nullptr;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Candidate = *It;
        if (!IsValid(Candidate) || Candidate->GetPersistentVehicleId() != QueuedVehicleId) continue;
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

void UGTTWorkshopRepairQueueSubsystem::TryExecuteReadyReservation()
{
    int32 Day = 0;
    float Hour = 0.0f;
    if (!ResolveClock(Day, Hour)) return;
    if (Day < ReadyDay || (Day == ReadyDay && Hour + KINDA_SMALL_NUMBER < ReadyHour)) return;
    if (!GTTWorkshopHoursPolicy::IsOpen(Hour)) return;

    if (!IsCargoVehicleCompatible(QueuedVehicleId) || IsVehicleImpounded(QueuedVehicleId))
    {
        ClearCheckpoint(TEXT("AUTHORITY_CONFLICT_AT_EXECUTION"));
        return;
    }
    if (RequiresHardWorkshopHold(QueuedVehicleId))
    {
        ClearCheckpoint(TEXT("HARD_HOLD_TAKES_PRIORITY"));
        return;
    }

    bool bAmbiguous = false;
    AGTTRoadVehicleNativePawn* Vehicle = FindExactQueuedVehicle(bAmbiguous);
    if (bAmbiguous)
    {
        ClearCheckpoint(TEXT("AMBIGUOUS_PERSISTENT_ID"));
        return;
    }
    if (!Vehicle || !Vehicle->IsLegacyTakeoverActive() || !Vehicle->GetMigrationSnapshot().bOwnedByPlayer)
        return;

    if (!NeedsMechanicalRepair(Vehicle))
    {
        ClearCheckpoint(TEXT("NO_LONGER_NEEDS_REPAIR"));
        return;
    }
    if (!IsVehicleAtWorkshop(Vehicle)) return;

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    UGTTPlayerEconomyComponent* Economy = PlayerPawn
        ? UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn)
        : nullptr;
    if (!Economy) return;

    if (!Economy->SpendCash(LockedQuote, FString::Printf(
        TEXT("Queued workshop repair %s - $%d"), *QueuedVehicleId.ToString(), LockedQuote)))
    {
        if (NoticeCooldown <= 0.0f)
        {
            Economy->PushMessage(FString::Printf(
                TEXT("QUEUED workshop repair for %s is ready, but needs $%d. Reservation stays locked and unpaid."),
                *QueuedVehicleId.ToString(), LockedQuote), 6.0f);
            NoticeCooldown = 8.0f;
        }
        return;
    }

    if (!Vehicle->ApplyNativeWorkshopService())
    {
        Economy->AddCash(LockedQuote, TEXT("Queued workshop repair rollback"));
        Economy->PushMessage(TEXT("Queued workshop repair could not be applied; payment returned and reservation cleared."), 6.0f);
        ClearCheckpoint(TEXT("SERVICE_MUTATION_FAILED"));
        return;
    }

    const FString VehicleIdText = QueuedVehicleId.ToString();
    const int32 Charged = LockedQuote;
    ClearCheckpoint(TEXT("SERVICE_COMPLETED"));
    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
        GameMode->SaveProgress();

    Economy->PushMessage(FString::Printf(
        TEXT("QUEUED workshop repair complete: %s | charged locked quote $%d exactly once | reservation cleared."),
        *VehicleIdText, Charged), 8.0f);
    UE_LOG(LogGTT, Display,
        TEXT("WORKSHOP_QUEUE_COMPLETED vehicle=%s charged=%d locked_quote_match=YES exact_id=YES saved=YES"),
        *VehicleIdText, Charged);
}
