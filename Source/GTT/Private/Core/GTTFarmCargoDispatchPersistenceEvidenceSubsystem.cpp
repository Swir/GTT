#include "Core/GTTFarmCargoDispatchPersistenceEvidenceSubsystem.h"

#include "Activities/GTTFarmCargoAuthoritySubsystem.h"
#include "Activities/GTTFarmJobDirector.h"
#include "Activities/GTTFarmJobTerminal.h"
#include "Components/PrimitiveComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Save/GTTSaveGame.h"
#include "Save/GTTRoadsideDispatchSaveGame.h"
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTFarmVanPawn.h"
#include "Vehicles/GTTRoadsideDispatchPersistenceSubsystem.h"
#include "Vehicles/GTTRoadsideRecoverySubsystem.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTLogisticsReputationSubsystem.h"
#include "GTT.h"

namespace
{
constexpr float StartDelaySeconds = 326.0f;
constexpr float GlobalDeadlineSeconds = 350.0f;
constexpr float CheckpointWaitSeconds = 0.55f;
constexpr float RestoreObservationSeconds = 0.65f;
constexpr float HandoffParkingOffsetCm = 120.0f;
constexpr float ExactVehicleFarOffsetCm = 1500.0f;
constexpr int32 MinimumEvidenceRouteTier = 2;
constexpr int32 MinimumEvidenceCash = 4000;
constexpr float WantedEvidenceHeat = 35.0f;
const FString DispatchCheckpointSlot(TEXT("GTT_RoadsideDispatch_01"));

const TCHAR* StageLabel(EGTTFarmJobStage Stage)
{
    switch (Stage)
    {
        case EGTTFarmJobStage::Idle: return TEXT("Idle");
        case EGTTFarmJobStage::ReachPickup: return TEXT("ReachPickup");
        case EGTTFarmJobStage::DeliverCargo: return TEXT("DeliverCargo");
        case EGTTFarmJobStage::DeliverFinalStop: return TEXT("DeliverFinalStop");
        default: return TEXT("Unknown");
    }
}
}

void UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRuntimeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRecoveryScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoBreakdownScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoDispatchScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoDispatchPersistenceScenario"));
    if (bEnabled)
    {
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME_BEGIN version=1 route=feed-tow-reload-wanted-reject-patch-reload-hill-wood start_delay=%.1f deadline=%.1f savegame_roundtrip=required exact_vehicle=required locked_quote=required eta=required single_charge=required"),
            StartDelaySeconds, GlobalDeadlineSeconds);
    }
}

TStatId UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmCargoDispatchPersistenceEvidenceSubsystem, STATGROUP_Tickables);
}

bool UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

APawn* UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::ResolvePlayerPawn() const
{
    const UWorld* World = GetWorld();
    if (!World) return nullptr;
    const APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return nullptr;
    APawn* Controlled = PC->GetPawn();
    if (AGTTRoadVehicleNativePawn* Native = Cast<AGTTRoadVehicleNativePawn>(Controlled)) return Native->GetDriverPawn();
    return Controlled;
}

AGTTFarmJobTerminal* UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::FindTerminal(uint8 TerminalTypeValue) const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    const EGTTFarmJobTerminalType Desired = static_cast<EGTTFarmJobTerminalType>(TerminalTypeValue);
    for (TActorIterator<AGTTFarmJobTerminal> It(World); It; ++It)
    {
        if (It->GetTerminalType() == Desired) return *It;
    }
    return nullptr;
}

AGTTMuleboxNativePawn* UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::FindNativeMulebox() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTMuleboxNativePawn> It(World); It; ++It)
    {
        if (It->IsNativeReady()) return *It;
    }
    return nullptr;
}

AGTTFarmVanPawn* UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::SpawnDecoyVan(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    FActorSpawnParameters Params;
    Params.Name = MakeUniqueObjectName(World, AGTTFarmVanPawn::StaticClass(), FName(TEXT("GTTFarmCargoDispatchPersistenceDecoy")));
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return World->SpawnActor<AGTTFarmVanPawn>(Location, FRotator::ZeroRotator, Params);
}

void UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::StageActor(
    AActor* Actor,
    const FVector& Location,
    const FRotator& Rotation) const
{
    if (!Actor) return;
    Actor->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    Actor->SetActorRotation(Rotation, ETeleportType::TeleportPhysics);
    if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
    {
        if (RootPrimitive->IsSimulatingPhysics())
        {
            RootPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
            RootPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        }
    }
}

bool UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::EnsureNativeDriver()
{
    UWorld* World = GetWorld();
    if (!World || !NativeMulebox.IsValid() || !PlayerPawn.IsValid()) return false;
    if (UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()) return true;
    if (!NativeMulebox->IsLegacyTakeoverActive() && !NativeMulebox->TryActivateLegacyTakeover()) return false;
    NativeMulebox->Interact_Implementation(PlayerPawn.Get());
    return UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()
        && NativeMulebox->GetDriverPawn() == PlayerPawn.Get();
}

void UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::StageTowRecommendedState()
{
    if (!NativeMulebox.IsValid()) return;
    FGTTRoadVehicleMigrationSnapshot Staged = NativeMulebox->GetMigrationSnapshot();
    Staged.ConditionPercent = FMath::Min(Staged.ConditionPercent, 0.19f);
    Staged.TireIntegrity = FMath::Min(Staged.TireIntegrity, 0.20f);
    Staged.FuelLiters = FMath::Max(Staged.FuelLiters, FMath::Min(NativeMulebox->GetFuelCapacityLiters(), 6.0f));
    NativeMulebox->RestorePersistentMigrationSnapshot(Staged);
}

