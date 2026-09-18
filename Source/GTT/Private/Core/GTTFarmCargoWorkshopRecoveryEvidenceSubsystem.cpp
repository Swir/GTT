#include "Core/GTTFarmCargoWorkshopRecoveryEvidenceSubsystem.h"

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
#include "Vehicles/GTTBreakdownDecisionSubsystem.h"
#include "Vehicles/GTTFarmVanPawn.h"
#include "Vehicles/GTTRoadsideRecoverySubsystem.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTGarageSlotTerminal.h"
#include "World/GTTLogisticsReputationSubsystem.h"
#include "World/GTTServiceTerminal.h"
#include "GTT.h"

namespace
{
constexpr float StartDelaySeconds = 356.0f;
constexpr float GlobalDeadlineSeconds = 382.0f;
constexpr float TowCompletionTimeoutSeconds = 5.5f;
constexpr float HandoffParkingOffsetCm = 120.0f;
constexpr float ExactVehicleFarOffsetCm = 1500.0f;
constexpr float WorkshopEvidenceRadiusCm = 1050.0f;
constexpr int32 MinimumEvidenceRouteTier = 2;
constexpr int32 MinimumEvidenceCash = 6000;

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

void UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRuntimeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRecoveryScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoBreakdownScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoDispatchScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoDispatchPersistenceScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoWorkshopRecoveryScenario"));
    if (bEnabled)
    {
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME_BEGIN version=1 route=feed-tow-workshop-hold-service-hill-wood start_delay=%.1f deadline=%.1f exact_vehicle=required locked_quote=required garage_bypass=forbidden workshop_charge=required"),
            StartDelaySeconds, GlobalDeadlineSeconds);
    }
}

TStatId UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem, STATGROUP_Tickables);
}

bool UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

APawn* UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::ResolvePlayerPawn() const
{
    const UWorld* World = GetWorld();
    if (!World) return nullptr;
    const APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return nullptr;
    APawn* Controlled = PC->GetPawn();
    if (AGTTRoadVehicleNativePawn* Native = Cast<AGTTRoadVehicleNativePawn>(Controlled)) return Native->GetDriverPawn();
    return Controlled;
}

AGTTFarmJobTerminal* UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::FindTerminal(uint8 TerminalTypeValue) const
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

AGTTMuleboxNativePawn* UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::FindNativeMulebox() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTMuleboxNativePawn> It(World); It; ++It)
    {
        if (It->IsNativeReady()) return *It;
    }
    return nullptr;
}

AGTTServiceTerminal* UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::FindWorkshopTerminal() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTServiceTerminal> It(World); It; ++It)
    {
        if (It->GetServiceType() == EGTTServiceType::Workshop) return *It;
    }
    return nullptr;
}

AGTTGarageSlotTerminal* UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::FindGarageSlotForVehicle(FName VehicleId) const
{
    UWorld* World = GetWorld();
    if (!World || !GarageFleet.IsValid() || VehicleId.IsNone()) return nullptr;
    for (TActorIterator<AGTTGarageSlotTerminal> It(World); It; ++It)
    {
        FGTTGarageFleetSnapshot Snapshot;
        if (GarageFleet->GetSlotSnapshot(It->GetSlotIndex(), Snapshot) && Snapshot.VehicleId == VehicleId) return *It;
    }
    return nullptr;
}

AGTTFarmVanPawn* UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::SpawnDecoyVan(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    FActorSpawnParameters Params;
    Params.Name = MakeUniqueObjectName(World, AGTTFarmVanPawn::StaticClass(), FName(TEXT("GTTFarmCargoWorkshopRecoveryDecoy")));
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return World->SpawnActor<AGTTFarmVanPawn>(Location, FRotator::ZeroRotator, Params);
}

void UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::StageActor(AActor* Actor, const FVector& Location, const FRotator& Rotation) const
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

