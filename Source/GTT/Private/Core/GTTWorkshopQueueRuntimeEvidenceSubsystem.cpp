#include "Core/GTTWorkshopQueueRuntimeEvidenceSubsystem.h"

#include "Components/PrimitiveComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Save/GTTSaveGame.h"
#include "Save/GTTWorkshopQueueSaveGame.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTServiceTerminal.h"
#include "World/GTTWorkshopHoursPolicy.h"
#include "World/GTTWorkshopRepairQueueSubsystem.h"
#include "GTT.h"

namespace
{
const FString QueueSlot(TEXT("GTT_WorkshopQueue_01"));
const FString PrimaryWorldSlot(TEXT("GTT_Prototype_01"));
constexpr int32 SaveUserIndex = 0;
constexpr float StartDelaySeconds = 410.0f;
constexpr float GlobalDeadlineSeconds = 434.0f;
constexpr float QueueTickProofSeconds = 1.35f;
constexpr float WorkshopStageOffsetCm = 140.0f;
constexpr float ExactVehicleAwayOffsetCm = 2600.0f;
constexpr int32 MinimumEvidenceCash = 6000;
}

void UGTTWorkshopQueueRuntimeEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTWorkshopHoursRuntimeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTWorkshopQueueRuntimeScenario"));
    if (bEnabled)
    {
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_QUEUE_RUNTIME_BEGIN version=1 route=after-hours-book-checkpoint-substitute-exact-service start_delay=%.1f deadline=%.1f exact_vehicle=required no_precharge=required single_debit=required"),
            StartDelaySeconds, GlobalDeadlineSeconds);
    }
}

TStatId UGTTWorkshopQueueRuntimeEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTWorkshopQueueRuntimeEvidenceSubsystem, STATGROUP_Tickables);
}

bool UGTTWorkshopQueueRuntimeEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

AGTTServiceTerminal* UGTTWorkshopQueueRuntimeEvidenceSubsystem::FindWorkshopTerminal() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTServiceTerminal> It(World); It; ++It)
    {
        if (IsValid(*It) && It->GetServiceType() == EGTTServiceType::Workshop) return *It;
    }
    return nullptr;
}

AGTTRoadVehicleNativePawn* UGTTWorkshopQueueRuntimeEvidenceSubsystem::FindEvidenceVehicle() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    FName PreferredId = NAME_None;
    if (const UGTTSaveGame* Save = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex)))
    {
        if (Save->bFarmCargoContractActive) PreferredId = Save->FarmCargoBoundVehicleId;
    }

    AGTTRoadVehicleNativePawn* Fallback = nullptr;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Candidate = *It;
        if (!IsValid(Candidate) || Candidate->GetPersistentVehicleId().IsNone()) continue;
        if (!Candidate->IsLegacyTakeoverActive()) continue;
        if (!Candidate->GetMigrationSnapshot().bOwnedByPlayer) continue;
        if (!PreferredId.IsNone() && Candidate->GetPersistentVehicleId() == PreferredId) return Candidate;
        if (!Fallback) Fallback = Candidate;
    }
    return Fallback;
}

AGTTRoadVehicleNativePawn* UGTTWorkshopQueueRuntimeEvidenceSubsystem::FindDecoyVehicle(FName ExcludedId) const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Candidate = *It;
        if (!IsValid(Candidate) || Candidate->GetPersistentVehicleId().IsNone()
            || Candidate->GetPersistentVehicleId() == ExcludedId) continue;
        if (!Candidate->IsLegacyTakeoverActive()) continue;
        if (!Candidate->GetMigrationSnapshot().bOwnedByPlayer) continue;
        return Candidate;
    }
    return nullptr;
}

