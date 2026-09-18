#include "Core/GTTFarmCargoBreakdownEvidenceSubsystem.h"

#include "Activities/GTTFarmCargoAuthoritySubsystem.h"
#include "Activities/GTTFarmCargoBreakdownRecoverySubsystem.h"
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
constexpr float StartDelaySeconds = 228.0f;
constexpr float GlobalDeadlineSeconds = 272.0f;
constexpr float PatchDispatchProofSeconds = 3.25f;
constexpr float PatchCooldownProofSeconds = 12.25f;
constexpr float ExactVehicleFarOffsetCm = 1500.0f;
constexpr float HandoffParkingOffsetCm = 120.0f;
constexpr float MinimumTowMovementCm = 500.0f;
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

bool BodyDamageEqual(const FGTTRoadBodyDamageSnapshot& A, const FGTTRoadBodyDamageSnapshot& B)
{
    return FMath::IsNearlyEqual(A.FrontHealth, B.FrontHealth, 0.001f)
        && FMath::IsNearlyEqual(A.RearHealth, B.RearHealth, 0.001f)
        && FMath::IsNearlyEqual(A.LeftHealth, B.LeftHealth, 0.001f)
        && FMath::IsNearlyEqual(A.RightHealth, B.RightHealth, 0.001f)
        && FMath::IsNearlyEqual(A.CoolingStress, B.CoolingStress, 0.001f)
        && A.DetachedPanelCount == B.DetachedPanelCount;
}
}

void UGTTFarmCargoBreakdownEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRuntimeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRecoveryScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoBreakdownScenario"));
    if (bEnabled)
    {
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME_BEGIN version=2 route=feed-breakdown-patch-tow-hill-wood start_delay=%.1f deadline=%.1f native_mulebox=required exact_vehicle=required emergency_patch=required paid_tow=required wrong_vehicle_probe=required"),
            StartDelaySeconds, GlobalDeadlineSeconds);
    }
}

TStatId UGTTFarmCargoBreakdownEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmCargoBreakdownEvidenceSubsystem, STATGROUP_Tickables);
}

bool UGTTFarmCargoBreakdownEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

APawn* UGTTFarmCargoBreakdownEvidenceSubsystem::ResolvePlayerPawn() const
{
    const UWorld* World = GetWorld();
    if (!World) return nullptr;
    const APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return nullptr;
    APawn* Controlled = PC->GetPawn();
    if (AGTTRoadVehicleNativePawn* Native = Cast<AGTTRoadVehicleNativePawn>(Controlled))
    {
        return Native->GetDriverPawn();
    }
    return Controlled;
}

AGTTFarmJobTerminal* UGTTFarmCargoBreakdownEvidenceSubsystem::FindTerminal(uint8 TerminalTypeValue) const
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

AGTTMuleboxNativePawn* UGTTFarmCargoBreakdownEvidenceSubsystem::FindNativeMulebox() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTMuleboxNativePawn> It(World); It; ++It)
    {
        if (It->IsNativeReady()) return *It;
    }
    return nullptr;
}

AGTTFarmVanPawn* UGTTFarmCargoBreakdownEvidenceSubsystem::SpawnDecoyVan(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    FActorSpawnParameters Params;
    Params.Name = MakeUniqueObjectName(World, AGTTFarmVanPawn::StaticClass(), FName(TEXT("GTTFarmCargoBreakdownDecoy")));
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return World->SpawnActor<AGTTFarmVanPawn>(Location, FRotator::ZeroRotator, Params);
}

void UGTTFarmCargoBreakdownEvidenceSubsystem::StageActor(AActor* Actor, const FVector& Location, const FRotator& Rotation) const
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

bool UGTTFarmCargoBreakdownEvidenceSubsystem::EnsureNativeDriver()
{
    UWorld* World = GetWorld();
    if (!World || !NativeMulebox.IsValid() || !PlayerPawn.IsValid()) return false;
    if (UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()) return true;
    if (!NativeMulebox->IsLegacyTakeoverActive() && !NativeMulebox->TryActivateLegacyTakeover()) return false;
    NativeMulebox->Interact_Implementation(PlayerPawn.Get());
    return UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()
        && NativeMulebox->GetDriverPawn() == PlayerPawn.Get();
}

