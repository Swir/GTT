#include "Core/GTTWorkshopPriorityPickupRuntimeEvidenceSubsystem.h"

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
constexpr float StartDelaySeconds = 451.0f;
constexpr float GlobalDeadlineSeconds = 468.0f;
constexpr float QueueTickProofSeconds = 0.80f;
constexpr float WorkshopStageOffsetCm = 150.0f;
constexpr int32 MinimumEvidenceCash = 12000;
constexpr float ExpectedUrgentMultiplier = 0.80f;
constexpr int32 ExpectedUrgentSurchargePercent = 20;
}

void UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTWorkshopPriorityPickupRuntimeScenario"));
    if (bEnabled)
    {
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_PRIORITY_PICKUP_RUNTIME_BEGIN version=1 route=standard-urgent-timed-checkout-pickup start_delay=%.1f deadline=%.1f surcharge_percent=20 service_multiplier=0.80 exact_vehicle=required no_precharge=required single_debit=required pickup=required"),
            StartDelaySeconds, GlobalDeadlineSeconds);
    }
}

TStatId UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem, STATGROUP_Tickables);
}

bool UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

AGTTServiceTerminal* UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::FindWorkshopTerminal() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTServiceTerminal> It(World); It; ++It)
    {
        if (IsValid(*It) && It->GetServiceType() == EGTTServiceType::Workshop) return *It;
    }
    return nullptr;
}

bool UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::FindEvidenceVehicles()
{
    UWorld* World = GetWorld();
    if (!World) return false;
    TArray<AGTTRoadVehicleNativePawn*> Candidates;
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        AGTTRoadVehicleNativePawn* Candidate = *It;
        if (!IsValid(Candidate) || Candidate->GetPersistentVehicleId().IsNone()) continue;
        if (!Candidate->IsLegacyTakeoverActive() || !Candidate->GetMigrationSnapshot().bOwnedByPlayer) continue;
        Candidates.Add(Candidate);
    }
    Candidates.Sort([](const AGTTRoadVehicleNativePawn& A, const AGTTRoadVehicleNativePawn& B)
    {
        return A.GetPersistentVehicleId().ToString() < B.GetPersistentVehicleId().ToString();
    });
    if (Candidates.Num() < 2) return false;
    Vehicle = Candidates[0];
    DecoyVehicle = Candidates[1];
    return true;
}

bool UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::ResolveActors()
{
    UWorld* World = GetWorld();
    if (!World) return false;
    if (!Workshop.IsValid()) Workshop = FindWorkshopTerminal();
    if (!Queue.IsValid()) Queue = World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>();
    if (!DayNight.IsValid())
    {
        if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) DayNight = GameMode->GetDayNightCycle();
    }
    if (!Vehicle.IsValid() || !DecoyVehicle.IsValid()) FindEvidenceVehicles();
    if (!Economy.IsValid())
    {
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
        {
            if (AGTTRoadVehicleNativePawn* Native = Cast<AGTTRoadVehicleNativePawn>(PlayerPawn)) PlayerPawn = Native->GetDriverPawn();
            if (PlayerPawn) Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
        }
    }
    return Workshop.IsValid() && Queue.IsValid() && DayNight.IsValid()
        && Vehicle.IsValid() && DecoyVehicle.IsValid() && Economy.IsValid();
}

void UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::StageVehicle(
    AGTTRoadVehicleNativePawn* Target, const FVector& Location) const
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

void UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::StageDamage(AGTTRoadVehicleNativePawn* Target) const
{
    if (!IsValid(Target)) return;
    FGTTRoadVehicleMigrationSnapshot State = Target->GetMigrationSnapshot();
    State.ConditionPercent = 0.46f;
    State.TireIntegrity = 0.58f;
    State.FuelLiters = FMath::Min(State.FuelLiters, Target->GetFuelCapacityLiters() * 0.31f);
    Target->RestorePersistentMigrationSnapshot(State);
}

float UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::CalculateExpectedStandardDuration(
    const AGTTRoadVehicleNativePawn* Target) const
{
    if (!Target) return UGTTWorkshopRepairQueueSubsystem::MinimumServiceDurationHours;
    const FGTTRoadVehicleMigrationSnapshot Migration = Target->GetMigrationSnapshot();
    const FGTTRoadBodyDamageSnapshot Body = Target->GetBodyDamageSnapshot();
    const float ConditionDeficit = 1.0f - FMath::Clamp(Migration.ConditionPercent, 0.0f, 1.0f);
    const float TireDeficit = 1.0f - FMath::Clamp(Migration.TireIntegrity, 0.0f, 1.0f);
    const float FuelCapacity = FMath::Max(1.0f, Target->GetFuelCapacityLiters());
    const float FuelDeficit = 1.0f - FMath::Clamp(Migration.FuelLiters / FuelCapacity, 0.0f, 1.0f);
    const float BodyHealth = FMath::Clamp(
        (Body.FrontHealth + Body.RearHealth + Body.LeftHealth + Body.RightHealth) * 0.25f, 0.0f, 1.0f);
    const float BodyDeficit = 1.0f - BodyHealth;
    const float DetachedPenalty = FMath::Clamp(static_cast<float>(Body.DetachedPanelCount) / 4.0f, 0.0f, 1.0f);
    const float Workload = ConditionDeficit * 0.35f + TireDeficit * 0.20f + FuelDeficit * 0.10f
        + BodyDeficit * 0.25f + DetachedPenalty * 0.10f;
    return FMath::Clamp(
        UGTTWorkshopRepairQueueSubsystem::MinimumServiceDurationHours + Workload,
        UGTTWorkshopRepairQueueSubsystem::MinimumServiceDurationHours,
        UGTTWorkshopRepairQueueSubsystem::MaximumServiceDurationHours);
}

bool UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::VerifyPriorityCheckpoint(bool bRequirePickup) const
{
    const UGTTWorkshopQueueSaveGame* Save = Cast<UGTTWorkshopQueueSaveGame>(
        UGameplayStatics::LoadGameFromSlot(QueueSlot, SaveUserIndex));
    if (!Save || Save->SchemaVersion != 1) return false;
    const FGTTWorkshopQueueSaveEntry* Entry = Save->Appointments.FindByPredicate(
        [this](const FGTTWorkshopQueueSaveEntry& Candidate)
        {
            return Candidate.PersistentVehicleId == VehicleId;
        });
    if (!Entry || !Entry->bUrgent || Entry->LockedQuote != UrgentQuote) return false;
    if (!bRequirePickup) return !Entry->bReadyForPickup && Entry->PaidAmount == 0;
    return Entry->bReadyForPickup && Entry->PaidAmount == UrgentQuote
        && Entry->PaidDay > 0 && Entry->PaidHour >= 0.0f && Entry->PaidHour < 24.0f;
}

bool UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::CapturePrimaryCargoBaseline()
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

bool UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::VerifyPrimaryCargoContinuity() const
{
    const UGTTSaveGame* Save = Cast<UGTTSaveGame>(UGameplayStatics::LoadGameFromSlot(PrimaryWorldSlot, SaveUserIndex));
    if (!Save || !bCargoBaselineCaptured) return false;
    return Save->bFarmCargoContractActive == bBaselineCargoActive
        && Save->FarmCargoStage == BaselineCargoStage
        && Save->FarmCargoBoundVehicleId == BaselineCargoVehicleId
        && Save->FarmCargoIntegrity <= BaselineCargoIntegrity + 0.001f
        && Save->FarmCargoTimeRemaining <= BaselineCargoTimeRemaining + 0.05f;
}

void UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    UE_LOG(LogGTT, Error,
        TEXT("WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::RestoreBaselineState()
{
    UWorld* World = GetWorld();
    if (!World || !bBaselineCaptured) return;
    if (Queue.IsValid())
    {
        const TArray<FGTTWorkshopRepairQueueSnapshot> Remaining = Queue->GetQueueSnapshots();
        for (const FGTTWorkshopRepairQueueSnapshot& Entry : Remaining)
        {
            FString Summary;
            if (Entry.bReadyForPickup)
            {
                if (Vehicle.IsValid() && Entry.PersistentVehicleId == VehicleId)
                    StageVehicle(Vehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 0.0f, 80.0f));
                Queue->ReleaseCompletedRepairForPickup(Entry.PersistentVehicleId, Summary);
            }
            if (Queue->HasQueuedRepairForVehicle(Entry.PersistentVehicleId))
                Queue->CancelQueuedRepair(Entry.PersistentVehicleId, Summary);
        }
    }
    if (Vehicle.IsValid())
    {
        Vehicle->RestorePersistentMigrationSnapshot(BaselineMigration);
        Vehicle->RestorePersistentBodyDamage(BaselineBodyDamage, BaselineDetachedMask);
        Vehicle->SetActorTransform(BaselineTransform, false, nullptr, ETeleportType::ResetPhysics);
    }
    if (Economy.IsValid()) Economy->RestoreState(BaselineCash, BaselineFishCount, BaselineFishWeightKg);
    if (DayNight.IsValid()) DayNight->RestoreTime(BaselineDay, BaselineHour);
    if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) GameMode->SaveProgress();
}

void UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bPass = bSequenceHealthy && bPriorityPromoted && bNoPrecharge && bPriorityPersisted
        && bUrgentTiming && bSingleDebit && bPickupPersisted && bPickupHoldObserved
        && bWrongIdRejected && bExactPickupReleased && bNoSecondCharge && bIdentityPreserved
        && bCargoContinuity && StandardQuote > 0 && UrgentQuote > StandardQuote && !VehicleId.IsNone();

    UE_LOG(LogGTT, Log,
        TEXT("WORKSHOP_PRIORITY_PICKUP_RUNTIME_COMPLETE result=%s priority_promoted=%d no_precharge=%d priority_persisted=%d urgent_timing_x080=%d single_debit=%d pickup_persisted=%d pickup_hold=%d wrong_id_rejected=%d exact_pickup=%d no_second_charge=%d identity_preserved=%d cargo_continuity=%d standard_quote=%d urgent_quote=%d standard_duration=%.3f urgent_duration=%.3f vehicle=%s decoy=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bPriorityPromoted ? 1 : 0, bNoPrecharge ? 1 : 0,
        bPriorityPersisted ? 1 : 0, bUrgentTiming ? 1 : 0, bSingleDebit ? 1 : 0,
        bPickupPersisted ? 1 : 0, bPickupHoldObserved ? 1 : 0, bWrongIdRejected ? 1 : 0,
        bExactPickupReleased ? 1 : 0, bNoSecondCharge ? 1 : 0, bIdentityPreserved ? 1 : 0,
        bCargoContinuity ? 1 : 0, StandardQuote, UrgentQuote, ExpectedStandardDuration,
        ObservedUrgentDuration, *VehicleId.ToString(), *DecoyVehicleId.ToString(),
        Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = EPhase::Complete;
    bFinished = true;
}

void UGTTWorkshopPriorityPickupRuntimeEvidenceSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    if (Elapsed < StartDelaySeconds) return;
    if (Elapsed >= GlobalDeadlineSeconds)
    {
        MarkFailure(TEXT("global-sequence-timeout")); FinishScenario(TEXT("deadline")); return;
    }
    if (!ResolveActors())
    {
        if (Elapsed > StartDelaySeconds + 4.0f)
        {
            MarkFailure(TEXT("priority-pickup-world-actors-unavailable")); FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    switch (Phase)
    {
    case EPhase::Waiting:
        if (Queue->GetQueuedRepairCount() != 0)
        {
            if (Elapsed > StartDelaySeconds + 2.0f)
            {
                MarkFailure(TEXT("queue-not-empty-after-legacy-evidence")); FinishScenario(TEXT("prepare-failed"));
            }
            return;
        }
        Phase = EPhase::PreparePriority;
        break;

    case EPhase::PreparePriority:
    {
        VehicleId = Vehicle->GetPersistentVehicleId();
        DecoyVehicleId = DecoyVehicle->GetPersistentVehicleId();
        BaselineDay = DayNight->GetDayNumber();
        BaselineHour = DayNight->GetTimeOfDayHours();
        BaselineCash = Economy->GetCash();
        BaselineFishCount = Economy->GetFishCount();
        BaselineFishWeightKg = Economy->GetFishWeightKg();
        BaselineMigration = Vehicle->GetMigrationSnapshot();
        BaselineBodyDamage = Vehicle->GetBodyDamageSnapshot();
        BaselineDetachedMask = Vehicle->GetDetachedPanelMask();
        BaselineTransform = Vehicle->GetActorTransform();
        bCargoBaselineCaptured = CapturePrimaryCargoBaseline();
        bBaselineCaptured = true;
        if (!bCargoBaselineCaptured || bBaselineCargoActive || VehicleId.IsNone() || DecoyVehicleId.IsNone()
            || VehicleId == DecoyVehicleId)
        {
            MarkFailure(TEXT("invalid-priority-pickup-baseline")); FinishScenario(TEXT("prepare-failed")); return;
        }

        Economy->RestoreState(FMath::Max(BaselineCash, MinimumEvidenceCash), BaselineFishCount, BaselineFishWeightKg);
        DayNight->RestoreTime(BaselineDay, GTTWorkshopHoursPolicy::ClosingHour + 0.35f);
        StageDamage(Vehicle.Get());
        StageVehicle(Vehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 0.0f, 80.0f));
        ExpectedStandardDuration = CalculateExpectedStandardDuration(Vehicle.Get());
        CashBeforeBooking = Economy->GetCash();

        FString QueueSummary;
        const bool bQueued = Queue->TryQueueNearestEligibleNativeRoadVehicle(Workshop->GetActorLocation(), 900.0f, QueueSummary);
        FGTTWorkshopRepairQueueSnapshot Standard = Queue->GetQueueSnapshot();
        StandardQuote = Standard.LockedQuote;
        ReadyDay = Standard.ReadyDay;
        ReadyHour = Standard.ReadyHour;

        FString PromoteSummary;
        const bool bPromoted = bQueued && Queue->PromoteQueuedRepairToUrgent(VehicleId, PromoteSummary);
        const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
        const FGTTWorkshopRepairQueueSnapshot* Urgent = Snapshots.FindByPredicate(
            [this](const FGTTWorkshopRepairQueueSnapshot& S){ return S.PersistentVehicleId == VehicleId; });
        const int32 ExpectedSurcharge = FMath::Max(1, FMath::CeilToInt(
            static_cast<float>(StandardQuote) * (static_cast<float>(ExpectedUrgentSurchargePercent) / 100.0f)));
        const int32 ExpectedUrgentQuote = StandardQuote + ExpectedSurcharge;
        if (Urgent)
        {
            UrgentQuote = Urgent->LockedQuote;
            ReadyDay = Urgent->ReadyDay;
            ReadyHour = Urgent->ReadyHour;
        }
        bPriorityPromoted = bPromoted && Urgent && Urgent->bUrgent && Urgent->Priority == TEXT("URGENT")
            && UrgentQuote == ExpectedUrgentQuote;
        bNoPrecharge = Economy->GetCash() == CashBeforeBooking;
        bPriorityPersisted = bPriorityPromoted && VerifyPriorityCheckpoint(false);
        const bool bPass = bQueued && bPriorityPromoted && bNoPrecharge && bPriorityPersisted;
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=PRIORITY result=%s queued=%d promoted=%d exact_id=%d standard_quote=%d urgent_quote=%d surcharge_20=%d no_precharge=%d disk_priority=%d vehicle=%s"),
            bPass ? TEXT("PASS") : TEXT("FAIL"), bQueued ? 1 : 0, bPromoted ? 1 : 0,
            Urgent && Urgent->PersistentVehicleId == VehicleId ? 1 : 0, StandardQuote, UrgentQuote,
            UrgentQuote == ExpectedUrgentQuote ? 1 : 0, bNoPrecharge ? 1 : 0,
            bPriorityPersisted ? 1 : 0, *VehicleId.ToString());
        if (!bPass)
        {
            MarkFailure(TEXT("priority-contract-failed")); FinishScenario(TEXT("priority-failed")); return;
        }

        StageVehicle(Vehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 0.0f, 80.0f));
        DayNight->RestoreTime(ReadyDay, ReadyHour);
        PhaseStartedAt = Elapsed;
        Phase = EPhase::AwaitCheckIn;
        break;
    }

    case EPhase::AwaitCheckIn:
    {
        if (Elapsed - PhaseStartedAt < QueueTickProofSeconds) break;
        const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
        const FGTTWorkshopRepairQueueSnapshot* Active = Snapshots.FindByPredicate(
            [this](const FGTTWorkshopRepairQueueSnapshot& S){ return S.PersistentVehicleId == VehicleId; });
        if (!Active || !Active->bCheckedIn)
        {
            MarkFailure(TEXT("urgent-checkin-missing")); FinishScenario(TEXT("checkin-failed")); return;
        }
        ServiceCompleteDay = Active->ServiceCompleteDay;
        ServiceCompleteHour = Active->ServiceCompleteHour;
        ObservedUrgentDuration = static_cast<float>(ServiceCompleteDay - Active->ServiceStartDay) * 24.0f
            + ServiceCompleteHour - Active->ServiceStartHour;
        const float ExpectedUrgentDuration = FMath::Clamp(
            ExpectedStandardDuration * ExpectedUrgentMultiplier,
            UGTTWorkshopRepairQueueSubsystem::MinimumServiceDurationHours * ExpectedUrgentMultiplier,
            UGTTWorkshopRepairQueueSubsystem::MaximumServiceDurationHours * ExpectedUrgentMultiplier);
        bUrgentTiming = Active->bUrgent && Active->Priority == TEXT("URGENT")
            && FMath::IsNearlyEqual(ObservedUrgentDuration, ExpectedUrgentDuration, 0.011f)
            && Economy->GetCash() == CashBeforeBooking;
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=CHECKIN result=%s urgent=%d no_precharge=%d standard_duration=%.3f urgent_duration=%.3f expected_urgent=%.3f complete_day=%d complete_hour=%.2f"),
            bUrgentTiming ? TEXT("PASS") : TEXT("FAIL"), Active->bUrgent ? 1 : 0,
            Economy->GetCash() == CashBeforeBooking ? 1 : 0, ExpectedStandardDuration,
            ObservedUrgentDuration, ExpectedUrgentDuration, ServiceCompleteDay, ServiceCompleteHour);
        if (!bUrgentTiming)
        {
            MarkFailure(TEXT("urgent-timing-contract-failed")); FinishScenario(TEXT("checkin-failed")); return;
        }
        DayNight->RestoreTime(ServiceCompleteDay, ServiceCompleteHour);
        PhaseStartedAt = Elapsed;
        Phase = EPhase::AwaitCheckout;
        break;
    }

    case EPhase::AwaitCheckout:
    {
        if (Elapsed - PhaseStartedAt < QueueTickProofSeconds) break;
        const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
        const FGTTWorkshopRepairQueueSnapshot* Paid = Snapshots.FindByPredicate(
            [this](const FGTTWorkshopRepairQueueSnapshot& S){ return S.PersistentVehicleId == VehicleId; });
        const int32 Charged = CashBeforeBooking - Economy->GetCash();
        const FGTTRoadVehicleMigrationSnapshot After = Vehicle->GetMigrationSnapshot();
        bSingleDebit = Charged == UrgentQuote && Economy->GetCash() == CashBeforeBooking - UrgentQuote;
        bPickupPersisted = Paid && Paid->bReadyForPickup && Paid->State == TEXT("READY_FOR_PICKUP")
            && Paid->PaidAmount == UrgentQuote && VerifyPriorityCheckpoint(true);
        bPickupHoldObserved = Queue->IsVehicleAwaitingPickup(VehicleId);
        bIdentityPreserved = Vehicle->GetPersistentVehicleId() == VehicleId;
        const bool bRepairComplete = After.ConditionPercent >= 0.999f && After.TireIntegrity >= 0.999f
            && After.FuelLiters + 0.05f >= Vehicle->GetFuelCapacityLiters();
        const bool bPass = bSingleDebit && bPickupPersisted && bPickupHoldObserved
            && bIdentityPreserved && bRepairComplete;
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=CHECKOUT result=%s single_debit=%d charged=%d locked_quote=%d pickup_persisted=%d pickup_hold=%d repair_complete=%d identity_preserved=%d"),
            bPass ? TEXT("PASS") : TEXT("FAIL"), bSingleDebit ? 1 : 0, Charged, UrgentQuote,
            bPickupPersisted ? 1 : 0, bPickupHoldObserved ? 1 : 0, bRepairComplete ? 1 : 0,
            bIdentityPreserved ? 1 : 0);
        if (!bPass)
        {
            MarkFailure(TEXT("checkout-or-pickup-checkpoint-failed")); FinishScenario(TEXT("checkout-failed")); return;
        }
        Phase = EPhase::VerifyPickup;
        break;
    }

    case EPhase::VerifyPickup:
    {
        const int32 CashBeforePickup = Economy->GetCash();
        FString WrongSummary;
        const bool bWrongRelease = Queue->ReleaseCompletedRepairForPickup(DecoyVehicleId, WrongSummary);
        bWrongIdRejected = !bWrongRelease && Queue->IsVehicleAwaitingPickup(VehicleId);

        StageVehicle(Vehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 0.0f, 80.0f));
        FString PickupSummary;
        const bool bReleased = Queue->ReleaseCompletedRepairForPickup(VehicleId, PickupSummary);
        bExactPickupReleased = bReleased && !Queue->HasQueuedRepairForVehicle(VehicleId)
            && !Queue->IsVehicleAwaitingPickup(VehicleId) && Queue->GetQueuedRepairCount() == 0;
        bNoSecondCharge = Economy->GetCash() == CashBeforePickup;
        bIdentityPreserved = bIdentityPreserved && Vehicle->GetPersistentVehicleId() == VehicleId;
        bCargoContinuity = VerifyPrimaryCargoContinuity();
        const bool bPass = bWrongIdRejected && bExactPickupReleased && bNoSecondCharge
            && bIdentityPreserved && bCargoContinuity;
        UE_LOG(LogGTT, Log,
            TEXT("WORKSHOP_PRIORITY_PICKUP_RUNTIME phase=PICKUP result=%s wrong_id_rejected=%d exact_pickup=%d no_second_charge=%d identity_preserved=%d cargo_continuity=%d queue_remaining=%d"),
            bPass ? TEXT("PASS") : TEXT("FAIL"), bWrongIdRejected ? 1 : 0,
            bExactPickupReleased ? 1 : 0, bNoSecondCharge ? 1 : 0,
            bIdentityPreserved ? 1 : 0, bCargoContinuity ? 1 : 0, Queue->GetQueuedRepairCount());
        if (!bPass)
        {
            MarkFailure(TEXT("pickup-release-contract-failed")); FinishScenario(TEXT("pickup-failed")); return;
        }
        FinishScenario(TEXT("complete"));
        break;
    }

    case EPhase::Complete:
        break;
    }
}