bool UGTTWorkshopQueueRuntimeEvidenceSubsystem::ResolveActors()
{
    UWorld* World = GetWorld();
    if (!World) return false;
    if (!Workshop.IsValid()) Workshop = FindWorkshopTerminal();
    if (!Queue.IsValid()) Queue = World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>();
    if (!DayNight.IsValid())
    {
        if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) DayNight = GameMode->GetDayNightCycle();
    }
    if (!Vehicle.IsValid()) Vehicle = FindEvidenceVehicle();
    if (Vehicle.IsValid() && !Decoy.IsValid()) Decoy = FindDecoyVehicle(Vehicle->GetPersistentVehicleId());
    if (!Economy.IsValid())
    {
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
        {
            if (AGTTRoadVehicleNativePawn* Native = Cast<AGTTRoadVehicleNativePawn>(PlayerPawn)) PlayerPawn = Native->GetDriverPawn();
            if (PlayerPawn) Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
        }
    }
    return Workshop.IsValid() && Queue.IsValid() && DayNight.IsValid() && Vehicle.IsValid() && Decoy.IsValid() && Economy.IsValid();
}

void UGTTWorkshopQueueRuntimeEvidenceSubsystem::StageVehicle(AGTTRoadVehicleNativePawn* Target, const FVector& Location) const
{
    if (!IsValid(Target)) return;
    Target->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
    {
        if (RootPrimitive->IsSimulatingPhysics())
        {
            RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
            RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        }
    }
}

void UGTTWorkshopQueueRuntimeEvidenceSubsystem::StageOrdinaryDamage()
{
    if (!Vehicle.IsValid()) return;
    FGTTRoadVehicleMigrationSnapshot State = Vehicle->GetMigrationSnapshot();
    State.ConditionPercent = 0.61f;
    State.TireIntegrity = 0.68f;
    State.FuelLiters = FMath::Min(State.FuelLiters, Vehicle->GetFuelCapacityLiters() * 0.45f);
    Vehicle->RestorePersistentMigrationSnapshot(State);
}

bool UGTTWorkshopQueueRuntimeEvidenceSubsystem::CapturePrimaryCargoBaseline()
{
    const UGTTSaveGame* Save = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex));
    if (!Save) return false;
    bBaselineCargoActive = Save->bFarmCargoContractActive;
    BaselineCargoStage = Save->FarmCargoStage;
    BaselineCargoTimeRemaining = Save->FarmCargoTimeRemaining;
    BaselineCargoIntegrity = Save->FarmCargoIntegrity;
    BaselineCargoVehicleId = Save->FarmCargoBoundVehicleId;
    return true;
}

bool UGTTWorkshopQueueRuntimeEvidenceSubsystem::VerifyPrimaryCargoContinuity()
{
    const UGTTSaveGame* Save = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex));
    if (!Save || !bCargoBaselineCaptured) return false;
    const bool bAuthorityStable = Save->bFarmCargoContractActive == bBaselineCargoActive
        && Save->FarmCargoStage == BaselineCargoStage
        && Save->FarmCargoBoundVehicleId == BaselineCargoVehicleId;
    const bool bIntegrityNotImproved = Save->FarmCargoIntegrity <= BaselineCargoIntegrity + 0.001f;
    const bool bTimerNotRewound = Save->FarmCargoTimeRemaining <= BaselineCargoTimeRemaining + 0.05f;
    return bAuthorityStable && bIntegrityNotImproved && bTimerNotRewound;
}

