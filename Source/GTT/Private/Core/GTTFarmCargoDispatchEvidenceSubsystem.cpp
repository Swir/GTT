#include "Core/GTTFarmCargoDispatchEvidenceSubsystem.h"

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
#include "World/GTTLogisticsReputationSubsystem.h"
#include "GTT.h"

namespace
{
constexpr float StartDelaySeconds = 282.0f;
constexpr float GlobalDeadlineSeconds = 320.0f;
constexpr float DispatchObservationSeconds = 0.75f;
constexpr float PatchCompletionProofSeconds = 3.25f;
constexpr float ExactVehicleFarOffsetCm = 1500.0f;
constexpr float HandoffParkingOffsetCm = 120.0f;
constexpr int32 MinimumEvidenceRouteTier = 2;
constexpr int32 MinimumEvidenceCash = 3000;

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

void UGTTFarmCargoDispatchEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRuntimeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRecoveryScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoBreakdownScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoDispatchScenario"));
    if (bEnabled)
    {
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_RUNTIME_BEGIN version=1 route=feed-dispatch-cancel-rerequest-hill-wood start_delay=%.1f deadline=%.1f native_mulebox=required locked_quote=required live_eta=required cancellation_no_charge=required exact_vehicle=required"),
            StartDelaySeconds, GlobalDeadlineSeconds);
    }
}

TStatId UGTTFarmCargoDispatchEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmCargoDispatchEvidenceSubsystem, STATGROUP_Tickables);
}

bool UGTTFarmCargoDispatchEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

APawn* UGTTFarmCargoDispatchEvidenceSubsystem::ResolvePlayerPawn() const
{
    const UWorld* World = GetWorld();
    if (!World) return nullptr;
    const APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return nullptr;
    APawn* Controlled = PC->GetPawn();
    if (AGTTRoadVehicleNativePawn* Native = Cast<AGTTRoadVehicleNativePawn>(Controlled)) return Native->GetDriverPawn();
    return Controlled;
}

AGTTFarmJobTerminal* UGTTFarmCargoDispatchEvidenceSubsystem::FindTerminal(uint8 TerminalTypeValue) const
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

AGTTMuleboxNativePawn* UGTTFarmCargoDispatchEvidenceSubsystem::FindNativeMulebox() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTMuleboxNativePawn> It(World); It; ++It)
    {
        if (It->IsNativeReady()) return *It;
    }
    return nullptr;
}

AGTTFarmVanPawn* UGTTFarmCargoDispatchEvidenceSubsystem::SpawnDecoyVan(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    FActorSpawnParameters Params;
    Params.Name = MakeUniqueObjectName(World, AGTTFarmVanPawn::StaticClass(), FName(TEXT("GTTFarmCargoDispatchDecoy")));
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return World->SpawnActor<AGTTFarmVanPawn>(Location, FRotator::ZeroRotator, Params);
}

void UGTTFarmCargoDispatchEvidenceSubsystem::StageActor(AActor* Actor, const FVector& Location, const FRotator& Rotation) const
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

bool UGTTFarmCargoDispatchEvidenceSubsystem::EnsureNativeDriver()
{
    UWorld* World = GetWorld();
    if (!World || !NativeMulebox.IsValid() || !PlayerPawn.IsValid()) return false;
    if (UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()) return true;
    if (!NativeMulebox->IsLegacyTakeoverActive() && !NativeMulebox->TryActivateLegacyTakeover()) return false;
    NativeMulebox->Interact_Implementation(PlayerPawn.Get());
    return UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()
        && NativeMulebox->GetDriverPawn() == PlayerPawn.Get();
}

bool UGTTFarmCargoDispatchEvidenceSubsystem::ResolveScenarioActors()
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
    if (!Logistics.IsValid()) Logistics = World->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    if (!Economy.IsValid() && PlayerPawn.IsValid()) Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn.Get());
    if (!Wanted.IsValid() && PlayerPawn.IsValid()) Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn.Get());
    if (!DayNight.IsValid())
    {
        if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) DayNight = GameMode->GetDayNightCycle();
    }
    return Director.IsValid() && PlayerPawn.IsValid() && StartTerminal.IsValid() && PickupTerminal.IsValid()
        && HillTerminal.IsValid() && FinalTerminal.IsValid() && NativeMulebox.IsValid() && Authority.IsValid()
        && Breakdown.IsValid() && Roadside.IsValid() && Logistics.IsValid() && Economy.IsValid() && DayNight.IsValid();
}

void UGTTFarmCargoDispatchEvidenceSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    UE_LOG(LogGTT, Error, TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTFarmCargoDispatchEvidenceSubsystem::RestoreBaselineState()
{
    UWorld* World = GetWorld();
    if (!World || !bBaselineCaptured) return;
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
        Authority->ClearLoadedVehicle(TEXT("dispatch-runtime-evidence-cleanup"));
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

void UGTTFarmCargoDispatchEvidenceSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bAuthorityCleared = Authority.IsValid() && !Authority->HasBoundCargoVehicle()
        && Authority->GetBoundCargoVehicleId().IsNone();
    const bool bPass = bSequenceHealthy && bAccepted && bPickupBound
        && bPatchRequestLocked && bPatchEtaAdvanced && bPatchCancelledNoCharge
        && bTowRequestLocked && bTowEtaAdvanced && bTowCancelledNoCharge
        && bPatchRerequested && bPatchCompleted && bPatchChargeMatched
        && bExactVehiclePreserved && bTimerContinued && bIntegrityNotImproved
        && bWrongVehicleRejected && bHillHandoff && bFinalHandoff && bSaveVerified && bAuthorityCleared
        && InitialPatchQuote > 0 && TowQuote > 0 && FinalPatchQuote > 0
        && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0;

    UE_LOG(LogGTT, Log,
        TEXT("FARM_CARGO_DISPATCH_RUNTIME_COMPLETE result=%s route=feed-dispatch-cancel-rerequest-hill-wood accepted=%d pickup=%d patch_locked=%d patch_eta_advanced=%d patch_cancel_no_charge=%d tow_locked=%d tow_eta_advanced=%d tow_cancel_no_charge=%d patch_rerequested=%d patch_complete=%d patch_charge_matched=%d exact_vehicle=%d timer_continued=%d integrity_not_improved=%d wrong_vehicle_rejected=%d hill=%d final=%d save=%d authority_cleared=%d initial_patch_quote=%d tow_quote=%d final_patch_quote=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d vehicle=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bAccepted ? 1 : 0, bPickupBound ? 1 : 0,
        bPatchRequestLocked ? 1 : 0, bPatchEtaAdvanced ? 1 : 0, bPatchCancelledNoCharge ? 1 : 0,
        bTowRequestLocked ? 1 : 0, bTowEtaAdvanced ? 1 : 0, bTowCancelledNoCharge ? 1 : 0,
        bPatchRerequested ? 1 : 0, bPatchCompleted ? 1 : 0, bPatchChargeMatched ? 1 : 0,
        bExactVehiclePreserved ? 1 : 0, bTimerContinued ? 1 : 0, bIntegrityNotImproved ? 1 : 0,
        bWrongVehicleRejected ? 1 : 0, bHillHandoff ? 1 : 0, bFinalHandoff ? 1 : 0,
        bSaveVerified ? 1 : 0, bAuthorityCleared ? 1 : 0, InitialPatchQuote, TowQuote, FinalPatchQuote,
        PayoutDelta, CargoRunsDelta, ReputationDelta, *LoadedVehicleId.ToString(), Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = EDispatchEvidencePhase::Complete;
    bFinished = true;
}

void UGTTFarmCargoDispatchEvidenceSubsystem::Tick(float DeltaTime)
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
            MarkFailure(TEXT("dispatch-evidence-world-actors-unavailable"));
            FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    UWorld* World = GetWorld();
    AGTTGameMode* GameMode = World ? World->GetAuthGameMode<AGTTGameMode>() : nullptr;

    switch (Phase)
    {
    case EDispatchEvidencePhase::Waiting:
        Phase = EDispatchEvidencePhase::Prepare;
        break;

    case EDispatchEvidencePhase::Prepare:
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
        BaselineWantedHeat = Wanted.IsValid() ? Wanted->GetHeat() : 0.0f;
        BaselineCash = Economy->GetCash();
        BaselineFishCount = Economy->GetFishCount();
        BaselineFishWeightKg = Economy->GetFishWeightKg();
        BaselineMigration = NativeMulebox->GetMigrationSnapshot();
        BaselineBodyDamage = NativeMulebox->GetBodyDamageSnapshot();
        BaselineDetachedMask = NativeMulebox->GetDetachedPanelMask();
        BaselineNativeTransform = NativeMulebox->GetActorTransform();
        bBaselineCaptured = true;

        DayNight->RestoreTime(BaselineDay, 9.0f);
        if (Wanted.IsValid()) Wanted->ClearWanted();
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
            MarkFailure(TEXT("tier2-stock-backed-dispatch-route-unavailable"));
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
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=PREPARE result=PASS route_tier=%d cash_seeded=%d"),
            Logistics->GetCargoRouteTier(), Economy->GetCash());
        Phase = EDispatchEvidencePhase::AcceptContract;
        break;
    }

    case EDispatchEvidencePhase::AcceptContract:
        StartTerminal->Interact_Implementation(PlayerPawn.Get());
        bAccepted = Director->GetStage() == EGTTFarmJobStage::ReachPickup;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=ACCEPT result=%s stage=%s"),
            bAccepted ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()));
        if (!bAccepted) { MarkFailure(TEXT("contract-acceptance-failed")); FinishScenario(TEXT("accept-failed")); return; }
        Phase = EDispatchEvidencePhase::EnterAndPickup;
        break;

    case EDispatchEvidencePhase::EnterAndPickup:
    {
        StageActor(NativeMulebox.Get(), PickupTerminal->GetActorLocation() + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        if (!EnsureNativeDriver()) { MarkFailure(TEXT("native-driver-entry-failed")); FinishScenario(TEXT("pickup-failed")); return; }
        PickupTerminal->Interact_Implementation(PlayerPawn.Get());
        LoadedVehicleId = Authority->GetBoundCargoVehicleId();
        bPickupBound = Director->GetStage() == EGTTFarmJobStage::DeliverCargo
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && LoadedVehicleId == NativeMulebox->GetPersistentVehicleId();
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=PICKUP result=%s stage=%s vehicle=%s timer=%.2f integrity=%.4f"),
            bPickupBound ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()), *LoadedVehicleId.ToString(),
            Director->GetTimeRemaining(), Director->GetCargoIntegrity());
        if (!bPickupBound) { MarkFailure(TEXT("native-exact-vehicle-binding-failed")); FinishScenario(TEXT("pickup-failed")); return; }
        Phase = EDispatchEvidencePhase::RequestPatchContract;
        break;
    }

    case EDispatchEvidencePhase::RequestPatchContract:
    {
        StageActor(NativeMulebox.Get(), PickupTerminal->GetActorLocation() + FVector(1400.0f, 500.0f, 80.0f));
        TimerBeforeDispatch = Director->GetTimeRemaining();
        IntegrityBeforeDispatch = Director->GetCargoIntegrity();
        CashBeforePatchRequest = Economy->GetCash();
        FGTTRoadVehicleMigrationSnapshot Staged = NativeMulebox->GetMigrationSnapshot();
        Staged.ConditionPercent = FMath::Min(Staged.ConditionPercent, 0.19f);
        Staged.TireIntegrity = FMath::Min(Staged.TireIntegrity, 0.20f);
        Staged.FuelLiters = FMath::Max(Staged.FuelLiters, FMath::Min(NativeMulebox->GetFuelCapacityLiters(), 6.0f));
        NativeMulebox->RestorePersistentMigrationSnapshot(Staged);
        const FGTTBreakdownAssessment Assessment = Breakdown->AssessVehicle(NativeMulebox.Get());
        const bool bRequested = Assessment.Recommendation == EGTTBreakdownRecommendation::TowRecommended
            && Assessment.bEmergencyPatchPossible && Roadside->RequestEmergencyRoadsidePatch(NativeMulebox.Get());
        InitialPatchQuote = Roadside->GetPendingRecoveryQuote(NativeMulebox.Get());
        PatchEtaInitial = Roadside->GetPendingRecoverySecondsRemaining(NativeMulebox.Get());
        const FName PendingId = Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get());
        bPatchRequestLocked = bRequested && Roadside->GetPendingRecoveryMode(NativeMulebox.Get()) == EGTTRoadsideRecoveryMode::EmergencyPatch
            && InitialPatchQuote > 0 && PatchEtaInitial > 0.0f && PendingId == LoadedVehicleId
            && Economy->GetCash() == CashBeforePatchRequest;
        PatchRequestedAt = Elapsed;
        FGTTRoadVehicleMigrationSnapshot Mutated = NativeMulebox->GetMigrationSnapshot();
        Mutated.ConditionPercent = FMath::Min(Mutated.ConditionPercent, 0.17f);
        Mutated.TireIntegrity = FMath::Min(Mutated.TireIntegrity, 0.18f);
        NativeMulebox->RestorePersistentMigrationSnapshot(Mutated);
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=PATCH_REQUEST result=%s quote_locked=%d locked_quote=%d eta_initial=%.3f target_pinned=%d no_charge_before_arrival=%d vehicle=%s"),
            bPatchRequestLocked ? TEXT("PASS") : TEXT("FAIL"), bPatchRequestLocked ? 1 : 0, InitialPatchQuote, PatchEtaInitial,
            PendingId == LoadedVehicleId ? 1 : 0, Economy->GetCash() == CashBeforePatchRequest ? 1 : 0, *LoadedVehicleId.ToString());
        if (!bPatchRequestLocked) { MarkFailure(TEXT("patch-dispatch-contract-not-locked")); FinishScenario(TEXT("patch-request-failed")); return; }
        Phase = EDispatchEvidencePhase::ObservePatchContract;
        break;
    }

    case EDispatchEvidencePhase::ObservePatchContract:
        if (Elapsed - PatchRequestedAt < DispatchObservationSeconds) break;
        PatchEtaObserved = Roadside->GetPendingRecoverySecondsRemaining(NativeMulebox.Get());
        bPatchEtaAdvanced = Roadside->IsRoadsidePatchPending(NativeMulebox.Get())
            && Roadside->GetPendingRecoveryQuote(NativeMulebox.Get()) == InitialPatchQuote
            && Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId
            && PatchEtaObserved >= 0.0f && PatchEtaObserved < PatchEtaInitial
            && Economy->GetCash() == CashBeforePatchRequest;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=PATCH_OBSERVE result=%s locked_quote=%d eta_initial=%.3f eta_observed=%.3f same_vehicle=%d no_charge=%d"),
            bPatchEtaAdvanced ? TEXT("PASS") : TEXT("FAIL"), Roadside->GetPendingRecoveryQuote(NativeMulebox.Get()),
            PatchEtaInitial, PatchEtaObserved, Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId ? 1 : 0,
            Economy->GetCash() == CashBeforePatchRequest ? 1 : 0);
        if (!bPatchEtaAdvanced) { MarkFailure(TEXT("patch-locked-quote-or-live-eta-regressed")); FinishScenario(TEXT("patch-observe-failed")); return; }
        Phase = EDispatchEvidencePhase::CancelPatch;
        break;

    case EDispatchEvidencePhase::CancelPatch:
        bPatchCancelledNoCharge = Roadside->CancelPendingRoadsideService(NativeMulebox.Get());
        CashAfterPatchCancel = Economy->GetCash();
        bPatchCancelledNoCharge = bPatchCancelledNoCharge && !Roadside->HasPendingRoadsideService(NativeMulebox.Get())
            && Roadside->GetPendingRecoveryQuote(NativeMulebox.Get()) == 0
            && Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()).IsNone()
            && CashAfterPatchCancel == CashBeforePatchRequest;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=PATCH_CANCEL result=%s cancelled=1 charged=NO cash_delta=%d"),
            bPatchCancelledNoCharge ? TEXT("PASS") : TEXT("FAIL"), CashBeforePatchRequest - CashAfterPatchCancel);
        if (!bPatchCancelledNoCharge) { MarkFailure(TEXT("patch-cancel-or-no-charge-contract-failed")); FinishScenario(TEXT("patch-cancel-failed")); return; }
        Phase = EDispatchEvidencePhase::RequestTowContract;
        break;

    case EDispatchEvidencePhase::RequestTowContract:
    {
        CashBeforeTowRequest = Economy->GetCash();
        const bool bRequested = Roadside->RequestRoadsideTow(NativeMulebox.Get());
        TowQuote = Roadside->GetPendingRecoveryQuote(NativeMulebox.Get());
        TowEtaInitial = Roadside->GetPendingRecoverySecondsRemaining(NativeMulebox.Get());
        const FName PendingId = Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get());
        bTowRequestLocked = bRequested && Roadside->GetPendingRecoveryMode(NativeMulebox.Get()) == EGTTRoadsideRecoveryMode::RoadsideAssistance
            && TowQuote > 0 && TowEtaInitial > 0.0f && PendingId == LoadedVehicleId && Economy->GetCash() == CashBeforeTowRequest;
        TowRequestedAt = Elapsed;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=TOW_REQUEST result=%s quote_locked=%d locked_quote=%d eta_initial=%.3f target_pinned=%d no_charge_before_arrival=%d vehicle=%s"),
            bTowRequestLocked ? TEXT("PASS") : TEXT("FAIL"), bTowRequestLocked ? 1 : 0, TowQuote, TowEtaInitial,
            PendingId == LoadedVehicleId ? 1 : 0, Economy->GetCash() == CashBeforeTowRequest ? 1 : 0, *LoadedVehicleId.ToString());
        if (!bTowRequestLocked) { MarkFailure(TEXT("tow-dispatch-contract-not-locked")); FinishScenario(TEXT("tow-request-failed")); return; }
        Phase = EDispatchEvidencePhase::ObserveTowContract;
        break;
    }

    case EDispatchEvidencePhase::ObserveTowContract:
        if (Elapsed - TowRequestedAt < DispatchObservationSeconds) break;
        TowEtaObserved = Roadside->GetPendingRecoverySecondsRemaining(NativeMulebox.Get());
        bTowEtaAdvanced = Roadside->IsRoadsideTowPending(NativeMulebox.Get())
            && Roadside->GetPendingRecoveryQuote(NativeMulebox.Get()) == TowQuote
            && Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId
            && TowEtaObserved >= 0.0f && TowEtaObserved < TowEtaInitial
            && Economy->GetCash() == CashBeforeTowRequest;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=TOW_OBSERVE result=%s locked_quote=%d eta_initial=%.3f eta_observed=%.3f same_vehicle=%d no_charge=%d"),
            bTowEtaAdvanced ? TEXT("PASS") : TEXT("FAIL"), Roadside->GetPendingRecoveryQuote(NativeMulebox.Get()),
            TowEtaInitial, TowEtaObserved, Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId ? 1 : 0,
            Economy->GetCash() == CashBeforeTowRequest ? 1 : 0);
        if (!bTowEtaAdvanced) { MarkFailure(TEXT("tow-locked-quote-or-live-eta-regressed")); FinishScenario(TEXT("tow-observe-failed")); return; }
        Phase = EDispatchEvidencePhase::CancelTow;
        break;

    case EDispatchEvidencePhase::CancelTow:
        bTowCancelledNoCharge = Roadside->CancelPendingRoadsideService(NativeMulebox.Get());
        CashAfterTowCancel = Economy->GetCash();
        bTowCancelledNoCharge = bTowCancelledNoCharge && !Roadside->HasPendingRoadsideService(NativeMulebox.Get())
            && Roadside->GetPendingRecoveryQuote(NativeMulebox.Get()) == 0
            && Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()).IsNone()
            && CashAfterTowCancel == CashBeforeTowRequest;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=TOW_CANCEL result=%s cancelled=1 charged=NO cash_delta=%d"),
            bTowCancelledNoCharge ? TEXT("PASS") : TEXT("FAIL"), CashBeforeTowRequest - CashAfterTowCancel);
        if (!bTowCancelledNoCharge) { MarkFailure(TEXT("tow-cancel-or-no-charge-contract-failed")); FinishScenario(TEXT("tow-cancel-failed")); return; }
        Phase = EDispatchEvidencePhase::ReRequestPatch;
        break;

    case EDispatchEvidencePhase::ReRequestPatch:
    {
        FGTTRoadVehicleMigrationSnapshot Staged = NativeMulebox->GetMigrationSnapshot();
        Staged.ConditionPercent = FMath::Min(Staged.ConditionPercent, 0.19f);
        Staged.TireIntegrity = FMath::Min(Staged.TireIntegrity, 0.20f);
        Staged.FuelLiters = FMath::Max(Staged.FuelLiters, FMath::Min(NativeMulebox->GetFuelCapacityLiters(), 6.0f));
        NativeMulebox->RestorePersistentMigrationSnapshot(Staged);
        const FGTTBreakdownAssessment Assessment = Breakdown->AssessVehicle(NativeMulebox.Get());
        CashBeforeFinalPatch = Economy->GetCash();
        bPatchRerequested = Assessment.bEmergencyPatchPossible && Roadside->RequestEmergencyRoadsidePatch(NativeMulebox.Get());
        FinalPatchQuote = Roadside->GetPendingRecoveryQuote(NativeMulebox.Get());
        bPatchRerequested = bPatchRerequested && FinalPatchQuote > 0
            && Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId;
        PatchRerequestedAt = Elapsed;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=PATCH_REREQUEST result=%s locked_quote=%d target_pinned=%d no_charge_before_arrival=%d"),
            bPatchRerequested ? TEXT("PASS") : TEXT("FAIL"), FinalPatchQuote,
            Roadside->GetPendingRecoveryVehicleId(NativeMulebox.Get()) == LoadedVehicleId ? 1 : 0,
            Economy->GetCash() == CashBeforeFinalPatch ? 1 : 0);
        if (!bPatchRerequested) { MarkFailure(TEXT("patch-rerequest-failed")); FinishScenario(TEXT("patch-rerequest-failed")); return; }
        Phase = EDispatchEvidencePhase::AwaitPatchCompletion;
        break;
    }

    case EDispatchEvidencePhase::AwaitPatchCompletion:
        if (Elapsed - PatchRerequestedAt < PatchCompletionProofSeconds) break;
        if (Roadside->IsRoadsidePatchPending(NativeMulebox.Get()))
        {
            if (Elapsed - PatchRerequestedAt > 6.0f) { MarkFailure(TEXT("rerequested-patch-did-not-complete")); FinishScenario(TEXT("patch-timeout")); }
            break;
        }
        CashAfterFinalPatch = Economy->GetCash();
        TimerAfterPatch = Director->GetTimeRemaining();
        IntegrityAfterPatch = Director->GetCargoIntegrity();
        bPatchCompleted = CashAfterFinalPatch < CashBeforeFinalPatch;
        bPatchChargeMatched = CashBeforeFinalPatch - CashAfterFinalPatch == FinalPatchQuote;
        bExactVehiclePreserved = Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId
            && NativeMulebox->GetPersistentVehicleId() == LoadedVehicleId;
        bTimerContinued = TimerAfterPatch < TimerBeforeDispatch;
        bIntegrityNotImproved = IntegrityAfterPatch <= IntegrityBeforeDispatch + KINDA_SMALL_NUMBER;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=PATCH_COMPLETE result=%s patch_complete=%d charge_matched=%d charged=%d locked_quote=%d exact_vehicle=%d timer_continued=%d integrity_not_improved=%d timer_before=%.2f timer_after=%.2f"),
            (bPatchCompleted && bPatchChargeMatched && bExactVehiclePreserved && bTimerContinued && bIntegrityNotImproved) ? TEXT("PASS") : TEXT("FAIL"),
            bPatchCompleted ? 1 : 0, bPatchChargeMatched ? 1 : 0, CashBeforeFinalPatch - CashAfterFinalPatch, FinalPatchQuote,
            bExactVehiclePreserved ? 1 : 0, bTimerContinued ? 1 : 0, bIntegrityNotImproved ? 1 : 0, TimerBeforeDispatch, TimerAfterPatch);
        if (!bPatchCompleted || !bPatchChargeMatched || !bExactVehiclePreserved || !bTimerContinued || !bIntegrityNotImproved)
        { MarkFailure(TEXT("completed-patch-dispatch-contract-proof-failed")); FinishScenario(TEXT("patch-complete-failed")); return; }
        Phase = EDispatchEvidencePhase::WrongVehicle;
        break;

    case EDispatchEvidencePhase::WrongVehicle:
    {
        const FVector HillLocation = HillTerminal->GetActorLocation();
        StageActor(NativeMulebox.Get(), HillLocation + FVector(ExactVehicleFarOffsetCm, 0.0f, 80.0f));
        StageActor(SpawnedDecoyVan.Get(), HillLocation + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        StageActor(PlayerPawn.Get(), HillLocation + FVector(0.0f, 180.0f, 80.0f));
        const EGTTFarmJobStage Before = Director->GetStage();
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bWrongVehicleRejected = Before == EGTTFarmJobStage::DeliverCargo
            && Director->GetStage() == EGTTFarmJobStage::DeliverCargo
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=WRONG_VEHICLE result=%s rejected=%d vehicle=%s"),
            bWrongVehicleRejected ? TEXT("PASS") : TEXT("FAIL"), bWrongVehicleRejected ? 1 : 0, *LoadedVehicleId.ToString());
        if (!bWrongVehicleRejected) { MarkFailure(TEXT("wrong-vehicle-after-dispatch-was-not-rejected")); FinishScenario(TEXT("wrong-vehicle-failed")); return; }
        Phase = EDispatchEvidencePhase::HillHandoff;
        break;
    }

    case EDispatchEvidencePhase::HillHandoff:
        StageActor(NativeMulebox.Get(), HillTerminal->GetActorLocation() + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        if (!EnsureNativeDriver()) { MarkFailure(TEXT("native-driver-reentry-failed")); FinishScenario(TEXT("hill-failed")); return; }
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bHillHandoff = Director->GetStage() == EGTTFarmJobStage::DeliverFinalStop && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=HILL_HANDOFF result=%s stage=%s same_vehicle=%d"),
            bHillHandoff ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()), Authority->GetBoundCargoVehicleId() == LoadedVehicleId ? 1 : 0);
        if (!bHillHandoff) { MarkFailure(TEXT("hill-handoff-failed")); FinishScenario(TEXT("hill-failed")); return; }
        Phase = EDispatchEvidencePhase::FinalHandoff;
        break;

    case EDispatchEvidencePhase::FinalHandoff:
        StageActor(NativeMulebox.Get(), FinalTerminal->GetActorLocation() + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        FinalTerminal->Interact_Implementation(PlayerPawn.Get());
        bFinalHandoff = Director->GetStage() == EGTTFarmJobStage::Idle;
        PayoutDelta = Economy->GetCash() - CashAfterFinalPatch;
        CargoRunsDelta = Logistics->GetCargoCompletedRuns() - EvidenceCargoRunsBefore;
        ReputationDelta = Logistics->GetReputation() - EvidenceReputationBefore;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=FINAL_HANDOFF result=%s final=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d authority_cleared=%d"),
            (bFinalHandoff && !Authority->HasBoundCargoVehicle() && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0) ? TEXT("PASS") : TEXT("FAIL"),
            bFinalHandoff ? 1 : 0, PayoutDelta, CargoRunsDelta, ReputationDelta, !Authority->HasBoundCargoVehicle() ? 1 : 0);
        if (!bFinalHandoff || Authority->HasBoundCargoVehicle() || PayoutDelta <= 0 || CargoRunsDelta != 1 || ReputationDelta <= 0)
        { MarkFailure(TEXT("final-payout-reputation-or-authority-proof-failed")); FinishScenario(TEXT("final-failed")); return; }
        Phase = EDispatchEvidencePhase::VerifyPersistence;
        break;

    case EDispatchEvidencePhase::VerifyPersistence:
        bSaveVerified = GameMode && GameMode->SaveProgress();
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_DISPATCH_RUNTIME phase=PERSISTENCE result=%s explicit_save=%d"),
            bSaveVerified ? TEXT("PASS") : TEXT("FAIL"), bSaveVerified ? 1 : 0);
        if (!bSaveVerified) MarkFailure(TEXT("post-dispatch-route-save-failed"));
        FinishScenario(TEXT("sequence-complete"));
        break;

    case EDispatchEvidencePhase::Complete:
        break;
    }
}
