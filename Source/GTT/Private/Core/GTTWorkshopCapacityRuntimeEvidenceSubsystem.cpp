#include "Core/GTTWorkshopCapacityRuntimeEvidenceSubsystem.h"

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
constexpr float StartDelaySeconds = 438.0f;
constexpr float GlobalDeadlineSeconds = 468.0f;
constexpr float QueueTickProofSeconds = 1.65f;
constexpr float WorkshopStageOffsetCm = 140.0f;
constexpr float AwayOffsetCm = 2600.0f;
constexpr int32 MinimumEvidenceCash = 12000;
constexpr float ExpectedSpacingHours = 0.75f;
}

void UGTTWorkshopCapacityRuntimeEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTWorkshopCapacityRuntimeScenario"));
    if (bEnabled)
    {
        GTT_LOG( Log,
            TEXT("WORKSHOP_CAPACITY_RUNTIME_BEGIN version=1 route=two-bookings-disk-cancel-rebook-underfunded-nonblocking start_delay=%.1f deadline=%.1f capacity=4 spacing_hours=0.75 exact_vehicle=required no_precharge=required"),
            StartDelaySeconds, GlobalDeadlineSeconds);
    }
}

TStatId UGTTWorkshopCapacityRuntimeEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTWorkshopCapacityRuntimeEvidenceSubsystem, STATGROUP_Tickables);
}

bool UGTTWorkshopCapacityRuntimeEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

AGTTServiceTerminal* UGTTWorkshopCapacityRuntimeEvidenceSubsystem::FindWorkshopTerminal() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTServiceTerminal> It(World); It; ++It)
    {
        if (IsValid(*It) && It->GetServiceType() == EGTTServiceType::Workshop) return *It;
    }
    return nullptr;
}

bool UGTTWorkshopCapacityRuntimeEvidenceSubsystem::FindEvidenceVehicles()
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
    FirstVehicle = Candidates[0];
    SecondVehicle = Candidates[1];
    return true;
}

bool UGTTWorkshopCapacityRuntimeEvidenceSubsystem::ResolveActors()
{
    UWorld* World = GetWorld();
    if (!World) return false;
    if (!Workshop.IsValid()) Workshop = FindWorkshopTerminal();
    if (!Queue.IsValid()) Queue = World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>();
    if (!DayNight.IsValid())
    {
        if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) DayNight = GameMode->GetDayNightCycle();
    }
    if (!FirstVehicle.IsValid() || !SecondVehicle.IsValid()) FindEvidenceVehicles();
    if (!Economy.IsValid())
    {
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
        {
            if (AGTTRoadVehicleNativePawn* Native = Cast<AGTTRoadVehicleNativePawn>(PlayerPawn)) PlayerPawn = Native->GetDriverPawn();
            if (PlayerPawn) Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
        }
    }
    return Workshop.IsValid() && Queue.IsValid() && DayNight.IsValid()
        && FirstVehicle.IsValid() && SecondVehicle.IsValid() && Economy.IsValid();
}

void UGTTWorkshopCapacityRuntimeEvidenceSubsystem::StageVehicle(AGTTRoadVehicleNativePawn* Target, const FVector& Location) const
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

void UGTTWorkshopCapacityRuntimeEvidenceSubsystem::StageDamage(AGTTRoadVehicleNativePawn* Target, bool bSevere) const
{
    if (!IsValid(Target)) return;
    FGTTRoadVehicleMigrationSnapshot State = Target->GetMigrationSnapshot();
    State.ConditionPercent = bSevere ? 0.34f : 0.82f;
    State.TireIntegrity = bSevere ? 0.42f : 0.84f;
    State.FuelLiters = FMath::Min(State.FuelLiters, Target->GetFuelCapacityLiters() * (bSevere ? 0.22f : 0.70f));
    Target->RestorePersistentMigrationSnapshot(State);
}

bool UGTTWorkshopCapacityRuntimeEvidenceSubsystem::CapturePrimaryCargoBaseline()
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

bool UGTTWorkshopCapacityRuntimeEvidenceSubsystem::VerifyPrimaryCargoContinuity() const
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