void UGTTWorkshopQueueRuntimeEvidenceSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    UE_LOG(LogGTT, Error, TEXT("WORKSHOP_QUEUE_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTWorkshopQueueRuntimeEvidenceSubsystem::RestoreBaselineState()
{
    UWorld* World = GetWorld();
    if (!World || !bBaselineCaptured) return;
    if (Queue.IsValid() && Queue->HasQueuedRepair() && !VehicleId.IsNone())
    {
        FString Summary;
        Queue->CancelQueuedRepair(VehicleId, Summary);
    }
    if (Vehicle.IsValid())
    {
        Vehicle->RestorePersistentMigrationSnapshot(BaselineMigration);
        Vehicle->RestorePersistentBodyDamage(BaselineBodyDamage, BaselineDetachedMask);
        Vehicle->SetActorTransform(BaselineTransform, false, nullptr, ETeleportType::ResetPhysics);
    }
    if (Decoy.IsValid()) Decoy->SetActorTransform(DecoyBaselineTransform, false, nullptr, ETeleportType::ResetPhysics);
    if (Economy.IsValid()) Economy->RestoreState(BaselineCash, BaselineFishCount, BaselineFishWeightKg);
    if (DayNight.IsValid()) DayNight->RestoreTime(BaselineDay, BaselineHour);
    if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) GameMode->SaveProgress();
}

void UGTTWorkshopQueueRuntimeEvidenceSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bPass = bSequenceHealthy && bBookingAccepted && bNoPrecharge && bCheckpointLoaded
        && bLockedQuotePreserved && bSubstituteRejected && bReservationPreserved && bSingleDebit
        && bExactExecution && bSidecarCleared && bIdentityPreserved && bMechanicalRepaired
        && bRefuelled && bCargoContinuity && LockedQuote > 0 && !VehicleId.IsNone();

    UE_LOG(LogGTT, Log,
        TEXT("WORKSHOP_QUEUE_RUNTIME_COMPLETE result=%s booked=%d no_precharge=%d checkpoint_loaded=%d locked_quote_preserved=%d substitute_rejected=%d reservation_preserved=%d single_debit=%d exact_execution=%d sidecar_cleared=%d identity_preserved=%d repaired=%d refuelled=%d cargo_continuity=%d locked_quote=%d vehicle=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bBookingAccepted ? 1 : 0, bNoPrecharge ? 1 : 0,
        bCheckpointLoaded ? 1 : 0, bLockedQuotePreserved ? 1 : 0, bSubstituteRejected ? 1 : 0,
        bReservationPreserved ? 1 : 0, bSingleDebit ? 1 : 0, bExactExecution ? 1 : 0,
        bSidecarCleared ? 1 : 0, bIdentityPreserved ? 1 : 0, bMechanicalRepaired ? 1 : 0,
        bRefuelled ? 1 : 0, bCargoContinuity ? 1 : 0, LockedQuote, *VehicleId.ToString(),
        Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = EPhase::Complete;
    bFinished = true;
}

void UGTTWorkshopQueueRuntimeEvidenceSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    if (Elapsed < StartDelaySeconds) return;
    if (Elapsed >= GlobalDeadlineSeconds)
    {
        MarkFailure(TEXT("global-sequence-timeout"));
        FinishScenario(TEXT("deadline"));
        return;
    }
    if (!ResolveActors())
    {
        if (Elapsed > StartDelaySeconds + 4.0f)
        {
            MarkFailure(TEXT("workshop-queue-world-actors-unavailable"));
            FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    switch (Phase)
    {
    case EPhase::Waiting:
        Phase = EPhase::PrepareAndBook;
        break;

    case EPhase::PrepareAndBook:
    {
        if (Queue->HasQueuedRepair())
        {
            MarkFailure(TEXT("queue-not-empty-at-start")); FinishScenario(TEXT("prepare-failed")); return;
        }
        VehicleId = Vehicle->GetPersistentVehicleId();
        BaselineDay = DayNight->GetDayNumber();
        BaselineHour = DayNight->GetTimeOfDayHours();
        BaselineCash = Economy->GetCash();
        BaselineFishCount = Economy->GetFishCount();
        BaselineFishWeightKg = Economy->GetFishWeightKg();
        BaselineMigration = Vehicle->GetMigrationSnapshot();
        BaselineBodyDamage = Vehicle->GetBodyDamageSnapshot();
        BaselineDetachedMask = Vehicle->GetDetachedPanelMask();
        BaselineTransform = Vehicle->GetActorTransform();
        DecoyBaselineTransform = Decoy->GetActorTransform();
        bCargoBaselineCaptured = CapturePrimaryCargoBaseline();
        bBaselineCaptured = true;
        if (!bCargoBaselineCaptured)
        {
            MarkFailure(TEXT("primary-save-missing")); FinishScenario(TEXT("prepare-failed")); return;
        }

        Economy->RestoreState(FMath::Max(BaselineCash, MinimumEvidenceCash), BaselineFishCount, BaselineFishWeightKg);
        StageVehicle(Decoy.Get(), Workshop->GetActorLocation() + FVector(ExactVehicleAwayOffsetCm, 1200.0f, 80.0f));
        StageVehicle(Vehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 0.0f, 80.0f));
        StageOrdinaryDamage();
        DayNight->RestoreTime(BaselineDay, GTTWorkshopHoursPolicy::ClosingHour + 0.25f);

        CashBeforeBooking = Economy->GetCash();
        FString Summary;
        bBookingAccepted = Queue->TryQueueNearestEligibleNativeRoadVehicle(Workshop->GetActorLocation(), 900.0f, Summary);
        const FGTTWorkshopRepairQueueSnapshot Snapshot = Queue->GetQueueSnapshot();
        LockedQuote = Snapshot.LockedQuote;
        ReadyDay = Snapshot.ReadyDay;
        ReadyHour = Snapshot.ReadyHour;
        bNoPrecharge = Economy->GetCash() == CashBeforeBooking;
        const bool bExactPinned = Snapshot.bQueued && Snapshot.PersistentVehicleId == VehicleId;
        const bool bScheduleValid = ReadyDay >= BaselineDay && FMath::IsNearlyEqual(ReadyHour, GTTWorkshopHoursPolicy::OpeningHour, 0.001f);
        const bool bSidecarSaved = UGameplayStatics::DoesSaveGameExist(QueueSlot, SaveUserIndex);
        const bool bPass = bBookingAccepted && bNoPrecharge && bExactPinned && bScheduleValid && bSidecarSaved && LockedQuote > 0;
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_QUEUE_RUNTIME phase=BOOK result=%s accepted=%d no_precharge=%d exact_id=%d sidecar_saved=%d locked_quote=%d ready_day=%d ready_hour=%.2f vehicle=%s"),
            bPass ? TEXT("PASS") : TEXT("FAIL"), bBookingAccepted ? 1 : 0, bNoPrecharge ? 1 : 0,
            bExactPinned ? 1 : 0, bSidecarSaved ? 1 : 0, LockedQuote, ReadyDay, ReadyHour, *VehicleId.ToString());
        if (!bPass) { MarkFailure(TEXT("booking-contract-failed")); FinishScenario(TEXT("book-failed")); return; }
        Phase = EPhase::VerifyCheckpointLoad;
        break;
    }

    case EPhase::VerifyCheckpointLoad:
    {
        const UGTTWorkshopQueueSaveGame* Save = Cast<UGTTWorkshopQueueSaveGame>(UGameplayStatics::LoadGameFromSlot(QueueSlot, SaveUserIndex));
        bCheckpointLoaded = Save && Save->SchemaVersion == 1 && Save->bQueued;
        bLockedQuotePreserved = bCheckpointLoaded && Save->PersistentVehicleId == VehicleId
            && Save->LockedQuote == LockedQuote && Save->ReadyDay == ReadyDay
            && FMath::IsNearlyEqual(Save->ReadyHour, ReadyHour, 0.001f);
        bNoPrecharge = bNoPrecharge && Economy->GetCash() == CashBeforeBooking;
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_QUEUE_RUNTIME phase=CHECKPOINT_LOAD result=%s loaded=%d exact_id=%d locked_quote_preserved=%d no_precharge=%d locked_quote=%d vehicle=%s"),
            (bCheckpointLoaded && bLockedQuotePreserved && bNoPrecharge) ? TEXT("PASS") : TEXT("FAIL"),
            bCheckpointLoaded ? 1 : 0, (Save && Save->PersistentVehicleId == VehicleId) ? 1 : 0,
            bLockedQuotePreserved ? 1 : 0, bNoPrecharge ? 1 : 0, LockedQuote, *VehicleId.ToString());
        if (!bCheckpointLoaded || !bLockedQuotePreserved || !bNoPrecharge)
        {
            MarkFailure(TEXT("checkpoint-roundtrip-failed")); FinishScenario(TEXT("checkpoint-failed")); return;
        }

        StageVehicle(Vehicle.Get(), Workshop->GetActorLocation() + FVector(ExactVehicleAwayOffsetCm, 0.0f, 80.0f));
        StageVehicle(Decoy.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 0.0f, 80.0f));
        DayNight->RestoreTime(ReadyDay, ReadyHour);
        PhaseStartedAt = Elapsed;
        Phase = EPhase::AwaitSubstituteRejection;
        break;
    }

    case EPhase::AwaitSubstituteRejection:
    {
        if (Elapsed - PhaseStartedAt < QueueTickProofSeconds) break;
        const FGTTWorkshopRepairQueueSnapshot Snapshot = Queue->GetQueueSnapshot();
        bReservationPreserved = Snapshot.bQueued && Snapshot.PersistentVehicleId == VehicleId && Snapshot.LockedQuote == LockedQuote;
        bSubstituteRejected = bReservationPreserved && Economy->GetCash() == CashBeforeBooking
            && Decoy->GetPersistentVehicleId() != VehicleId;
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_QUEUE_RUNTIME phase=SUBSTITUTE result=%s substitute_rejected=%d reservation_preserved=%d no_charge=%d exact_away=1 decoy=%s target=%s"),
            bSubstituteRejected ? TEXT("PASS") : TEXT("FAIL"), bSubstituteRejected ? 1 : 0,
            bReservationPreserved ? 1 : 0, Economy->GetCash() == CashBeforeBooking ? 1 : 0,
            *Decoy->GetPersistentVehicleId().ToString(), *VehicleId.ToString());
        if (!bSubstituteRejected)
        {
            MarkFailure(TEXT("substitute-vehicle-was-not-rejected")); FinishScenario(TEXT("substitute-failed")); return;
        }
        StageVehicle(Vehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 0.0f, 80.0f));
        PhaseStartedAt = Elapsed;
        Phase = EPhase::AwaitExactExecution;
        break;
    }

    case EPhase::AwaitExactExecution:
    {
        if (Elapsed - PhaseStartedAt < QueueTickProofSeconds) break;
        const FGTTRoadVehicleMigrationSnapshot After = Vehicle->GetMigrationSnapshot();
        const int32 Charged = CashBeforeBooking - Economy->GetCash();
        bSingleDebit = Charged == LockedQuote;
        bExactExecution = !Queue->HasQueuedRepair() && Charged > 0;
        bSidecarCleared = !UGameplayStatics::DoesSaveGameExist(QueueSlot, SaveUserIndex);
        bIdentityPreserved = Vehicle->GetPersistentVehicleId() == VehicleId;
        bMechanicalRepaired = After.ConditionPercent >= 0.999f && After.TireIntegrity >= 0.999f;
        bRefuelled = After.FuelLiters + 0.05f >= Vehicle->GetFuelCapacityLiters();
        bCargoContinuity = VerifyPrimaryCargoContinuity();
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_QUEUE_RUNTIME phase=EXACT_SERVICE result=%s completed=%d charged=%d locked_quote=%d single_debit=%d sidecar_cleared=%d identity_preserved=%d repaired=%d refuelled=%d vehicle=%s"),
            (bExactExecution && bSingleDebit && bSidecarCleared && bIdentityPreserved && bMechanicalRepaired && bRefuelled) ? TEXT("PASS") : TEXT("FAIL"),
            bExactExecution ? 1 : 0, Charged, LockedQuote, bSingleDebit ? 1 : 0, bSidecarCleared ? 1 : 0,
            bIdentityPreserved ? 1 : 0, bMechanicalRepaired ? 1 : 0, bRefuelled ? 1 : 0, *VehicleId.ToString());
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_QUEUE_RUNTIME phase=CARGO result=%s authority_preserved=%d active_before=%d active_after=%d bound_before=%s bound_expected=%s stage_before=%d integrity_before=%.3f timer_before=%.2f"),
            bCargoContinuity ? TEXT("PASS") : TEXT("FAIL"), bCargoContinuity ? 1 : 0,
            bBaselineCargoActive ? 1 : 0, bBaselineCargoActive ? 1 : 0,
            *BaselineCargoVehicleId.ToString(), *BaselineCargoVehicleId.ToString(), static_cast<int32>(BaselineCargoStage),
            BaselineCargoIntegrity, BaselineCargoTimeRemaining);
        if (!bExactExecution || !bSingleDebit || !bSidecarCleared || !bIdentityPreserved || !bMechanicalRepaired || !bRefuelled || !bCargoContinuity)
        {
            MarkFailure(TEXT("exact-service-contract-failed")); FinishScenario(TEXT("service-failed")); return;
        }
        FinishScenario(TEXT("complete"));
        break;
    }

    case EPhase::Complete:
        break;
    }
}