bool UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::EnsureNativeDriver()
{
    UWorld* World = GetWorld();
    if (!World || !NativeMulebox.IsValid() || !PlayerPawn.IsValid()) return false;
    if (UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()) return true;
    if (!NativeMulebox->IsLegacyTakeoverActive() && !NativeMulebox->TryActivateLegacyTakeover()) return false;
    NativeMulebox->Interact_Implementation(PlayerPawn.Get());
    return UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()
        && NativeMulebox->GetDriverPawn() == PlayerPawn.Get();
}

void UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::StageTowRecommendedState()
{
    if (!NativeMulebox.IsValid()) return;
    FGTTRoadVehicleMigrationSnapshot Staged = NativeMulebox->GetMigrationSnapshot();
    Staged.ConditionPercent = FMath::Min(Staged.ConditionPercent, 0.19f);
    Staged.TireIntegrity = FMath::Min(Staged.TireIntegrity, 0.20f);
    Staged.FuelLiters = FMath::Max(Staged.FuelLiters, FMath::Min(NativeMulebox->GetFuelCapacityLiters(), 6.0f));
    NativeMulebox->RestorePersistentMigrationSnapshot(Staged);
}

bool UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::ResolveScenarioActors()
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
    if (!WorkshopTerminal.IsValid()) WorkshopTerminal = FindWorkshopTerminal();
    if (!Authority.IsValid()) Authority = World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>();
    if (!Breakdown.IsValid()) Breakdown = World->GetSubsystem<UGTTBreakdownDecisionSubsystem>();
    if (!GarageFleet.IsValid()) GarageFleet = World->GetSubsystem<UGTTGarageFleetSubsystem>();
    if (!Roadside.IsValid()) Roadside = World->GetSubsystem<UGTTRoadsideRecoverySubsystem>();
    if (!Logistics.IsValid()) Logistics = World->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    if (!Economy.IsValid() && PlayerPawn.IsValid()) Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn.Get());
    if (!Wanted.IsValid() && PlayerPawn.IsValid()) Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn.Get());
    if (!DayNight.IsValid())
    {
        if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) DayNight = GameMode->GetDayNightCycle();
    }
    return Director.IsValid() && PlayerPawn.IsValid() && StartTerminal.IsValid() && PickupTerminal.IsValid()
        && HillTerminal.IsValid() && FinalTerminal.IsValid() && NativeMulebox.IsValid() && WorkshopTerminal.IsValid()
        && Authority.IsValid() && Breakdown.IsValid() && GarageFleet.IsValid() && Roadside.IsValid()
        && Logistics.IsValid() && Economy.IsValid() && Wanted.IsValid() && DayNight.IsValid();
}

void UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    UE_LOG(LogGTT, Error, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::RestoreBaselineState()
{
    UWorld* World = GetWorld();
    if (!World || !bBaselineCaptured) return;
    if (Roadside.IsValid() && NativeMulebox.IsValid() && Roadside->HasPendingRoadsideService(NativeMulebox.Get()))
        Roadside->CancelPendingRoadsideService(NativeMulebox.Get());
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
        Authority->ClearLoadedVehicle(TEXT("workshop-recovery-runtime-evidence-cleanup"));
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

void UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bAuthorityCleared = Authority.IsValid() && !Authority->HasBoundCargoVehicle()
        && Authority->GetBoundCargoVehicleId().IsNone();
    const bool bPass = bSequenceHealthy && bAccepted && bPickupBound && bTowRequested && bTowCompleted
        && bTowSingleCharge && bTowDamagePreserved && bTowIdentityPreserved && bWorkshopDestination
        && bWorkshopHoldDetected && bGarageRecallBlocked && bGarageRecallNoCharge && bGarageRecallNoMove
        && bWorkshopServiceApplied && bWorkshopSingleCharge && bWorkshopHoldCleared && bMechanicalRepaired
        && bRefuelled && bExactVehiclePreserved && bTimerContinued && bIntegrityNotImproved
        && bWrongVehicleRejected && bHillHandoff && bFinalHandoff && bSaveVerified && bAuthorityCleared
        && TowLockedQuote > 0 && WorkshopQuote > 0 && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0;

    UE_LOG(LogGTT, Log,
        TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME_COMPLETE result=%s route=feed-tow-workshop-hold-service-hill-wood accepted=%d pickup=%d tow_requested=%d tow_completed=%d tow_single_charge=%d tow_damage_preserved=%d tow_identity_preserved=%d workshop_destination=%d hold_detected=%d garage_recall_blocked=%d garage_no_charge=%d garage_no_move=%d workshop_service=%d workshop_single_charge=%d hold_cleared=%d repaired=%d refuelled=%d exact_vehicle=%d timer_continued=%d integrity_not_improved=%d wrong_vehicle_rejected=%d hill=%d final=%d save=%d authority_cleared=%d tow_quote=%d workshop_quote=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d vehicle=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bAccepted ? 1 : 0, bPickupBound ? 1 : 0,
        bTowRequested ? 1 : 0, bTowCompleted ? 1 : 0, bTowSingleCharge ? 1 : 0,
        bTowDamagePreserved ? 1 : 0, bTowIdentityPreserved ? 1 : 0, bWorkshopDestination ? 1 : 0,
        bWorkshopHoldDetected ? 1 : 0, bGarageRecallBlocked ? 1 : 0, bGarageRecallNoCharge ? 1 : 0,
        bGarageRecallNoMove ? 1 : 0, bWorkshopServiceApplied ? 1 : 0, bWorkshopSingleCharge ? 1 : 0,
        bWorkshopHoldCleared ? 1 : 0, bMechanicalRepaired ? 1 : 0, bRefuelled ? 1 : 0,
        bExactVehiclePreserved ? 1 : 0, bTimerContinued ? 1 : 0, bIntegrityNotImproved ? 1 : 0,
        bWrongVehicleRejected ? 1 : 0, bHillHandoff ? 1 : 0, bFinalHandoff ? 1 : 0,
        bSaveVerified ? 1 : 0, bAuthorityCleared ? 1 : 0, TowLockedQuote, WorkshopQuote,
        PayoutDelta, CargoRunsDelta, ReputationDelta, *LoadedVehicleId.ToString(),
        Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = EWorkshopEvidencePhase::Complete;
    bFinished = true;
}

void UGTTFarmCargoWorkshopRecoveryEvidenceSubsystem::Tick(float DeltaTime)
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
            MarkFailure(TEXT("workshop-recovery-world-actors-unavailable"));
            FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    UWorld* World = GetWorld();
    AGTTGameMode* GameMode = World ? World->GetAuthGameMode<AGTTGameMode>() : nullptr;

    switch (Phase)
    {
    case EWorkshopEvidencePhase::Waiting:
        Phase = EWorkshopEvidencePhase::Prepare;
        break;

    case EWorkshopEvidencePhase::Prepare:
    {
        if (!GameMode || Director->GetStage() != EGTTFarmJobStage::Idle || GameMode->GetWildlifeAlertLevel() > 0)
        {
            MarkFailure(TEXT("farm-director-or-legal-work-state-not-clean")); FinishScenario(TEXT("prepare-failed")); return;
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
            MarkFailure(TEXT("tier2-stock-backed-workshop-route-unavailable")); FinishScenario(TEXT("prepare-failed")); return;
        }
        if (!NativeMulebox->IsLegacyTakeoverActive() && !NativeMulebox->TryActivateLegacyTakeover())
        {
            MarkFailure(TEXT("native-mulebox-takeover-unavailable")); FinishScenario(TEXT("prepare-failed")); return;
        }
        SpawnedDecoyVan = SpawnDecoyVan(PlayerPawn->GetActorLocation() + FVector(500.0f, 0.0f, 80.0f));
        if (!SpawnedDecoyVan.IsValid())
        {
            MarkFailure(TEXT("decoy-vehicle-spawn-failed")); FinishScenario(TEXT("prepare-failed")); return;
        }
        EvidenceCargoRunsBefore = Logistics->GetCargoCompletedRuns();
        EvidenceReputationBefore = Logistics->GetReputation();
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=PREPARE result=PASS route_tier=%d cash_seeded=%d"),
            Logistics->GetCargoRouteTier(), Economy->GetCash());
        Phase = EWorkshopEvidencePhase::AcceptContract;
        break;
    }

    case EWorkshopEvidencePhase::AcceptContract:
        StartTerminal->Interact_Implementation(PlayerPawn.Get());
        bAccepted = Director->GetStage() == EGTTFarmJobStage::ReachPickup;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=ACCEPT result=%s stage=%s"),
            bAccepted ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()));
        if (!bAccepted) { MarkFailure(TEXT("contract-acceptance-failed")); FinishScenario(TEXT("accept-failed")); return; }
        Phase = EWorkshopEvidencePhase::EnterAndPickup;
        break;

    case EWorkshopEvidencePhase::EnterAndPickup:
    {
        StageActor(NativeMulebox.Get(), PickupTerminal->GetActorLocation() + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        if (!EnsureNativeDriver()) { MarkFailure(TEXT("native-driver-entry-failed")); FinishScenario(TEXT("pickup-failed")); return; }
        PickupTerminal->Interact_Implementation(PlayerPawn.Get());
        LoadedVehicleId = Authority->GetBoundCargoVehicleId();
        bPickupBound = Director->GetStage() == EGTTFarmJobStage::DeliverCargo
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && LoadedVehicleId == NativeMulebox->GetPersistentVehicleId();
        TimerBeforeRecovery = Director->GetTimeRemaining();
        IntegrityBeforeRecovery = Director->GetCargoIntegrity();
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=PICKUP result=%s vehicle=%s timer=%.2f integrity=%.4f"),
            bPickupBound ? TEXT("PASS") : TEXT("FAIL"), *LoadedVehicleId.ToString(), TimerBeforeRecovery, IntegrityBeforeRecovery);
        if (!bPickupBound) { MarkFailure(TEXT("native-exact-vehicle-binding-failed")); FinishScenario(TEXT("pickup-failed")); return; }
        Phase = EWorkshopEvidencePhase::RequestTow;
        break;
    }

    case EWorkshopEvidencePhase::RequestTow:
    {
        StageActor(NativeMulebox.Get(), PickupTerminal->GetActorLocation() + FVector(1400.0f, 500.0f, 80.0f));
        StageTowRecommendedState();
        const FGTTBreakdownAssessment Assessment = Breakdown->AssessVehicle(NativeMulebox.Get());
        const FGTTRoadVehicleMigrationSnapshot Before = NativeMulebox->GetMigrationSnapshot();
        TowConditionBefore = Before.ConditionPercent;
        TowTireBefore = Before.TireIntegrity;
        CashBeforeTow = Economy->GetCash();
        bTowRequested = Assessment.Recommendation == EGTTBreakdownRecommendation::TowRecommended
            && Roadside->RequestRoadsideTow(NativeMulebox.Get());
        TowLockedQuote = Roadside->GetPendingRecoveryQuote(NativeMulebox.Get());
        const bool bPinned = bTowRequested && TowLockedQuote > 0
            && Roadside->GetPendingRecoveryMode(NativeMulebox.Get()) == EGTTRoadsideRecoveryMode::RoadsideAssistance
            && Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId
            && Economy->GetCash() == CashBeforeTow;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=TOW_REQUEST result=%s locked_quote=%d target_pinned=%d charged=NO"),
            bPinned ? TEXT("PASS") : TEXT("FAIL"), TowLockedQuote,
            Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId ? 1 : 0);
        if (!bPinned) { MarkFailure(TEXT("tow-request-contract-failed")); FinishScenario(TEXT("tow-request-failed")); return; }
        PhaseStartedAt = Elapsed;
        Phase = EWorkshopEvidencePhase::AwaitTowCompletion;
        break;
    }

    case EWorkshopEvidencePhase::AwaitTowCompletion:
    {
        if (Roadside->IsRoadsideTowPending(NativeMulebox.Get()))
        {
            if (Elapsed - PhaseStartedAt > TowCompletionTimeoutSeconds)
            {
                MarkFailure(TEXT("tow-did-not-complete")); FinishScenario(TEXT("tow-timeout"));
            }
            break;
        }
        const FGTTRoadVehicleMigrationSnapshot After = NativeMulebox->GetMigrationSnapshot();
        bTowCompleted = CashBeforeTow - Economy->GetCash() > 0;
        bTowSingleCharge = CashBeforeTow - Economy->GetCash() == TowLockedQuote;
        bTowDamagePreserved = FMath::IsNearlyEqual(After.ConditionPercent, TowConditionBefore, 0.001f)
            && FMath::IsNearlyEqual(After.TireIntegrity, TowTireBefore, 0.001f);
        bTowIdentityPreserved = NativeMulebox->GetPersistentVehicleId() == LoadedVehicleId
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        bWorkshopDestination = WorkshopTerminal.IsValid()
            && FVector::Dist(NativeMulebox->GetActorLocation(), WorkshopTerminal->GetActorLocation()) <= WorkshopEvidenceRadiusCm;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=TOW_COMPLETE result=%s single_charge=%d charged=%d locked_quote=%d damage_preserved=%d identity_preserved=%d workshop_destination=%d workshop_distance=%.1f"),
            (bTowCompleted && bTowSingleCharge && bTowDamagePreserved && bTowIdentityPreserved && bWorkshopDestination) ? TEXT("PASS") : TEXT("FAIL"),
            bTowSingleCharge ? 1 : 0, CashBeforeTow - Economy->GetCash(), TowLockedQuote, bTowDamagePreserved ? 1 : 0,
            bTowIdentityPreserved ? 1 : 0, bWorkshopDestination ? 1 : 0,
            WorkshopTerminal.IsValid() ? FVector::Dist(NativeMulebox->GetActorLocation(), WorkshopTerminal->GetActorLocation()) : -1.0f);
        if (!bTowCompleted || !bTowSingleCharge || !bTowDamagePreserved || !bTowIdentityPreserved || !bWorkshopDestination)
        {
            MarkFailure(TEXT("tow-workshop-handoff-failed")); FinishScenario(TEXT("tow-complete-failed")); return;
        }
        Phase = EWorkshopEvidencePhase::VerifyWorkshopHold;
        break;
    }

    case EWorkshopEvidencePhase::VerifyWorkshopHold:
    {
        FGTTGarageFleetSnapshot Snapshot;
        const TArray<FGTTGarageFleetSnapshot> Fleet = GarageFleet->BuildFleetSnapshot(8);
        const FGTTGarageFleetSnapshot* Found = Fleet.FindByPredicate([this](const FGTTGarageFleetSnapshot& Candidate)
            { return Candidate.VehicleId == LoadedVehicleId; });
        if (Found) Snapshot = *Found;
        bWorkshopHoldDetected = Found && GarageFleet->IsVehicleOnWorkshopHold(LoadedVehicleId)
            && (Snapshot.ServiceStatus == TEXT("TOW") || Snapshot.ServiceStatus == TEXT("IMMOBILE"));
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=WORKSHOP_HOLD result=%s hold=%d service_status=%s repair_estimate=%d hold_count=%d"),
            bWorkshopHoldDetected ? TEXT("PASS") : TEXT("FAIL"), bWorkshopHoldDetected ? 1 : 0,
            Found ? *Snapshot.ServiceStatus : TEXT("MISSING"), Found ? Snapshot.RepairEstimate : 0, GarageFleet->GetWorkshopHoldCount(8));
        if (!bWorkshopHoldDetected) { MarkFailure(TEXT("authoritative-workshop-hold-missing")); FinishScenario(TEXT("hold-failed")); return; }
        Phase = EWorkshopEvidencePhase::RejectGarageRecall;
        break;
    }

    case EWorkshopEvidencePhase::RejectGarageRecall:
    {
        AGTTGarageSlotTerminal* Slot = FindGarageSlotForVehicle(LoadedVehicleId);
        if (!Slot) { MarkFailure(TEXT("garage-slot-for-loaded-vehicle-missing")); FinishScenario(TEXT("garage-recall-failed")); return; }
        const int32 CashBeforeRecall = Economy->GetCash();
        const FTransform BeforeRecall = NativeMulebox->GetActorTransform();
        Slot->Interact_Implementation(PlayerPawn.Get());
        bGarageRecallNoCharge = Economy->GetCash() == CashBeforeRecall;
        bGarageRecallNoMove = NativeMulebox->GetActorLocation().Equals(BeforeRecall.GetLocation(), 2.0f);
        bGarageRecallBlocked = GarageFleet->IsVehicleOnWorkshopHold(LoadedVehicleId)
            && bGarageRecallNoCharge && bGarageRecallNoMove;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=GARAGE_RECALL_REJECT result=%s blocked=%d no_charge=%d no_move=%d slot=%d"),
            bGarageRecallBlocked ? TEXT("PASS") : TEXT("FAIL"), bGarageRecallBlocked ? 1 : 0,
            bGarageRecallNoCharge ? 1 : 0, bGarageRecallNoMove ? 1 : 0, Slot->GetSlotIndex());
        if (!bGarageRecallBlocked) { MarkFailure(TEXT("garage-recall-bypassed-workshop-hold")); FinishScenario(TEXT("garage-recall-failed")); return; }
        Phase = EWorkshopEvidencePhase::ApplyWorkshopService;
        break;
    }

    case EWorkshopEvidencePhase::ApplyWorkshopService:
    {
        StageActor(NativeMulebox.Get(), WorkshopTerminal->GetActorLocation() + FVector(140.0f, 0.0f, 80.0f));
        WorkshopQuote = WorkshopTerminal->GetNativeRoadRepairQuote(NativeMulebox.Get());
        CashBeforeWorkshop = Economy->GetCash();
        const float TimerBeforeService = Director->GetTimeRemaining();
        const float IntegrityBeforeService = Director->GetCargoIntegrity();
        WorkshopTerminal->Interact_Implementation(PlayerPawn.Get());
        const FGTTRoadVehicleMigrationSnapshot Serviced = NativeMulebox->GetMigrationSnapshot();
        bWorkshopServiceApplied = WorkshopQuote > 0 && Economy->GetCash() < CashBeforeWorkshop;
        bWorkshopSingleCharge = CashBeforeWorkshop - Economy->GetCash() == WorkshopQuote;
        bWorkshopHoldCleared = !GarageFleet->IsVehicleOnWorkshopHold(LoadedVehicleId);
        bMechanicalRepaired = Serviced.ConditionPercent >= 0.999f && Serviced.TireIntegrity >= 0.999f;
        bRefuelled = Serviced.FuelLiters + 0.05f >= NativeMulebox->GetFuelCapacityLiters();
        bExactVehiclePreserved = NativeMulebox->GetPersistentVehicleId() == LoadedVehicleId
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        bTimerContinued = Director->GetTimeRemaining() < TimerBeforeRecovery
            && Director->GetTimeRemaining() <= TimerBeforeService + KINDA_SMALL_NUMBER;
        bIntegrityNotImproved = Director->GetCargoIntegrity() <= IntegrityBeforeRecovery + KINDA_SMALL_NUMBER
            && Director->GetCargoIntegrity() <= IntegrityBeforeService + KINDA_SMALL_NUMBER;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=WORKSHOP_SERVICE result=%s service=%d single_charge=%d charged=%d quote=%d hold_cleared=%d repaired=%d refuelled=%d exact_vehicle=%d timer_continued=%d integrity_not_improved=%d"),
            (bWorkshopServiceApplied && bWorkshopSingleCharge && bWorkshopHoldCleared && bMechanicalRepaired && bRefuelled && bExactVehiclePreserved && bTimerContinued && bIntegrityNotImproved) ? TEXT("PASS") : TEXT("FAIL"),
            bWorkshopServiceApplied ? 1 : 0, bWorkshopSingleCharge ? 1 : 0, CashBeforeWorkshop - Economy->GetCash(), WorkshopQuote,
            bWorkshopHoldCleared ? 1 : 0, bMechanicalRepaired ? 1 : 0, bRefuelled ? 1 : 0, bExactVehiclePreserved ? 1 : 0,
            bTimerContinued ? 1 : 0, bIntegrityNotImproved ? 1 : 0);
        if (!bWorkshopServiceApplied || !bWorkshopSingleCharge || !bWorkshopHoldCleared || !bMechanicalRepaired
            || !bRefuelled || !bExactVehiclePreserved || !bTimerContinued || !bIntegrityNotImproved)
        {
            MarkFailure(TEXT("workshop-service-authority-or-economy-proof-failed")); FinishScenario(TEXT("workshop-service-failed")); return;
        }
        Phase = EWorkshopEvidencePhase::WrongVehicle;
        break;
    }

    case EWorkshopEvidencePhase::WrongVehicle:
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
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=WRONG_VEHICLE result=%s rejected=%d vehicle=%s"),
            bWrongVehicleRejected ? TEXT("PASS") : TEXT("FAIL"), bWrongVehicleRejected ? 1 : 0, *LoadedVehicleId.ToString());
        if (!bWrongVehicleRejected) { MarkFailure(TEXT("wrong-vehicle-after-workshop-was-not-rejected")); FinishScenario(TEXT("wrong-vehicle-failed")); return; }
        Phase = EWorkshopEvidencePhase::HillHandoff;
        break;
    }

    case EWorkshopEvidencePhase::HillHandoff:
    {
        if (!EnsureNativeDriver()) { MarkFailure(TEXT("native-driver-reentry-after-workshop-failed")); FinishScenario(TEXT("hill-failed")); return; }
        StageActor(NativeMulebox.Get(), HillTerminal->GetActorLocation() + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bHillHandoff = Director->GetStage() == EGTTFarmJobStage::DeliverFinalStop
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=HILL_HANDOFF result=%s same_vehicle=%d"),
            bHillHandoff ? TEXT("PASS") : TEXT("FAIL"), bHillHandoff ? 1 : 0);
        if (!bHillHandoff) { MarkFailure(TEXT("hill-handoff-after-workshop-failed")); FinishScenario(TEXT("hill-failed")); return; }
        Phase = EWorkshopEvidencePhase::FinalHandoff;
        break;
    }

    case EWorkshopEvidencePhase::FinalHandoff:
    {
        const int32 CashBeforeFinal = Economy->GetCash();
        StageActor(NativeMulebox.Get(), FinalTerminal->GetActorLocation() + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        FinalTerminal->Interact_Implementation(PlayerPawn.Get());
        bFinalHandoff = Director->GetStage() == EGTTFarmJobStage::Idle && !Authority->HasBoundCargoVehicle();
        PayoutDelta = Economy->GetCash() - CashBeforeFinal;
        CargoRunsDelta = Logistics->GetCargoCompletedRuns() - EvidenceCargoRunsBefore;
        ReputationDelta = Logistics->GetReputation() - EvidenceReputationBefore;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=FINAL_HANDOFF result=%s final=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d authority_cleared=%d"),
            (bFinalHandoff && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0) ? TEXT("PASS") : TEXT("FAIL"),
            bFinalHandoff ? 1 : 0, PayoutDelta, CargoRunsDelta, ReputationDelta, !Authority->HasBoundCargoVehicle() ? 1 : 0);
        if (!bFinalHandoff || PayoutDelta <= 0 || CargoRunsDelta != 1 || ReputationDelta <= 0)
        {
            MarkFailure(TEXT("final-payout-reputation-or-authority-proof-failed")); FinishScenario(TEXT("final-failed")); return;
        }
        Phase = EWorkshopEvidencePhase::VerifyPersistence;
        break;
    }

    case EWorkshopEvidencePhase::VerifyPersistence:
        bSaveVerified = GameMode && GameMode->SaveProgress();
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_WORKSHOP_RECOVERY_RUNTIME phase=PERSISTENCE result=%s explicit_save=%d"),
            bSaveVerified ? TEXT("PASS") : TEXT("FAIL"), bSaveVerified ? 1 : 0);
        if (!bSaveVerified) MarkFailure(TEXT("post-workshop-route-save-failed"));
        FinishScenario(TEXT("sequence-complete"));
        break;

    case EWorkshopEvidencePhase::Complete:
        break;
    }
}