void UGTTWorkshopCapacityRuntimeEvidenceSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    GTT_LOG( Error, TEXT("WORKSHOP_CAPACITY_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTWorkshopCapacityRuntimeEvidenceSubsystem::RestoreBaselineState()
{
    UWorld* World = GetWorld();
    if (!World || !bBaselineCaptured) return;
    if (Queue.IsValid())
    {
        const TArray<FGTTWorkshopRepairQueueSnapshot> Remaining = Queue->GetQueueSnapshots();
        for (const FGTTWorkshopRepairQueueSnapshot& Entry : Remaining)
        {
            FString Summary;
            Queue->CancelQueuedRepair(Entry.PersistentVehicleId, Summary);
        }
    }
    if (FirstVehicle.IsValid())
    {
        FirstVehicle->RestorePersistentMigrationSnapshot(FirstBaselineMigration);
        FirstVehicle->RestorePersistentBodyDamage(FirstBaselineBodyDamage, FirstBaselineDetachedMask);
        FirstVehicle->SetActorTransform(FirstBaselineTransform, false, nullptr, ETeleportType::ResetPhysics);
    }
    if (SecondVehicle.IsValid())
    {
        SecondVehicle->RestorePersistentMigrationSnapshot(SecondBaselineMigration);
        SecondVehicle->RestorePersistentBodyDamage(SecondBaselineBodyDamage, SecondBaselineDetachedMask);
        SecondVehicle->SetActorTransform(SecondBaselineTransform, false, nullptr, ETeleportType::ResetPhysics);
    }
    if (Economy.IsValid()) Economy->RestoreState(BaselineCash, BaselineFishCount, BaselineFishWeightKg);
    if (DayNight.IsValid()) DayNight->RestoreTime(BaselineDay, BaselineHour);
    if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) GameMode->SaveProgress();
}

void UGTTWorkshopCapacityRuntimeEvidenceSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bPass = bSequenceHealthy && bTwoBookingsAccepted && bNoPrecharge && bCapacitySpacing
        && bDiskRoundtrip && bIndependentCancel && bRebookPreserved && bUnderfundedNonBlocking
        && bLaterSingleDebit && bEarlierReservationPreserved && bLaterExactService
        && bIdentityPreserved && bCargoContinuity && FirstLockedQuote > SecondLockedQuote
        && SecondLockedQuote > 0 && !FirstVehicleId.IsNone() && !SecondVehicleId.IsNone();

    GTT_LOG( Log,
        TEXT("WORKSHOP_CAPACITY_RUNTIME_COMPLETE result=%s two_bookings=%d no_precharge=%d spacing_45m=%d disk_roundtrip=%d independent_cancel=%d rebook_preserved=%d underfunded_nonblocking=%d later_single_debit=%d earlier_preserved=%d later_exact_service=%d identity_preserved=%d cargo_continuity=%d first_quote=%d second_quote=%d first_vehicle=%s second_vehicle=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bTwoBookingsAccepted ? 1 : 0, bNoPrecharge ? 1 : 0,
        bCapacitySpacing ? 1 : 0, bDiskRoundtrip ? 1 : 0, bIndependentCancel ? 1 : 0,
        bRebookPreserved ? 1 : 0, bUnderfundedNonBlocking ? 1 : 0, bLaterSingleDebit ? 1 : 0,
        bEarlierReservationPreserved ? 1 : 0, bLaterExactService ? 1 : 0, bIdentityPreserved ? 1 : 0,
        bCargoContinuity ? 1 : 0, FirstLockedQuote, SecondLockedQuote,
        *FirstVehicleId.ToString(), *SecondVehicleId.ToString(), Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = EPhase::Complete;
    bFinished = true;
}

void UGTTWorkshopCapacityRuntimeEvidenceSubsystem::Tick(float DeltaTime)
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
            MarkFailure(TEXT("capacity-world-actors-unavailable")); FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    switch (Phase)
    {
    case EPhase::Waiting:
        Phase = EPhase::PrepareBookings;
        break;

    case EPhase::PrepareBookings:
    {
        if (Queue->GetQueuedRepairCount() != 0)
        {
            MarkFailure(TEXT("queue-not-empty-at-start")); FinishScenario(TEXT("prepare-failed")); return;
        }
        FirstVehicleId = FirstVehicle->GetPersistentVehicleId();
        SecondVehicleId = SecondVehicle->GetPersistentVehicleId();
        BaselineDay = DayNight->GetDayNumber();
        BaselineHour = DayNight->GetTimeOfDayHours();
        BaselineCash = Economy->GetCash();
        BaselineFishCount = Economy->GetFishCount();
        BaselineFishWeightKg = Economy->GetFishWeightKg();
        FirstBaselineMigration = FirstVehicle->GetMigrationSnapshot();
        SecondBaselineMigration = SecondVehicle->GetMigrationSnapshot();
        FirstBaselineBodyDamage = FirstVehicle->GetBodyDamageSnapshot();
        SecondBaselineBodyDamage = SecondVehicle->GetBodyDamageSnapshot();
        FirstBaselineDetachedMask = FirstVehicle->GetDetachedPanelMask();
        SecondBaselineDetachedMask = SecondVehicle->GetDetachedPanelMask();
        FirstBaselineTransform = FirstVehicle->GetActorTransform();
        SecondBaselineTransform = SecondVehicle->GetActorTransform();
        bCargoBaselineCaptured = CapturePrimaryCargoBaseline();
        bBaselineCaptured = true;
        if (!bCargoBaselineCaptured || bBaselineCargoActive)
        {
            MarkFailure(bBaselineCargoActive ? TEXT("active-farm-cargo-correctly-forbids-multi-vehicle-booking") : TEXT("primary-save-missing"));
            FinishScenario(TEXT("prepare-failed")); return;
        }

        Economy->RestoreState(FMath::Max(BaselineCash, MinimumEvidenceCash), BaselineFishCount, BaselineFishWeightKg);
        DayNight->RestoreTime(BaselineDay, GTTWorkshopHoursPolicy::ClosingHour + 0.35f);
        StageDamage(FirstVehicle.Get(), true);
        StageDamage(SecondVehicle.Get(), false);
        CashBeforeBookings = Economy->GetCash();

        StageVehicle(FirstVehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 0.0f, 80.0f));
        StageVehicle(SecondVehicle.Get(), Workshop->GetActorLocation() + FVector(AwayOffsetCm, 900.0f, 80.0f));
        FString FirstSummary;
        const bool bFirstAccepted = Queue->TryQueueNearestEligibleNativeRoadVehicle(Workshop->GetActorLocation(), 900.0f, FirstSummary);

        StageVehicle(FirstVehicle.Get(), Workshop->GetActorLocation() + FVector(AwayOffsetCm, -900.0f, 80.0f));
        StageVehicle(SecondVehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 180.0f, 80.0f));
        FString SecondSummary;
        const bool bSecondAccepted = Queue->TryQueueNearestEligibleNativeRoadVehicle(Workshop->GetActorLocation(), 900.0f, SecondSummary);

        const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
        const FGTTWorkshopRepairQueueSnapshot* First = Snapshots.FindByPredicate([this](const FGTTWorkshopRepairQueueSnapshot& S){ return S.PersistentVehicleId == FirstVehicleId; });
        const FGTTWorkshopRepairQueueSnapshot* Second = Snapshots.FindByPredicate([this](const FGTTWorkshopRepairQueueSnapshot& S){ return S.PersistentVehicleId == SecondVehicleId; });
        if (First) { FirstLockedQuote = First->LockedQuote; FirstReadyDay = First->ReadyDay; FirstReadyHour = First->ReadyHour; }
        if (Second) { SecondLockedQuote = Second->LockedQuote; SecondReadyDay = Second->ReadyDay; SecondReadyHour = Second->ReadyHour; }
        bTwoBookingsAccepted = bFirstAccepted && bSecondAccepted && Snapshots.Num() == 2 && First && Second;
        bNoPrecharge = Economy->GetCash() == CashBeforeBookings;
        bCapacitySpacing = First && Second && FirstReadyDay == SecondReadyDay
            && FMath::IsNearlyEqual(SecondReadyHour - FirstReadyHour, ExpectedSpacingHours, 0.001f);
        const bool bQuotesOrdered = FirstLockedQuote > SecondLockedQuote && SecondLockedQuote > 0;
        const bool bPass = bTwoBookingsAccepted && bNoPrecharge && bCapacitySpacing && bQuotesOrdered;
        GTT_LOG( Log,
            TEXT("WORKSHOP_CAPACITY_RUNTIME phase=BOOK_TWO result=%s count=%d no_precharge=%d spacing_45m=%d quote_order=%d first_quote=%d second_quote=%d first_ready=%.2f second_ready=%.2f first=%s second=%s"),
            bPass ? TEXT("PASS") : TEXT("FAIL"), Snapshots.Num(), bNoPrecharge ? 1 : 0,
            bCapacitySpacing ? 1 : 0, bQuotesOrdered ? 1 : 0, FirstLockedQuote, SecondLockedQuote,
            FirstReadyHour, SecondReadyHour, *FirstVehicleId.ToString(), *SecondVehicleId.ToString());
        if (!bPass) { MarkFailure(TEXT("two-booking-contract-failed")); FinishScenario(TEXT("book-failed")); return; }
        Phase = EPhase::VerifyDiskAndCancel;
        break;
    }

    case EPhase::VerifyDiskAndCancel:
    {
        const UGTTWorkshopQueueSaveGame* Save = Cast<UGTTWorkshopQueueSaveGame>(UGameplayStatics::LoadGameFromSlot(QueueSlot, SaveUserIndex));
        bool bFirstOnDisk = false;
        bool bSecondOnDisk = false;
        if (Save && Save->SchemaVersion == 1)
        {
            for (const FGTTWorkshopQueueSaveEntry& Entry : Save->Appointments)
            {
                if (Entry.PersistentVehicleId == FirstVehicleId && Entry.LockedQuote == FirstLockedQuote) bFirstOnDisk = true;
                if (Entry.PersistentVehicleId == SecondVehicleId && Entry.LockedQuote == SecondLockedQuote) bSecondOnDisk = true;
            }
        }
        bDiskRoundtrip = Save && Save->Appointments.Num() == 2 && bFirstOnDisk && bSecondOnDisk;
        FString CancelSummary;
        const bool bCancelled = Queue->CancelQueuedRepair(SecondVehicleId, CancelSummary);
        const bool bFirstStillQueued = Queue->HasQueuedRepairForVehicle(FirstVehicleId);
        const bool bSecondGone = !Queue->HasQueuedRepairForVehicle(SecondVehicleId);
        bIndependentCancel = bCancelled && bFirstStillQueued && bSecondGone && Queue->GetQueuedRepairCount() == 1
            && Economy->GetCash() == CashBeforeBookings;
        GTT_LOG( Log,
            TEXT("WORKSHOP_CAPACITY_RUNTIME phase=DISK_CANCEL result=%s disk_roundtrip=%d independent_cancel=%d first_preserved=%d second_removed=%d no_charge=%d"),
            (bDiskRoundtrip && bIndependentCancel) ? TEXT("PASS") : TEXT("FAIL"), bDiskRoundtrip ? 1 : 0,
            bIndependentCancel ? 1 : 0, bFirstStillQueued ? 1 : 0, bSecondGone ? 1 : 0,
            Economy->GetCash() == CashBeforeBookings ? 1 : 0);
        if (!bDiskRoundtrip || !bIndependentCancel)
        {
            MarkFailure(TEXT("disk-or-independent-cancel-failed")); FinishScenario(TEXT("cancel-failed")); return;
        }
        Phase = EPhase::VerifyRebook;
        break;
    }

    case EPhase::VerifyRebook:
    {
        StageVehicle(FirstVehicle.Get(), Workshop->GetActorLocation() + FVector(AwayOffsetCm, -900.0f, 80.0f));
        StageVehicle(SecondVehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 180.0f, 80.0f));
        FString RebookSummary;
        const bool bRebooked = Queue->TryQueueNearestEligibleNativeRoadVehicle(Workshop->GetActorLocation(), 900.0f, RebookSummary);
        const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
        const FGTTWorkshopRepairQueueSnapshot* Second = Snapshots.FindByPredicate([this](const FGTTWorkshopRepairQueueSnapshot& S){ return S.PersistentVehicleId == SecondVehicleId; });
        bRebookPreserved = bRebooked && Snapshots.Num() == 2 && Queue->HasQueuedRepairForVehicle(FirstVehicleId)
            && Second && Second->LockedQuote == SecondLockedQuote && Second->ReadyDay == FirstReadyDay
            && FMath::IsNearlyEqual(Second->ReadyHour - FirstReadyHour, ExpectedSpacingHours, 0.001f)
            && Economy->GetCash() == CashBeforeBookings;
        if (Second) { SecondReadyDay = Second->ReadyDay; SecondReadyHour = Second->ReadyHour; }
        GTT_LOG( Log,
            TEXT("WORKSHOP_CAPACITY_RUNTIME phase=REBOOK result=%s rebooked=%d count=%d exact_second=%d locked_quote_preserved=%d spacing_45m=%d no_precharge=%d"),
            bRebookPreserved ? TEXT("PASS") : TEXT("FAIL"), bRebooked ? 1 : 0, Snapshots.Num(),
            Second && Second->PersistentVehicleId == SecondVehicleId ? 1 : 0,
            Second && Second->LockedQuote == SecondLockedQuote ? 1 : 0,
            Second && FMath::IsNearlyEqual(Second->ReadyHour - FirstReadyHour, ExpectedSpacingHours, 0.001f) ? 1 : 0,
            Economy->GetCash() == CashBeforeBookings ? 1 : 0);
        if (!bRebookPreserved)
        {
            MarkFailure(TEXT("rebook-contract-failed")); FinishScenario(TEXT("rebook-failed")); return;
        }

        Economy->RestoreState(SecondLockedQuote, BaselineFishCount, BaselineFishWeightKg);
        StageVehicle(FirstVehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, -160.0f, 80.0f));
        StageVehicle(SecondVehicle.Get(), Workshop->GetActorLocation() + FVector(WorkshopStageOffsetCm, 160.0f, 80.0f));
        DayNight->RestoreTime(SecondReadyDay, SecondReadyHour);
        PhaseStartedAt = Elapsed;
        Phase = EPhase::AwaitCapacityExecution;
        break;
    }

    case EPhase::AwaitCapacityExecution:
    {
        if (Elapsed - PhaseStartedAt < QueueTickProofSeconds) break;
        const FGTTRoadVehicleMigrationSnapshot FirstAfter = FirstVehicle->GetMigrationSnapshot();
        const FGTTRoadVehicleMigrationSnapshot SecondAfter = SecondVehicle->GetMigrationSnapshot();
        const int32 Charged = SecondLockedQuote - Economy->GetCash();
        bEarlierReservationPreserved = Queue->HasQueuedRepairForVehicle(FirstVehicleId)
            && !Queue->HasQueuedRepairForVehicle(SecondVehicleId) && Queue->GetQueuedRepairCount() == 1;
        bUnderfundedNonBlocking = bEarlierReservationPreserved && FirstLockedQuote > SecondLockedQuote;
        bLaterSingleDebit = Charged == SecondLockedQuote && Economy->GetCash() == 0;
        bLaterExactService = SecondAfter.ConditionPercent >= 0.999f && SecondAfter.TireIntegrity >= 0.999f
            && SecondAfter.FuelLiters + 0.05f >= SecondVehicle->GetFuelCapacityLiters()
            && FirstAfter.ConditionPercent < 0.90f;
        bIdentityPreserved = FirstVehicle->GetPersistentVehicleId() == FirstVehicleId
            && SecondVehicle->GetPersistentVehicleId() == SecondVehicleId;
        bCargoContinuity = VerifyPrimaryCargoContinuity();
        const bool bPass = bUnderfundedNonBlocking && bLaterSingleDebit && bLaterExactService
            && bIdentityPreserved && bCargoContinuity;
        GTT_LOG( Log,
            TEXT("WORKSHOP_CAPACITY_RUNTIME phase=EXECUTE result=%s underfunded_nonblocking=%d earlier_preserved=%d later_single_debit=%d charged=%d second_quote=%d later_exact_service=%d identity_preserved=%d cargo_continuity=%d remaining=%d"),
            bPass ? TEXT("PASS") : TEXT("FAIL"), bUnderfundedNonBlocking ? 1 : 0,
            bEarlierReservationPreserved ? 1 : 0, bLaterSingleDebit ? 1 : 0, Charged, SecondLockedQuote,
            bLaterExactService ? 1 : 0, bIdentityPreserved ? 1 : 0, bCargoContinuity ? 1 : 0,
            Queue->GetQueuedRepairCount());
        if (!bPass)
        {
            MarkFailure(TEXT("capacity-execution-contract-failed")); FinishScenario(TEXT("execution-failed")); return;
        }
        FinishScenario(TEXT("complete"));
        break;
    }

    case EPhase::Complete:
        break;
    }
}