bool UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::ReadDispatchCheckpoint(
    uint8 ExpectedMode,
    int32 ExpectedQuote,
    float& OutEta) const
{
    OutEta = 0.0f;
    const UGTTRoadsideDispatchSaveGame* Save = Cast<UGTTRoadsideDispatchSaveGame>(
        UGameplayStatics::LoadGameFromSlot(DispatchCheckpointSlot, 0));
    if (!Save) return false;
    OutEta = Save->SecondsRemaining;
    return Save->SchemaVersion == 2
        && Save->bDispatchPending
        && Save->RecoveryMode == ExpectedMode
        && Save->PersistentVehicleId == LoadedVehicleId
        && Save->LockedQuote == ExpectedQuote
        && Save->LockedQuote > 0
        && Save->SecondsRemaining > 0.0f
        && Save->AuthorizedCash >= 0;
}

bool UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::ResolveScenarioActors()
{
    UWorld* World = GetWorld();
    if (!World) return false;
    if (!Director.IsValid()) Director = Cast<AGTTFarmJobDirector>(UGameplayStatics::GetActorOfClass(World, AGTTFarmJobDirector::StaticClass()));
    if (!PlayerPawn.IsValid()) PlayerPawn = ResolvePlayerPawn();
    if (!StartTerminal.IsValid()) StartTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::Start));
    if (!PickupTerminal.IsValid()) PickupTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::Pickup));
    if (!HillTerminal.IsValid()) HillTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::Finish));
    if (!FinalTerminal.IsValid()) FinalTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::FinalFinish));
    if (!NativeMulebox.IsValid()) NativeMulebox = FindNativeMulebox();
    if (!Authority.IsValid()) Authority = World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>();
    if (!Breakdown.IsValid()) Breakdown = World->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    if (!Roadside.IsValid()) Roadside = World->GetSubsystem<UGTTRoadsideRecoverySubsystem>();
    if (!DispatchPersistence.IsValid()) DispatchPersistence = World->GetSubsystem<UGTTRoadsideDispatchPersistenceSubsystem>();
    if (!Logistics.IsValid()) Logistics = World->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    if (!Economy.IsValid() && PlayerPawn.IsValid()) Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn.Get());
    if (!Wanted.IsValid() && PlayerPawn.IsValid()) Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn.Get());
    if (!DayNight.IsValid())
    {
        if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) DayNight = GameMode->GetDayNightCycle();
    }
    return Director.IsValid() && PlayerPawn.IsValid() && StartTerminal.IsValid() && PickupTerminal.IsValid()
        && HillTerminal.IsValid() && FinalTerminal.IsValid() && NativeMulebox.IsValid() && Authority.IsValid()
        && Breakdown.IsValid() && Roadside.IsValid() && DispatchPersistence.IsValid() && Logistics.IsValid()
        && Economy.IsValid() && Wanted.IsValid() && DayNight.IsValid();
}

void UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    UE_LOG(LogGTT, Error,
        TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::RestoreBaselineState()
{
    UWorld* World = GetWorld();
    if (!World || !bBaselineCaptured) return;
    if (Roadside.IsValid() && NativeMulebox.IsValid() && Roadside->HasPendingRoadsideService(NativeMulebox.Get()))
    {
        Roadside->CancelPendingRoadsideService(NativeMulebox.Get());
    }
    if (NativeMulebox.IsValid())
    {
        if (UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()) NativeMulebox->ExitNativeVehicle();
        NativeMulebox->RestorePersistentMigrationSnapshot(BaselineMigration);
        NativeMulebox->RestorePersistentBodyDamage(BaselineBodyDamage, BaselineDetachedMask);
        NativeMulebox->SetCargoLoadFactor(0.0f);
        NativeMulebox->SetActorTransform(BaselineNativeTransform, false, nullptr, ETeleportType::ResetPhysics);
    }
    if (SpawnedDecoyVan.IsValid()) SpawnedDecoyVan->Destroy();
    if (Authority.IsValid())
    {
        Authority->ClearLoadedVehicle(TEXT("dispatch-persistence-runtime-evidence-cleanup"));
        if (BaselineLogisticsSave) Authority->RestoreFromSave(BaselineLogisticsSave);
    }
    if (Logistics.IsValid() && BaselineLogisticsSave) Logistics->RestoreFromSave(BaselineLogisticsSave);
    if (Director.IsValid() && BaselineLogisticsSave) Director->RestoreActiveCargoFromSave(BaselineLogisticsSave);
    if (Economy.IsValid()) Economy->RestoreState(BaselineCash, BaselineFishCount, BaselineFishWeightKg);
    if (DayNight.IsValid()) DayNight->RestoreTime(BaselineDay, BaselineHour);
    if (Wanted.IsValid())
    {
        Wanted->ClearWanted();
        if (BaselineWantedHeat > 0.0f) Wanted->AddHeat(BaselineWantedHeat);
    }
    if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) GameMode->SaveProgress();
}

void UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bAuthorityCleared = Authority.IsValid() && !Authority->HasBoundCargoVehicle()
        && Authority->GetBoundCargoVehicleId().IsNone();
    const bool bPass = bSequenceHealthy && bAccepted && bPickupBound
        && bTowCheckpointSaved && bTowPrimarySaved && bTowPrimaryLoaded && bTowReloadRearmed
        && bTowRestored && bTowQuotePreserved && bTowEtaPreserved && bTowExactVehicle && bTowCancelledNoCharge
        && bWantedCheckpointSaved && bWantedPrimarySaved && bWantedPrimaryLoaded && bWantedRestoreRearmed
        && bWantedRestoreRejectedNoCharge
        && bPatchCheckpointSaved && bPatchPrimarySaved && bPatchPrimaryLoaded && bPatchReloadRearmed
        && bPatchRestored && bPatchQuotePreserved && bPatchEtaPreserved && bPatchExactVehicle
        && bPatchNoChargeBeforeArrival && bPatchCompleted && bPatchSingleCharge
        && bTimerContinued && bIntegrityNotImproved && bWrongVehicleRejected
        && bHillHandoff && bFinalHandoff && bSaveVerified && bAuthorityCleared
        && TowLockedQuote > 0 && WantedTowLockedQuote > 0 && PatchLockedQuote > 0
        && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0;

    UE_LOG(LogGTT, Log,
        TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME_COMPLETE result=%s route=feed-tow-reload-wanted-reject-patch-reload-hill-wood accepted=%d pickup=%d tow_checkpoint=%d tow_primary_save=%d tow_primary_load=%d tow_rearm=%d tow_restored=%d tow_quote_preserved=%d tow_eta_preserved=%d tow_exact_vehicle=%d tow_cancel_no_charge=%d wanted_checkpoint=%d wanted_primary_save=%d wanted_primary_load=%d wanted_rearm=%d wanted_rejected_no_charge=%d patch_checkpoint=%d patch_primary_save=%d patch_primary_load=%d patch_rearm=%d patch_restored=%d patch_quote_preserved=%d patch_eta_preserved=%d patch_exact_vehicle=%d patch_no_charge_before_arrival=%d patch_completed=%d patch_single_charge=%d timer_continued=%d integrity_not_improved=%d wrong_vehicle_rejected=%d hill=%d final=%d save=%d authority_cleared=%d tow_quote=%d wanted_tow_quote=%d patch_quote=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d vehicle=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bAccepted ? 1 : 0, bPickupBound ? 1 : 0,
        bTowCheckpointSaved ? 1 : 0, bTowPrimarySaved ? 1 : 0, bTowPrimaryLoaded ? 1 : 0,
        bTowReloadRearmed ? 1 : 0, bTowRestored ? 1 : 0, bTowQuotePreserved ? 1 : 0,
        bTowEtaPreserved ? 1 : 0, bTowExactVehicle ? 1 : 0, bTowCancelledNoCharge ? 1 : 0,
        bWantedCheckpointSaved ? 1 : 0, bWantedPrimarySaved ? 1 : 0, bWantedPrimaryLoaded ? 1 : 0,
        bWantedRestoreRearmed ? 1 : 0, bWantedRestoreRejectedNoCharge ? 1 : 0,
        bPatchCheckpointSaved ? 1 : 0, bPatchPrimarySaved ? 1 : 0, bPatchPrimaryLoaded ? 1 : 0,
        bPatchReloadRearmed ? 1 : 0, bPatchRestored ? 1 : 0, bPatchQuotePreserved ? 1 : 0,
        bPatchEtaPreserved ? 1 : 0, bPatchExactVehicle ? 1 : 0, bPatchNoChargeBeforeArrival ? 1 : 0,
        bPatchCompleted ? 1 : 0, bPatchSingleCharge ? 1 : 0, bTimerContinued ? 1 : 0,
        bIntegrityNotImproved ? 1 : 0, bWrongVehicleRejected ? 1 : 0, bHillHandoff ? 1 : 0,
        bFinalHandoff ? 1 : 0, bSaveVerified ? 1 : 0, bAuthorityCleared ? 1 : 0,
        TowLockedQuote, WantedTowLockedQuote, PatchLockedQuote, PayoutDelta, CargoRunsDelta, ReputationDelta,
        *LoadedVehicleId.ToString(), Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = EPersistenceEvidencePhase::Complete;
    bFinished = true;
}