bool UGTTFarmCargoBreakdownEvidenceSubsystem::ResolveScenarioActors()
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
    if (!CargoRecovery.IsValid()) CargoRecovery = World->GetSubsystem<UGTTFarmCargoBreakdownRecoverySubsystem>();
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
        && CargoRecovery.IsValid() && Breakdown.IsValid() && Roadside.IsValid() && Logistics.IsValid()
        && Economy.IsValid() && DayNight.IsValid();
}

void UGTTFarmCargoBreakdownEvidenceSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    UE_LOG(LogGTT, Error,
        TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTFarmCargoBreakdownEvidenceSubsystem::RestoreBaselineState()
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
    if (Authority.IsValid()) Authority->ClearLoadedVehicle(TEXT("breakdown-runtime-evidence-cleanup"));
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

void UGTTFarmCargoBreakdownEvidenceSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bAuthorityCleared = Authority.IsValid() && !Authority->HasBoundCargoVehicle()
        && Authority->GetBoundCargoVehicleId().IsNone();
    const bool bPass = bSequenceHealthy && bAccepted && bPickupBound
        && bPatchBreakdownProven && bPatchRequested && bPatchCompleted && bPatchIdentityPreserved
        && bPatchBodyPreserved && bPatchTimerContinued && bPatchIntegrityNotImproved && bPatchWorkshopStillRequired
        && bBreakdownProven && bTowRequested && bTowCompleted && bIdentityPreserved
        && bTimerContinued && bIntegrityNotImproved && bDamagePreserved
        && bWrongVehicleRejected && bHillHandoff && bFinalHandoff && bSaveVerified && bAuthorityCleared
        && PatchCostDelta > 0 && TowCostDelta > 0 && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0;

    UE_LOG(LogGTT, Log,
        TEXT("FARM_CARGO_BREAKDOWN_RUNTIME_COMPLETE result=%s route=feed-breakdown-patch-tow-hill-wood accepted=%d pickup=%d patch_breakdown=%d patch_requested=%d patch_complete=%d patch_identity_preserved=%d patch_body_preserved=%d patch_timer_continued=%d patch_integrity_not_improved=%d patch_workshop_required=%d breakdown=%d tow_requested=%d tow_complete=%d identity_preserved=%d timer_continued=%d integrity_not_improved=%d damage_preserved=%d wrong_vehicle_rejected=%d hill=%d final=%d save=%d authority_cleared=%d patch_cost_delta=%d tow_cost_delta=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d vehicle=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bAccepted ? 1 : 0, bPickupBound ? 1 : 0,
        bPatchBreakdownProven ? 1 : 0, bPatchRequested ? 1 : 0, bPatchCompleted ? 1 : 0,
        bPatchIdentityPreserved ? 1 : 0, bPatchBodyPreserved ? 1 : 0, bPatchTimerContinued ? 1 : 0,
        bPatchIntegrityNotImproved ? 1 : 0, bPatchWorkshopStillRequired ? 1 : 0,
        bBreakdownProven ? 1 : 0, bTowRequested ? 1 : 0, bTowCompleted ? 1 : 0,
        bIdentityPreserved ? 1 : 0, bTimerContinued ? 1 : 0, bIntegrityNotImproved ? 1 : 0,
        bDamagePreserved ? 1 : 0, bWrongVehicleRejected ? 1 : 0, bHillHandoff ? 1 : 0,
        bFinalHandoff ? 1 : 0, bSaveVerified ? 1 : 0, bAuthorityCleared ? 1 : 0,
        PatchCostDelta, TowCostDelta, PayoutDelta, CargoRunsDelta, ReputationDelta, *LoadedVehicleId.ToString(),
        Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = EBreakdownEvidencePhase::Complete;
    bFinished = true;
}

void UGTTFarmCargoBreakdownEvidenceSubsystem::Tick(float DeltaTime)
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
            MarkFailure(TEXT("breakdown-evidence-world-actors-unavailable"));
            FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    UWorld* World = GetWorld();
    AGTTGameMode* GameMode = World ? World->GetAuthGameMode<AGTTGameMode>() : nullptr;

    switch (Phase)
    {
    case EBreakdownEvidencePhase::Waiting:
        Phase = EBreakdownEvidencePhase::Prepare;
        break;

    case EBreakdownEvidencePhase::Prepare:
    {
        if (!GameMode || Director->GetStage() != EGTTFarmJobStage::Idle || GameMode->GetWildlifeAlertLevel() > 0)
        {
            MarkFailure(TEXT("farm-director-or-legal-work-state-not-clean"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }

        BaselineLogisticsSave = NewObject<UGTTSaveGame>(this);
        Logistics->CaptureToSave(BaselineLogisticsSave);
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
            MarkFailure(TEXT("tier2-stock-backed-breakdown-route-unavailable"));
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
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=PREPARE result=PASS native_ready=1 takeover=1 route_tier=%d active_order_tier=%d cash_seeded=%d hour=%.2f"),
            Logistics->GetCargoRouteTier(), Logistics->GetActiveCargoOrderTier(), Economy->GetCash(), DayNight->GetTimeOfDayHours());
        Phase = EBreakdownEvidencePhase::AcceptContract;
        break;
    }

    case EBreakdownEvidencePhase::AcceptContract:
        StartTerminal->Interact_Implementation(PlayerPawn.Get());
        bAccepted = Director->GetStage() == EGTTFarmJobStage::ReachPickup;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=ACCEPT result=%s stage=%s"),
            bAccepted ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()));
        if (!bAccepted)
        {
            MarkFailure(TEXT("contract-acceptance-failed"));
            FinishScenario(TEXT("accept-failed"));
            return;
        }
        Phase = EBreakdownEvidencePhase::EnterAndPickup;
        break;

    case EBreakdownEvidencePhase::EnterAndPickup:
    {
        const FVector PickupLocation = PickupTerminal->GetActorLocation();
        StageActor(NativeMulebox.Get(), PickupLocation + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        if (!EnsureNativeDriver())
        {
            MarkFailure(TEXT("native-mulebox-driver-entry-failed"));
            FinishScenario(TEXT("pickup-failed"));
            return;
        }
        PickupTerminal->Interact_Implementation(PlayerPawn.Get());
        LoadedVehicleId = Authority->GetBoundCargoVehicleId();
        bPickupBound = Director->GetStage() == EGTTFarmJobStage::DeliverCargo
            && Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && LoadedVehicleId == NativeMulebox->GetPersistentVehicleId();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=PICKUP result=%s stage=%s native_controlled=%d vehicle=%s timer=%.2f integrity=%.4f"),
            bPickupBound ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()),
            UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get() ? 1 : 0,
            *LoadedVehicleId.ToString(), Director->GetTimeRemaining(), Director->GetCargoIntegrity());
        if (!bPickupBound)
        {
            MarkFailure(TEXT("native-exact-vehicle-binding-failed"));
            FinishScenario(TEXT("pickup-failed"));
            return;
        }
        Phase = EBreakdownEvidencePhase::DamageAndRequestPatch;
        break;
    }

    case EBreakdownEvidencePhase::DamageAndRequestPatch:
    {
        StageActor(NativeMulebox.Get(), PickupTerminal->GetActorLocation() + FVector(1400.0f, 500.0f, 80.0f));
        TimerBeforePatch = Director->GetTimeRemaining();
        IntegrityBeforePatch = Director->GetCargoIntegrity();
        CashBeforePatch = Economy->GetCash();
        PatchBodyBefore = NativeMulebox->GetBodyDamageSnapshot();
        PatchDetachedMaskBefore = NativeMulebox->GetDetachedPanelMask();

        FGTTRoadVehicleMigrationSnapshot Staged = NativeMulebox->GetMigrationSnapshot();
        Staged.ConditionPercent = FMath::Min(Staged.ConditionPercent, 0.19f);
        Staged.TireIntegrity = FMath::Min(Staged.TireIntegrity, 0.20f);
        Staged.FuelLiters = FMath::Max(Staged.FuelLiters, FMath::Min(NativeMulebox->GetFuelCapacityLiters(), 6.0f));
        NativeMulebox->RestorePersistentMigrationSnapshot(Staged);

        const FGTTBreakdownAssessment Assessment = Breakdown->AssessVehicle(NativeMulebox.Get());
        bPatchBreakdownProven = Assessment.Recommendation == EGTTBreakdownRecommendation::TowRecommended
            && Assessment.bEmergencyPatchPossible && Assessment.EmergencyPatchEstimate > 0;
        bPatchRequested = bPatchBreakdownProven && Roadside->RequestEmergencyRoadsidePatch(NativeMulebox.Get());
        PatchRequestedAt = Elapsed;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=BREAKDOWN_PATCH_REQUEST result=%s breakdown=%d patch_requested=%d severity=%.4f condition=%.4f tire_integrity=%.4f fuel=%.3f patch_quote=%d tow_quote=%d timer_before=%.2f integrity_before=%.4f cash_before=%d vehicle=%s"),
            (bPatchBreakdownProven && bPatchRequested) ? TEXT("PASS") : TEXT("FAIL"), bPatchBreakdownProven ? 1 : 0,
            bPatchRequested ? 1 : 0, Assessment.Severity, Staged.ConditionPercent, Staged.TireIntegrity, Staged.FuelLiters,
            Assessment.EmergencyPatchEstimate, Assessment.TowEstimate, TimerBeforePatch, IntegrityBeforePatch,
            CashBeforePatch, *LoadedVehicleId.ToString());
        if (!bPatchBreakdownProven || !bPatchRequested)
        {
            MarkFailure(TEXT("tow-recommended-emergency-patch-request-failed"));
            FinishScenario(TEXT("patch-request-failed"));
            return;
        }
        Phase = EBreakdownEvidencePhase::AwaitPatch;
        break;
    }

    case EBreakdownEvidencePhase::AwaitPatch:
    {
        if (Elapsed - PatchRequestedAt < PatchDispatchProofSeconds) break;
        if (Roadside->IsRoadsidePatchPending(NativeMulebox.Get()))
        {
            if (Elapsed - PatchRequestedAt > 6.0f)
            {
                MarkFailure(TEXT("roadside-patch-did-not-complete"));
                FinishScenario(TEXT("patch-timeout"));
            }
            break;
        }

        CashAfterPatch = Economy->GetCash();
        PatchCostDelta = CashBeforePatch - CashAfterPatch;
        TimerAfterPatch = Director->GetTimeRemaining();
        IntegrityAfterPatch = Director->GetCargoIntegrity();
        const FGTTRoadVehicleMigrationSnapshot AfterPatch = NativeMulebox->GetMigrationSnapshot();
        const FGTTRoadBodyDamageSnapshot BodyAfterPatch = NativeMulebox->GetBodyDamageSnapshot();
        ConditionAfterPatch = AfterPatch.ConditionPercent;
        TireIntegrityAfterPatch = AfterPatch.TireIntegrity;
        FuelAfterPatch = AfterPatch.FuelLiters;
        bPatchCompleted = PatchCostDelta > 0 && AfterPatch.ConditionPercent >= 0.299f
            && AfterPatch.TireIntegrity >= 0.319f && AfterPatch.FuelLiters >= FMath::Min(NativeMulebox->GetFuelCapacityLiters(), 5.0f) - 0.01f;
        bPatchIdentityPreserved = Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId
            && NativeMulebox->GetPersistentVehicleId() == LoadedVehicleId;
        bPatchBodyPreserved = BodyDamageEqual(PatchBodyBefore, BodyAfterPatch)
            && PatchDetachedMaskBefore == NativeMulebox->GetDetachedPanelMask();
        bPatchTimerContinued = TimerAfterPatch < TimerBeforePatch;
        bPatchIntegrityNotImproved = IntegrityAfterPatch <= IntegrityBeforePatch + KINDA_SMALL_NUMBER;
        bPatchWorkshopStillRequired = AfterPatch.ConditionPercent < 0.999f || AfterPatch.TireIntegrity < 0.999f
            || BodyAfterPatch.DetachedPanelCount > 0 || !BodyDamageEqual(BodyAfterPatch, FGTTRoadBodyDamageSnapshot());
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=PATCH_COMPLETE result=%s patch_complete=%d identity_preserved=%d body_preserved=%d timer_continued=%d integrity_not_improved=%d workshop_still_required=%d patch_cost_delta=%d timer_before=%.2f timer_after=%.2f integrity_before=%.4f integrity_after=%.4f condition_after=%.4f tire_after=%.4f fuel_after=%.3f vehicle=%s"),
            (bPatchCompleted && bPatchIdentityPreserved && bPatchBodyPreserved && bPatchTimerContinued && bPatchIntegrityNotImproved && bPatchWorkshopStillRequired) ? TEXT("PASS") : TEXT("FAIL"),
            bPatchCompleted ? 1 : 0, bPatchIdentityPreserved ? 1 : 0, bPatchBodyPreserved ? 1 : 0,
            bPatchTimerContinued ? 1 : 0, bPatchIntegrityNotImproved ? 1 : 0, bPatchWorkshopStillRequired ? 1 : 0,
            PatchCostDelta, TimerBeforePatch, TimerAfterPatch, IntegrityBeforePatch, IntegrityAfterPatch,
            ConditionAfterPatch, TireIntegrityAfterPatch, FuelAfterPatch, *LoadedVehicleId.ToString());
        if (!bPatchCompleted || !bPatchIdentityPreserved || !bPatchBodyPreserved || !bPatchTimerContinued
            || !bPatchIntegrityNotImproved || !bPatchWorkshopStillRequired)
        {
            MarkFailure(TEXT("post-patch-cargo-continuity-proof-failed"));
            FinishScenario(TEXT("patch-proof-failed"));
            return;
        }
        PatchCompletedAt = Elapsed;
        Phase = EBreakdownEvidencePhase::AwaitPatchCooldown;
        break;
    }

    case EBreakdownEvidencePhase::AwaitPatchCooldown:
        if (Elapsed - PatchCompletedAt < PatchCooldownProofSeconds) break;
        if (Director->GetStage() != EGTTFarmJobStage::DeliverCargo
            || Authority->GetBoundCargoVehicle() != NativeMulebox.Get()
            || Authority->GetBoundCargoVehicleId() != LoadedVehicleId)
        {
            MarkFailure(TEXT("cargo-authority-changed-during-patch-cooldown"));
            FinishScenario(TEXT("patch-cooldown-failed"));
            return;
        }
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=PATCH_COOLDOWN result=PASS waited=%.2f stage=%s timer=%.2f integrity=%.4f vehicle=%s"),
            Elapsed - PatchCompletedAt, StageLabel(Director->GetStage()), Director->GetTimeRemaining(), Director->GetCargoIntegrity(),
            *LoadedVehicleId.ToString());
        Phase = EBreakdownEvidencePhase::DamageAndRequestTow;
        break;

    case EBreakdownEvidencePhase::DamageAndRequestTow:
    {
        TimerBeforeTow = Director->GetTimeRemaining();
        IntegrityBeforeTow = Director->GetCargoIntegrity();
        CashBeforeTow = Economy->GetCash();
        PreTowLocation = NativeMulebox->GetActorLocation();

        const bool bDamageApplied = NativeMulebox->ApplyPoliceSpikeDamage(0.96f, 0.30f);
        const FGTTRoadVehicleMigrationSnapshot DamagedState = NativeMulebox->GetMigrationSnapshot();
        TireIntegrityAfterDamage = DamagedState.TireIntegrity;
        const FGTTBreakdownAssessment Assessment = Breakdown->AssessVehicle(NativeMulebox.Get());
        bBreakdownProven = bDamageApplied && DamagedState.TireIntegrity <= 0.08f
            && (Assessment.bTowRecommended || Assessment.Recommendation == EGTTBreakdownRecommendation::Immobilized);
        bTowRequested = bBreakdownProven && Roadside->RequestRoadsideTow(NativeMulebox.Get());
        TowRequestedAt = Elapsed;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=BREAKDOWN_TOW_REQUEST result=%s breakdown=%d tow_requested=%d severity=%.4f tire_integrity=%.4f tow_quote=%d timer_before=%.2f integrity_before=%.4f cash_before=%d vehicle=%s"),
            (bBreakdownProven && bTowRequested) ? TEXT("PASS") : TEXT("FAIL"), bBreakdownProven ? 1 : 0,
            bTowRequested ? 1 : 0, Assessment.Severity, TireIntegrityAfterDamage, Assessment.TowEstimate,
            TimerBeforeTow, IntegrityBeforeTow, CashBeforeTow, *LoadedVehicleId.ToString());
        if (!bBreakdownProven || !bTowRequested)
        {
            MarkFailure(TEXT("native-rebreakdown-or-paid-tow-request-failed"));
            FinishScenario(TEXT("tow-request-failed"));
            return;
        }
        Phase = EBreakdownEvidencePhase::AwaitTow;
        break;
    }

    case EBreakdownEvidencePhase::AwaitTow:
    {
        if (Elapsed - TowRequestedAt < 2.75f) break;
        const float MovedCm = FVector::Dist2D(PreTowLocation, NativeMulebox->GetActorLocation());
        if (Roadside->IsRoadsideTowPending(NativeMulebox.Get()) || MovedCm < MinimumTowMovementCm)
        {
            if (Elapsed - TowRequestedAt > 6.0f)
            {
                MarkFailure(TEXT("roadside-tow-did-not-complete"));
                FinishScenario(TEXT("tow-timeout"));
            }
            break;
        }

        CashAfterTow = Economy->GetCash();
        TowCostDelta = CashBeforeTow - CashAfterTow;
        TimerAfterTow = Director->GetTimeRemaining();
        IntegrityAfterTow = Director->GetCargoIntegrity();
        const FGTTRoadVehicleMigrationSnapshot AfterTow = NativeMulebox->GetMigrationSnapshot();
        bTowCompleted = TowCostDelta > 0;
        bIdentityPreserved = Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId
            && NativeMulebox->GetPersistentVehicleId() == LoadedVehicleId;
        bTimerContinued = TimerAfterTow < TimerBeforeTow;
        bIntegrityNotImproved = IntegrityAfterTow <= IntegrityBeforeTow + KINDA_SMALL_NUMBER;
        bDamagePreserved = AfterTow.TireIntegrity <= TireIntegrityAfterDamage + 0.001f;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=TOW_COMPLETE result=%s tow_complete=%d identity_preserved=%d timer_continued=%d integrity_not_improved=%d damage_preserved=%d tow_cost_delta=%d moved_cm=%.1f timer_before=%.2f timer_after=%.2f integrity_before=%.4f integrity_after=%.4f tire_after=%.4f vehicle=%s"),
            (bTowCompleted && bIdentityPreserved && bTimerContinued && bIntegrityNotImproved && bDamagePreserved) ? TEXT("PASS") : TEXT("FAIL"),
            bTowCompleted ? 1 : 0, bIdentityPreserved ? 1 : 0, bTimerContinued ? 1 : 0,
            bIntegrityNotImproved ? 1 : 0, bDamagePreserved ? 1 : 0, TowCostDelta, MovedCm,
            TimerBeforeTow, TimerAfterTow, IntegrityBeforeTow, IntegrityAfterTow, AfterTow.TireIntegrity,
            *LoadedVehicleId.ToString());
        if (!bTowCompleted || !bIdentityPreserved || !bTimerContinued || !bIntegrityNotImproved || !bDamagePreserved)
        {
            MarkFailure(TEXT("post-tow-contract-continuity-proof-failed"));
            FinishScenario(TEXT("tow-proof-failed"));
            return;
        }
        Phase = EBreakdownEvidencePhase::WrongVehicleAfterTow;
        break;
    }

    case EBreakdownEvidencePhase::WrongVehicleAfterTow:
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
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=WRONG_VEHICLE_AFTER_TOW result=%s wrong_vehicle_rejected=%d stage=%s vehicle=%s"),
            bWrongVehicleRejected ? TEXT("PASS") : TEXT("FAIL"), bWrongVehicleRejected ? 1 : 0,
            StageLabel(Director->GetStage()), *LoadedVehicleId.ToString());
        if (!bWrongVehicleRejected)
        {
            MarkFailure(TEXT("wrong-vehicle-after-tow-was-not-rejected"));
            FinishScenario(TEXT("wrong-vehicle-failed"));
            return;
        }
        Phase = EBreakdownEvidencePhase::HillHandoff;
        break;
    }

    case EBreakdownEvidencePhase::HillHandoff:
    {
        const FVector HillLocation = HillTerminal->GetActorLocation();
        StageActor(NativeMulebox.Get(), HillLocation + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        if (!EnsureNativeDriver())
        {
            MarkFailure(TEXT("native-driver-reentry-after-tow-failed"));
            FinishScenario(TEXT("hill-failed"));
            return;
        }
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bHillHandoff = Director->GetStage() == EGTTFarmJobStage::DeliverFinalStop
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=HILL_HANDOFF result=%s hill=%d stage=%s same_vehicle=%d timer=%.2f integrity=%.4f"),
            bHillHandoff ? TEXT("PASS") : TEXT("FAIL"), bHillHandoff ? 1 : 0, StageLabel(Director->GetStage()),
            Authority->GetBoundCargoVehicleId() == LoadedVehicleId ? 1 : 0, Director->GetTimeRemaining(), Director->GetCargoIntegrity());
        if (!bHillHandoff)
        {
            MarkFailure(TEXT("hill-handoff-after-patch-and-tow-failed"));
            FinishScenario(TEXT("hill-failed"));
            return;
        }
        Phase = EBreakdownEvidencePhase::FinalHandoff;
        break;
    }

    case EBreakdownEvidencePhase::FinalHandoff:
    {
        const FVector FinalLocation = FinalTerminal->GetActorLocation();
        StageActor(NativeMulebox.Get(), FinalLocation + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        FinalTerminal->Interact_Implementation(PlayerPawn.Get());
        bFinalHandoff = Director->GetStage() == EGTTFarmJobStage::Idle;
        PayoutDelta = Economy->GetCash() - CashAfterTow;
        CargoRunsDelta = Logistics->GetCargoCompletedRuns() - EvidenceCargoRunsBefore;
        ReputationDelta = Logistics->GetReputation() - EvidenceReputationBefore;
        const bool bAuthorityCleared = !Authority->HasBoundCargoVehicle() && Authority->GetBoundCargoVehicleId().IsNone();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=FINAL_HANDOFF result=%s final=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d authority_cleared=%d"),
            (bFinalHandoff && bAuthorityCleared && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0) ? TEXT("PASS") : TEXT("FAIL"),
            bFinalHandoff ? 1 : 0, PayoutDelta, CargoRunsDelta, ReputationDelta, bAuthorityCleared ? 1 : 0);
        if (!bFinalHandoff || !bAuthorityCleared || PayoutDelta <= 0 || CargoRunsDelta != 1 || ReputationDelta <= 0)
        {
            MarkFailure(TEXT("final-payout-reputation-or-authority-proof-failed"));
            FinishScenario(TEXT("final-failed"));
            return;
        }
        Phase = EBreakdownEvidencePhase::VerifyPersistence;
        break;
    }

    case EBreakdownEvidencePhase::VerifyPersistence:
        bSaveVerified = GameMode && GameMode->SaveProgress();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_BREAKDOWN_RUNTIME phase=PERSISTENCE result=%s explicit_save=%d recovery_state=%s"),
            bSaveVerified ? TEXT("PASS") : TEXT("FAIL"), bSaveVerified ? 1 : 0,
            CargoRecovery.IsValid() ? *CargoRecovery->GetRecoveryStateLabel() : TEXT("unavailable"));
        if (!bSaveVerified) MarkFailure(TEXT("post-breakdown-route-save-failed"));
        FinishScenario(TEXT("sequence-complete"));
        break;

    case EBreakdownEvidencePhase::Complete:
        break;
    }
}
