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

FName UGTTWorkshopRepairQueueSubsystem::GetQueuedVehicleId() const
{
    return QueueEntries.Num() > 0 ? QueueEntries[0].PersistentVehicleId : NAME_None;
}

int32 UGTTWorkshopRepairQueueSubsystem::GetLockedQuote() const
{
    return QueueEntries.Num() > 0 ? QueueEntries[0].LockedQuote : 0;
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
            TEXT("Workshop appointment book is full (%d/%d). Cancel an exact reservation or wait for service."),
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
    Entry.State = TEXT("QUEUED");
    QueueEntries.Add(Entry);

    if (!WriteCheckpoint())
    {
        QueueEntries.Pop();
        OutSummary = TEXT("Workshop appointment could not be saved; no charge or vehicle mutation occurred.");
        return false;
    }

    OutSummary = FString::Printf(
        TEXT("QUEUED %d/%d: %s repair | locked quote $%d | appointment day %d %s | exact vehicle ID pinned | no pre-charge."),
        QueueEntries.Num(), MaxQueuedRepairs, *VehicleId.ToString(), Quote, ReadyDay,
        *GTTWorkshopHoursPolicy::FormatHour(ReadyHour));
    UE_LOG(LogGTT, Display,
        TEXT("WORKSHOP_QUEUE_ACCEPTED vehicle=%s locked_quote=%d requested_day=%d requested_hour=%.2f ready_day=%d ready_hour=%.2f position=%d capacity=%d charged=NO exact_id=YES"),
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

    const FString Id = QueueEntries[Index].PersistentVehicleId.ToString();
    RemoveEntryAt(Index, TEXT("PLAYER_CANCELLED"));
    OutSummary = FString::Printf(
        TEXT("Workshop appointment for %s cancelled. No charge was taken; %d/%d appointment slots remain occupied."),
        *Id, QueueEntries.Num(), MaxQueuedRepairs);
    return true;
}

FGTTWorkshopRepairQueueSnapshot UGTTWorkshopRepairQueueSubsystem::BuildSnapshot(
    const FGTTWorkshopRepairQueueSnapshot& Entry, int32 Position) const
{
    FGTTWorkshopRepairQueueSnapshot Snapshot = Entry;
    Snapshot.bQueued = true;
    Snapshot.QueuePosition = Position;
    Snapshot.State = TEXT("QUEUED");
    Snapshot.HoursUntilReady = 0.0f;

    int32 Day = 0;
    float Hour = 0.0f;
    if (ResolveClock(Day, Hour))
    {
        Snapshot.HoursUntilReady = FMath::Max(0.0f,
            static_cast<float>(Entry.ReadyDay - Day) * 24.0f + Entry.ReadyHour - Hour);
        if (Day > Entry.ReadyDay || (Day == Entry.ReadyDay && Hour + KINDA_SMALL_NUMBER >= Entry.ReadyHour))
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
    return FText::FromString(FString::Printf(
        TEXT("Workshop appointments: %d/%d | next %s #%d %s | locked $%d | day %d %s | ETA %.1f h | no pre-charge"),
        QueueEntries.Num(), MaxQueuedRepairs, *Next.State, Next.QueuePosition,
        *Next.PersistentVehicleId.ToString(), Next.LockedQuote, Next.ReadyDay,
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
        Entry.State = TEXT("QUEUED");
        QueueEntries.Add(Entry);
    }

    QueueEntries.Sort([](const FGTTWorkshopRepairQueueSnapshot& A, const FGTTWorkshopRepairQueueSnapshot& B)
    {
        if (A.ReadyDay != B.ReadyDay) return A.ReadyDay < B.ReadyDay;
        if (!FMath::IsNearlyEqual(A.ReadyHour, B.ReadyHour)) return A.ReadyHour < B.ReadyHour;
        return A.PersistentVehicleId.LexicalLess(B.PersistentVehicleId);
    });

    if (QueueEntries.Num() == 0)
    {
        ClearCheckpoint(TEXT("NO_VALID_APPOINTMENTS"));
        return;
    }

    UE_LOG(LogGTT, Display,
        TEXT("WORKSHOP_QUEUE_RESTORED count=%d capacity=%d first_vehicle=%s first_locked_quote=%d charged=NO exact_id=YES legacy_migrated=%s"),
        QueueEntries.Num(), MaxQueuedRepairs, *QueueEntries[0].PersistentVehicleId.ToString(),
        QueueEntries[0].LockedQuote, bMigratedLegacy ? TEXT("YES") : TEXT("NO"));
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
        const FGTTWorkshopRepairQueueSnapshot Entry = QueueEntries[Index];
        const bool bDue = Day > Entry.ReadyDay
            || (Day == Entry.ReadyDay && Hour + KINDA_SMALL_NUMBER >= Entry.ReadyHour);
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
        if (!IsVehicleAtWorkshop(Vehicle))
        {
            ++Index;
            continue;
        }

        const int32 LockedQuote = Entry.LockedQuote;
        if (!Economy->SpendCash(LockedQuote, FString::Printf(
            TEXT("Queued workshop repair %s - $%d"), *Entry.PersistentVehicleId.ToString(), LockedQuote)))
        {
            if (NoticeCooldown <= 0.0f)
            {
                Economy->PushMessage(FString::Printf(
                    TEXT("Workshop appointment #%d for %s is ready but needs $%d. It stays locked/unpaid; later due appointments can still proceed."),
                    Index + 1, *Entry.PersistentVehicleId.ToString(), LockedQuote), 6.0f);
                NoticeCooldown = 8.0f;
            }
            ++Index; // Capacity rule: an underfunded vehicle never blocks later due appointments.
            continue;
        }

        if (!Vehicle->ApplyNativeWorkshopService())
        {
            Economy->AddCash(LockedQuote, TEXT("Queued workshop repair rollback"));
            Economy->PushMessage(TEXT("Queued workshop repair could not be applied; payment returned and only that appointment was cleared."), 6.0f);
            RemoveEntryAt(Index, TEXT("SERVICE_MUTATION_FAILED"));
            continue;
        }

        const FString VehicleIdText = Entry.PersistentVehicleId.ToString();
        const int32 Charged = LockedQuote;
        RemoveEntryAt(Index, TEXT("SERVICE_COMPLETED"));
        if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
            GameMode->SaveProgress();

        Economy->PushMessage(FString::Printf(
            TEXT("Workshop appointment complete: %s | charged locked quote $%d exactly once | %d/%d appointments remain."),
            *VehicleIdText, Charged, QueueEntries.Num(), MaxQueuedRepairs), 8.0f);
        UE_LOG(LogGTT, Display,
            TEXT("WORKSHOP_QUEUE_COMPLETED vehicle=%s charged=%d locked_quote_match=YES exact_id=YES saved=YES remaining=%d capacity=%d"),
            *VehicleIdText, Charged, QueueEntries.Num(), MaxQueuedRepairs);
        // Do not increment Index: after RemoveEntryAt, the next appointment moved into this slot.
    }
}