void UGTTFarmCargoDispatchPersistenceEvidenceSubsystem::Tick(float DeltaTime)
{
    Elapsed += DeltaTime;
    if (Elapsed < StartDelaySeconds) return;
    if (Elapsed >= GlobalDeadlineSeconds)
    {
        MarkFailure(TEXT("global-sequence-timeout"));
        FinishScenario(TEXT("deadline"));
        return;
    }
    if (!ResolveScenarioActors())
    {
        if (Elapsed > StartDelaySeconds + 4.0f)
        {
            MarkFailure(TEXT("dispatch-persistence-world-actors-unavailable"));
            FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    UWorld* World = GetWorld();
    AGTTGameMode* GameMode = World ? World->GetAuthGameMode<AGTTGameMode>() : nullptr;

    switch (Phase)
    {
    case EPersistenceEvidencePhase::Waiting:
        Phase = EPersistenceEvidencePhase::Prepare;
        break;

    case EPersistenceEvidencePhase::Prepare:
    {
        if (!GameMode || Director->GetStage() != EGTTFarmJobStage::Idle || GameMode->GetWildlifeAlertLevel() > 0)
        {
            MarkFailure(TEXT("farm-director-or-legal-work-state-not-clean"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }
        BaselineLogisticsSave = NewObject<UGTTSaveGame>(this);
        Logistics->CaptureToSave(BaselineLogisticsSave);
        Director->CaptureActiveCargoToSave(BaselineLogisticsSave);
        Authority->CaptureToSave(BaselineLogisticsSave);
        BaselineDay = DayNight->GetDayNumber();
        BaselineHour = DayNight->GetTimeOfDayHours();
        BaselineWantedHeat = Wanted->GetHeat();
        BaselineCash = Economy->GetCash();
        BaselineFishCount = Economy->GetFishCount();
        BaselineFishWeightKg = Economy->GetFishWeightKg();
        BaselineMigration = NativeMulebox->GetMigrationSnapshot();
        BaselineBodyDamage = NativeMulebox->GetBodyDamageSnapshot();
        BaselineDetachedMask = NativeMulebox->GetDetachedPanelMask();
        BaselineNativeTransform = NativeMulebox->GetActorTransform();
        bBaselineCaptured = true;

        DayNight->RestoreTime(BaselineDay, 9.0f);
        Wanted->ClearWanted();
        Economy->RestoreState(FMath::Max(BaselineCash, MinimumEvidenceCash), BaselineFishCount, BaselineFishWeightKg);
        int32 SeedRuns = 0;
        while (Logistics->GetCargoRouteTier() < MinimumEvidenceRouteTier && SeedRuns < 3)
        {
            Logistics->RecordCargoSuccess(0, 1.0f, true, false, true);
            ++SeedRuns;
        }
        if (!Logistics->IsCargoDepotWindowOpen() || !Logistics->CanAcceptCargoContract()
            || Logistics->GetActiveCargoOrderTier() < MinimumEvidenceRouteTier)
        {
            MarkFailure(TEXT("tier2-stock-backed-persistence-route-unavailable"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }
        if (!NativeMulebox->IsLegacyTakeoverActive() && !NativeMulebox->TryActivateLegacyTakeover())
        {
            MarkFailure(TEXT("native-mulebox-takeover-unavailable"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }
        SpawnedDecoyVan = SpawnDecoyVan(PlayerPawn->GetActorLocation() + FVector(500.0f, 0.0f, 80.0f));
        if (!SpawnedDecoyVan.IsValid())
        {
            MarkFailure(TEXT("decoy-vehicle-spawn-failed"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }
        EvidenceCargoRunsBefore = Logistics->GetCargoCompletedRuns();
        EvidenceReputationBefore = Logistics->GetReputation();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=PREPARE result=PASS route_tier=%d cash_seeded=%d"),
            Logistics->GetCargoRouteTier(), Economy->GetCash());
        Phase = EPersistenceEvidencePhase::AcceptContract;
        break;
    }

    case EPersistenceEvidencePhase::AcceptContract:
        StartTerminal->Interact_Implementation(PlayerPawn.Get());
        bAccepted = Director->GetStage() == EGTTFarmJobStage::ReachPickup;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=ACCEPT result=%s stage=%s"),
            bAccepted ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()));
        if (!bAccepted) { MarkFailure(TEXT("contract-acceptance-failed")); FinishScenario(TEXT("accept-failed")); return; }
        Phase = EPersistenceEvidencePhase::EnterAndPickup;
        break;

    case EPersistenceEvidencePhase::EnterAndPickup:
    {
        StageActor(NativeMulebox.Get(), PickupTerminal->GetActorLocation() + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        if (!EnsureNativeDriver()) { MarkFailure(TEXT("native-driver-entry-failed")); FinishScenario(TEXT("pickup-failed")); return; }
        PickupTerminal->Interact_Implementation(PlayerPawn.Get());
        LoadedVehicleId = Authority->GetBoundCargoVehicleId();
        bPickupBound = Director->GetStage() == EGTTFarmJobStage::DeliverCargo
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && LoadedVehicleId == NativeMulebox->GetPersistentVehicleId();
        TimerBeforePersistence = Director->GetTimeRemaining();
        IntegrityBeforePersistence = Director->GetCargoIntegrity();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=PICKUP result=%s stage=%s vehicle=%s timer=%.2f integrity=%.4f"),
            bPickupBound ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()), *LoadedVehicleId.ToString(),
            TimerBeforePersistence, IntegrityBeforePersistence);
        if (!bPickupBound) { MarkFailure(TEXT("native-exact-vehicle-binding-failed")); FinishScenario(TEXT("pickup-failed")); return; }
        Phase = EPersistenceEvidencePhase::RequestTow;
        break;
    }

    case EPersistenceEvidencePhase::RequestTow:
    {
        StageActor(NativeMulebox.Get(), PickupTerminal->GetActorLocation() + FVector(1400.0f, 500.0f, 80.0f));
        StageTowRecommendedState();
        const FGTTBreakdownAssessment Assessment = Breakdown->AssessVehicle(NativeMulebox.Get());
        CashBeforeTow = Economy->GetCash();
        const bool bRequested = Assessment.Recommendation == EGTTBreakdownRecommendation::TowRecommended
            && Roadside->RequestRoadsideTow(NativeMulebox.Get());
        TowLockedQuote = Roadside->GetPendingRecoveryQuote(NativeMulebox.Get());
        const bool bLocked = bRequested && TowLockedQuote > 0
            && Roadside->GetPendingRecoveryMode(NativeMulebox.Get()) == EGTTRoadsideRecoveryMode::RoadsideAssistance
            && Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId
            && Economy->GetCash() == CashBeforeTow;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=TOW_REQUEST result=%s locked_quote=%d target_pinned=%d charged=NO"),
            bLocked ? TEXT("PASS") : TEXT("FAIL"), TowLockedQuote,
            Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId ? 1 : 0);
        if (!bLocked) { MarkFailure(TEXT("tow-request-contract-failed")); FinishScenario(TEXT("tow-request-failed")); return; }
        PhaseStartedAt = Elapsed;
        Phase = EPersistenceEvidencePhase::WaitTowCheckpoint;
        break;
    }

    case EPersistenceEvidencePhase::WaitTowCheckpoint:
        if (Elapsed - PhaseStartedAt < CheckpointWaitSeconds) break;
        bTowCheckpointSaved = ReadDispatchCheckpoint(static_cast<uint8>(EGTTRoadsideRecoveryMode::RoadsideAssistance), TowLockedQuote, TowCheckpointEta);
        if (!bTowCheckpointSaved)
        {
            if (Elapsed - PhaseStartedAt > 1.35f) { MarkFailure(TEXT("tow-sidecar-checkpoint-missing")); FinishScenario(TEXT("tow-checkpoint-failed")); }
            break;
        }
        bTowPrimarySaved = GameMode && GameMode->SaveProgress();
        if (!Roadside->CancelPendingRoadsideService(NativeMulebox.Get()))
        {
            MarkFailure(TEXT("tow-pre-reload-cancel-failed")); FinishScenario(TEXT("tow-reload-failed")); return;
        }
        bTowPrimaryLoaded = GameMode && GameMode->LoadProgress();
        bTowReloadRearmed = bTowPrimaryLoaded && DispatchPersistence->ReloadCheckpointForRuntimeEvidence();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=TOW_RELOAD result=%s checkpoint=%d primary_save=%d primary_load=%d rearm=%d saved_eta=%.3f locked_quote=%d charged=NO"),
            (bTowPrimarySaved && bTowPrimaryLoaded && bTowReloadRearmed) ? TEXT("PASS") : TEXT("FAIL"),
            bTowCheckpointSaved ? 1 : 0, bTowPrimarySaved ? 1 : 0, bTowPrimaryLoaded ? 1 : 0,
            bTowReloadRearmed ? 1 : 0, TowCheckpointEta, TowLockedQuote);
        if (!bTowPrimarySaved || !bTowPrimaryLoaded || !bTowReloadRearmed)
        {
            MarkFailure(TEXT("tow-save-load-rearm-failed")); FinishScenario(TEXT("tow-reload-failed")); return;
        }
        PhaseStartedAt = Elapsed;
        Phase = EPersistenceEvidencePhase::ObserveTowRestore;
        break;

    case EPersistenceEvidencePhase::ObserveTowRestore:
        if (Elapsed - PhaseStartedAt < RestoreObservationSeconds) break;
        bTowRestored = Roadside->IsRoadsideTowPending(NativeMulebox.Get());
        bTowQuotePreserved = Roadside->GetPendingRecoveryQuote(NativeMulebox.Get()) == TowLockedQuote;
        bTowExactVehicle = Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        {
            const float RestoredEta = Roadside->GetPendingRecoverySecondsRemaining(NativeMulebox.Get());
            bTowEtaPreserved = RestoredEta > 0.0f && RestoredEta <= TowCheckpointEta + 0.40f;
            UE_LOG(LogGTT, Log,
                TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=TOW_RESTORED result=%s restored=%d quote_preserved=%d eta_preserved=%d exact_vehicle=%d saved_eta=%.3f restored_eta=%.3f charged=NO"),
                (bTowRestored && bTowQuotePreserved && bTowEtaPreserved && bTowExactVehicle && Economy->GetCash() == CashBeforeTow) ? TEXT("PASS") : TEXT("FAIL"),
                bTowRestored ? 1 : 0, bTowQuotePreserved ? 1 : 0, bTowEtaPreserved ? 1 : 0, bTowExactVehicle ? 1 : 0,
                TowCheckpointEta, RestoredEta);
        }
        if (!bTowRestored || !bTowQuotePreserved || !bTowEtaPreserved || !bTowExactVehicle || Economy->GetCash() != CashBeforeTow)
        {
            MarkFailure(TEXT("tow-restored-contract-mismatch")); FinishScenario(TEXT("tow-restore-failed")); return;
        }
        Phase = EPersistenceEvidencePhase::CancelRestoredTow;
        break;

    case EPersistenceEvidencePhase::CancelRestoredTow:
        bTowCancelledNoCharge = Roadside->CancelPendingRoadsideService(NativeMulebox.Get())
            && Economy->GetCash() == CashBeforeTow;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=TOW_CANCEL_AFTER_RESTORE result=%s cancel_no_charge=%d cash_delta=%d"),
            bTowCancelledNoCharge ? TEXT("PASS") : TEXT("FAIL"), bTowCancelledNoCharge ? 1 : 0,
            CashBeforeTow - Economy->GetCash());
        if (!bTowCancelledNoCharge) { MarkFailure(TEXT("restored-tow-cancel-charged-or-failed")); FinishScenario(TEXT("tow-cancel-failed")); return; }
        PhaseStartedAt = Elapsed;
        Phase = EPersistenceEvidencePhase::RequestWantedTow;
        break;

    case EPersistenceEvidencePhase::RequestWantedTow:
        if (Elapsed - PhaseStartedAt < 0.40f) break;
        StageTowRecommendedState();
        CashBeforeWantedTow = Economy->GetCash();
        if (!Roadside->RequestRoadsideTow(NativeMulebox.Get()))
        {
            MarkFailure(TEXT("wanted-test-tow-request-failed")); FinishScenario(TEXT("wanted-request-failed")); return;
        }
        WantedTowLockedQuote = Roadside->GetPendingRecoveryQuote(NativeMulebox.Get());
        PhaseStartedAt = Elapsed;
        Phase = EPersistenceEvidencePhase::WaitWantedTowCheckpoint;
        break;

    case EPersistenceEvidencePhase::WaitWantedTowCheckpoint:
        if (Elapsed - PhaseStartedAt < CheckpointWaitSeconds) break;
        bWantedCheckpointSaved = ReadDispatchCheckpoint(static_cast<uint8>(EGTTRoadsideRecoveryMode::RoadsideAssistance), WantedTowLockedQuote, WantedCheckpointEta);
        if (!bWantedCheckpointSaved)
        {
            if (Elapsed - PhaseStartedAt > 1.35f) { MarkFailure(TEXT("wanted-tow-sidecar-checkpoint-missing")); FinishScenario(TEXT("wanted-checkpoint-failed")); }
            break;
        }
        bWantedPrimarySaved = GameMode && GameMode->SaveProgress();
        if (!Roadside->CancelPendingRoadsideService(NativeMulebox.Get()))
        {
            MarkFailure(TEXT("wanted-tow-pre-reload-cancel-failed")); FinishScenario(TEXT("wanted-reload-failed")); return;
        }
        bWantedPrimaryLoaded = GameMode && GameMode->LoadProgress();
        Wanted->AddHeat(WantedEvidenceHeat);
        bWantedRestoreRearmed = bWantedPrimaryLoaded && Wanted->GetWantedLevel() > 0
            && DispatchPersistence->ReloadCheckpointForRuntimeEvidence();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=WANTED_RELOAD result=%s checkpoint=%d primary_save=%d primary_load=%d rearm=%d wanted=%d locked_quote=%d charged=NO"),
            (bWantedPrimarySaved && bWantedPrimaryLoaded && bWantedRestoreRearmed) ? TEXT("PASS") : TEXT("FAIL"),
            bWantedCheckpointSaved ? 1 : 0, bWantedPrimarySaved ? 1 : 0, bWantedPrimaryLoaded ? 1 : 0,
            bWantedRestoreRearmed ? 1 : 0, Wanted->GetWantedLevel(), WantedTowLockedQuote);
        if (!bWantedPrimarySaved || !bWantedPrimaryLoaded || !bWantedRestoreRearmed)
        {
            MarkFailure(TEXT("wanted-save-load-rearm-failed")); FinishScenario(TEXT("wanted-reload-failed")); return;
        }
        PhaseStartedAt = Elapsed;
        Phase = EPersistenceEvidencePhase::ObserveWantedReject;
        break;

    case EPersistenceEvidencePhase::ObserveWantedReject:
        if (Elapsed - PhaseStartedAt < RestoreObservationSeconds) break;
        bWantedRestoreRejectedNoCharge = !Roadside->HasPendingRoadsideService(NativeMulebox.Get())
            && !UGameplayStatics::DoesSaveGameExist(DispatchCheckpointSlot, 0)
            && Economy->GetCash() == CashBeforeWantedTow
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=WANTED_REJECT result=%s rejected_no_charge=%d sidecar_cleared=%d exact_cargo_vehicle=%d cash_delta=%d"),
            bWantedRestoreRejectedNoCharge ? TEXT("PASS") : TEXT("FAIL"), bWantedRestoreRejectedNoCharge ? 1 : 0,
            !UGameplayStatics::DoesSaveGameExist(DispatchCheckpointSlot, 0) ? 1 : 0,
            Authority->GetBoundCargoVehicleId() == LoadedVehicleId ? 1 : 0,
            CashBeforeWantedTow - Economy->GetCash());
        Wanted->ClearWanted();
        if (!bWantedRestoreRejectedNoCharge) { MarkFailure(TEXT("wanted-restore-did-not-fail-closed")); FinishScenario(TEXT("wanted-reject-failed")); return; }
        Phase = EPersistenceEvidencePhase::RequestPatch;
        break;

    case EPersistenceEvidencePhase::RequestPatch:
    {
        StageTowRecommendedState();
        const FGTTBreakdownAssessment Assessment = Breakdown->AssessVehicle(NativeMulebox.Get());
        CashBeforePatch = Economy->GetCash();
        const bool bRequested = Assessment.bEmergencyPatchPossible && Roadside->RequestEmergencyRoadsidePatch(NativeMulebox.Get());
        PatchLockedQuote = Roadside->GetPendingRecoveryQuote(NativeMulebox.Get());
        const bool bLocked = bRequested && PatchLockedQuote > 0
            && Roadside->GetPendingRecoveryMode(NativeMulebox.Get()) == EGTTRoadsideRecoveryMode::EmergencyPatch
            && Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId
            && Economy->GetCash() == CashBeforePatch;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=PATCH_REQUEST result=%s locked_quote=%d target_pinned=%d charged=NO"),
            bLocked ? TEXT("PASS") : TEXT("FAIL"), PatchLockedQuote,
            Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId ? 1 : 0);
        if (!bLocked) { MarkFailure(TEXT("patch-request-contract-failed")); FinishScenario(TEXT("patch-request-failed")); return; }
        PhaseStartedAt = Elapsed;
        Phase = EPersistenceEvidencePhase::WaitPatchCheckpoint;
        break;
    }

    case EPersistenceEvidencePhase::WaitPatchCheckpoint:
        if (Elapsed - PhaseStartedAt < CheckpointWaitSeconds) break;
        bPatchCheckpointSaved = ReadDispatchCheckpoint(static_cast<uint8>(EGTTRoadsideRecoveryMode::EmergencyPatch), PatchLockedQuote, PatchCheckpointEta);
        if (!bPatchCheckpointSaved)
        {
            if (Elapsed - PhaseStartedAt > 1.35f) { MarkFailure(TEXT("patch-sidecar-checkpoint-missing")); FinishScenario(TEXT("patch-checkpoint-failed")); }
            break;
        }
        bPatchPrimarySaved = GameMode && GameMode->SaveProgress();
        if (!Roadside->CancelPendingRoadsideService(NativeMulebox.Get()))
        {
            MarkFailure(TEXT("patch-pre-reload-cancel-failed")); FinishScenario(TEXT("patch-reload-failed")); return;
        }
        bPatchPrimaryLoaded = GameMode && GameMode->LoadProgress();
        bPatchReloadRearmed = bPatchPrimaryLoaded && DispatchPersistence->ReloadCheckpointForRuntimeEvidence();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=PATCH_RELOAD result=%s checkpoint=%d primary_save=%d primary_load=%d rearm=%d saved_eta=%.3f locked_quote=%d charged=NO"),
            (bPatchPrimarySaved && bPatchPrimaryLoaded && bPatchReloadRearmed) ? TEXT("PASS") : TEXT("FAIL"),
            bPatchCheckpointSaved ? 1 : 0, bPatchPrimarySaved ? 1 : 0, bPatchPrimaryLoaded ? 1 : 0,
            bPatchReloadRearmed ? 1 : 0, PatchCheckpointEta, PatchLockedQuote);
        if (!bPatchPrimarySaved || !bPatchPrimaryLoaded || !bPatchReloadRearmed)
        {
            MarkFailure(TEXT("patch-save-load-rearm-failed")); FinishScenario(TEXT("patch-reload-failed")); return;
        }
        PhaseStartedAt = Elapsed;
        Phase = EPersistenceEvidencePhase::ObservePatchRestore;
        break;

    case EPersistenceEvidencePhase::ObservePatchRestore:
        if (Elapsed - PhaseStartedAt < RestoreObservationSeconds) break;
        bPatchRestored = Roadside->IsRoadsidePatchPending(NativeMulebox.Get());
        bPatchQuotePreserved = Roadside->GetPendingRecoveryQuote(NativeMulebox.Get()) == PatchLockedQuote;
        bPatchExactVehicle = Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        {
            const float RestoredEta = Roadside->GetPendingRecoverySecondsRemaining(NativeMulebox.Get());
            bPatchEtaPreserved = RestoredEta > 0.0f && RestoredEta <= PatchCheckpointEta + 0.40f;
            bPatchNoChargeBeforeArrival = Economy->GetCash() == CashBeforePatch;
            UE_LOG(LogGTT, Log,
                TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=PATCH_RESTORED result=%s restored=%d quote_preserved=%d eta_preserved=%d exact_vehicle=%d no_charge_before_arrival=%d saved_eta=%.3f restored_eta=%.3f"),
                (bPatchRestored && bPatchQuotePreserved && bPatchEtaPreserved && bPatchExactVehicle && bPatchNoChargeBeforeArrival) ? TEXT("PASS") : TEXT("FAIL"),
                bPatchRestored ? 1 : 0, bPatchQuotePreserved ? 1 : 0, bPatchEtaPreserved ? 1 : 0,
                bPatchExactVehicle ? 1 : 0, bPatchNoChargeBeforeArrival ? 1 : 0, PatchCheckpointEta, RestoredEta);
        }
        if (!bPatchRestored || !bPatchQuotePreserved || !bPatchEtaPreserved || !bPatchExactVehicle || !bPatchNoChargeBeforeArrival)
        {
            MarkFailure(TEXT("patch-restored-contract-mismatch")); FinishScenario(TEXT("patch-restore-failed")); return;
        }
        PhaseStartedAt = Elapsed;
        Phase = EPersistenceEvidencePhase::AwaitPatchCompletion;
        break;

    case EPersistenceEvidencePhase::AwaitPatchCompletion:
        if (Roadside->IsRoadsidePatchPending(NativeMulebox.Get()))
        {
            if (Elapsed - PhaseStartedAt > 4.5f) { MarkFailure(TEXT("restored-patch-did-not-complete")); FinishScenario(TEXT("patch-timeout")); }
            break;
        }
        bPatchCompleted = Economy->GetCash() < CashBeforePatch;
        bPatchSingleCharge = CashBeforePatch - Economy->GetCash() == PatchLockedQuote;
        bTimerContinued = Director->GetTimeRemaining() < TimerBeforePersistence;
        bIntegrityNotImproved = Director->GetCargoIntegrity() <= IntegrityBeforePersistence + KINDA_SMALL_NUMBER;
        bPatchExactVehicle = bPatchExactVehicle
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId
            && NativeMulebox->GetPersistentVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=PATCH_COMPLETE result=%s completed=%d single_charge=%d charged=%d locked_quote=%d exact_vehicle=%d timer_continued=%d integrity_not_improved=%d"),
            (bPatchCompleted && bPatchSingleCharge && bPatchExactVehicle && bTimerContinued && bIntegrityNotImproved) ? TEXT("PASS") : TEXT("FAIL"),
            bPatchCompleted ? 1 : 0, bPatchSingleCharge ? 1 : 0, CashBeforePatch - Economy->GetCash(), PatchLockedQuote,
            bPatchExactVehicle ? 1 : 0, bTimerContinued ? 1 : 0, bIntegrityNotImproved ? 1 : 0);
        if (!bPatchCompleted || !bPatchSingleCharge || !bPatchExactVehicle || !bTimerContinued || !bIntegrityNotImproved)
        {
            MarkFailure(TEXT("restored-patch-completion-contract-failed")); FinishScenario(TEXT("patch-complete-failed")); return;
        }
        Phase = EPersistenceEvidencePhase::WrongVehicle;
        break;

    case EPersistenceEvidencePhase::WrongVehicle:
    {
        const FVector HillLocation = HillTerminal->GetActorLocation();
        StageActor(NativeMulebox.Get(), HillLocation + FVector(ExactVehicleFarOffsetCm, 0.0f, 80.0f));
        StageActor(SpawnedDecoyVan.Get(), HillLocation + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        const EGTTFarmJobStage Before = Director->GetStage();
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bWrongVehicleRejected = Before == EGTTFarmJobStage::DeliverCargo
            && Director->GetStage() == EGTTFarmJobStage::DeliverCargo
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=WRONG_VEHICLE result=%s rejected=%d bound_vehicle=%s stage=%s"),
            bWrongVehicleRejected ? TEXT("PASS") : TEXT("FAIL"), bWrongVehicleRejected ? 1 : 0,
            *Authority->GetBoundCargoVehicleId().ToString(), StageLabel(Director->GetStage()));
        if (!bWrongVehicleRejected) { MarkFailure(TEXT("wrong-vehicle-after-persistence-was-not-rejected")); FinishScenario(TEXT("wrong-vehicle-failed")); return; }
        Phase = EPersistenceEvidencePhase::HillHandoff;
        break;
    }

    case EPersistenceEvidencePhase::HillHandoff:
    {
        const FVector HillLocation = HillTerminal->GetActorLocation();
        StageActor(NativeMulebox.Get(), HillLocation + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bHillHandoff = Director->GetStage() == EGTTFarmJobStage::DeliverFinalStop
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=HILL_HANDOFF result=%s same_vehicle=%d stage=%s"),
            bHillHandoff ? TEXT("PASS") : TEXT("FAIL"), bHillHandoff ? 1 : 0, StageLabel(Director->GetStage()));
        if (!bHillHandoff) { MarkFailure(TEXT("hill-handoff-after-dispatch-persistence-failed")); FinishScenario(TEXT("hill-failed")); return; }
        Phase = EPersistenceEvidencePhase::FinalHandoff;
        break;
    }

    case EPersistenceEvidencePhase::FinalHandoff:
    {
        const int32 CashBeforeFinal = Economy->GetCash();
        const FVector FinalLocation = FinalTerminal->GetActorLocation();
        StageActor(NativeMulebox.Get(), FinalLocation + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        FinalTerminal->Interact_Implementation(PlayerPawn.Get());
        bFinalHandoff = Director->GetStage() == EGTTFarmJobStage::Idle && !Authority->HasBoundCargoVehicle();
        PayoutDelta = Economy->GetCash() - CashBeforeFinal;
        CargoRunsDelta = Logistics->GetCargoCompletedRuns() - EvidenceCargoRunsBefore;
        ReputationDelta = Logistics->GetReputation() - EvidenceReputationBefore;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=FINAL_HANDOFF result=%s final=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d authority_cleared=%d"),
            (bFinalHandoff && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0) ? TEXT("PASS") : TEXT("FAIL"),
            bFinalHandoff ? 1 : 0, PayoutDelta, CargoRunsDelta, ReputationDelta,
            !Authority->HasBoundCargoVehicle() ? 1 : 0);
        if (!bFinalHandoff || PayoutDelta <= 0 || CargoRunsDelta != 1 || ReputationDelta <= 0)
        {
            MarkFailure(TEXT("final-payout-reputation-or-authority-proof-failed")); FinishScenario(TEXT("final-failed")); return;
        }
        Phase = EPersistenceEvidencePhase::VerifyPersistence;
        break;
    }

    case EPersistenceEvidencePhase::VerifyPersistence:
        bSaveVerified = GameMode && GameMode->SaveProgress();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_PERSISTENCE_RUNTIME phase=PERSISTENCE result=%s explicit_save=%d sidecar_present=%d"),
            bSaveVerified ? TEXT("PASS") : TEXT("FAIL"), bSaveVerified ? 1 : 0,
            UGameplayStatics::DoesSaveGameExist(DispatchCheckpointSlot, 0) ? 1 : 0);
        if (!bSaveVerified) MarkFailure(TEXT("post-persistence-route-save-failed"));
        FinishScenario(TEXT("sequence-complete"));
        break;

    case EPersistenceEvidencePhase::Complete:
        break;
    }
}
