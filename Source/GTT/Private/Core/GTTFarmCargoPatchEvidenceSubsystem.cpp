#include "Core/GTTFarmCargoPatchEvidenceSubsystem.h"

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
constexpr float StartDelaySeconds = 252.0f;
constexpr float GlobalDeadlineSeconds = 274.0f;
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

bool BodySnapshotMatches(const FGTTRoadBodyDamageSnapshot& Before, const FGTTRoadBodyDamageSnapshot& After, int32 BeforeMask, int32 AfterMask)
{
    return FMath::IsNearlyEqual(Before.FrontHealth, After.FrontHealth, 0.001f)
        && FMath::IsNearlyEqual(Before.RearHealth, After.RearHealth, 0.001f)
        && FMath::IsNearlyEqual(Before.LeftHealth, After.LeftHealth, 0.001f)
        && FMath::IsNearlyEqual(Before.RightHealth, After.RightHealth, 0.001f)
        && FMath::IsNearlyEqual(Before.CoolingStress, After.CoolingStress, 0.001f)
        && Before.DetachedPanelCount == After.DetachedPanelCount
        && BeforeMask == AfterMask;
}
}

void UGTTFarmCargoPatchEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRuntimeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRecoveryScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoBreakdownScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoPatchScenario"));
    if (bEnabled)
    {
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_PATCH_RUNTIME_BEGIN version=1 route=feed-breakdown-patch-hill-wood start_delay=%.1f deadline=%.1f native_mulebox=required exact_vehicle=required paid_patch=required wrong_vehicle_probe=required"),
            StartDelaySeconds, GlobalDeadlineSeconds);
    }
}

TStatId UGTTFarmCargoPatchEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmCargoPatchEvidenceSubsystem, STATGROUP_Tickables);
}

bool UGTTFarmCargoPatchEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

APawn* UGTTFarmCargoPatchEvidenceSubsystem::ResolvePlayerPawn() const
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

AGTTFarmJobTerminal* UGTTFarmCargoPatchEvidenceSubsystem::FindTerminal(uint8 TerminalTypeValue) const
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

AGTTMuleboxNativePawn* UGTTFarmCargoPatchEvidenceSubsystem::FindNativeMulebox() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTMuleboxNativePawn> It(World); It; ++It)
    {
        if (It->IsNativeReady()) return *It;
    }
    return nullptr;
}

AGTTFarmVanPawn* UGTTFarmCargoPatchEvidenceSubsystem::SpawnDecoyVan(const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    FActorSpawnParameters Params;
    Params.Name = MakeUniqueObjectName(World, AGTTFarmVanPawn::StaticClass(), FName(TEXT("GTTFarmCargoPatchDecoy")));
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return World->SpawnActor<AGTTFarmVanPawn>(Location, FRotator::ZeroRotator, Params);
}

void UGTTFarmCargoPatchEvidenceSubsystem::StageActor(AActor* Actor, const FVector& Location, const FRotator& Rotation) const
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

bool UGTTFarmCargoPatchEvidenceSubsystem::EnsureNativeDriver()
{
    UWorld* World = GetWorld();
    if (!World || !NativeMulebox.IsValid() || !PlayerPawn.IsValid()) return false;
    if (UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()) return true;
    if (!NativeMulebox->IsLegacyTakeoverActive() && !NativeMulebox->TryActivateLegacyTakeover()) return false;
    NativeMulebox->Interact_Implementation(PlayerPawn.Get());
    return UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get()
        && NativeMulebox->GetDriverPawn() == PlayerPawn.Get();
}

bool UGTTFarmCargoPatchEvidenceSubsystem::ResolveScenarioActors()
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

void UGTTFarmCargoPatchEvidenceSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    UE_LOG(LogGTT, Error,
        TEXT("FARM_CARGO_PATCH_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTFarmCargoPatchEvidenceSubsystem::RestoreBaselineState()
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
    if (Authority.IsValid()) Authority->ClearLoadedVehicle(TEXT("patch-runtime-evidence-cleanup"));
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

void UGTTFarmCargoPatchEvidenceSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bAuthorityCleared = Authority.IsValid() && !Authority->HasBoundCargoVehicle()
        && Authority->GetBoundCargoVehicleId().IsNone();
    const bool bPass = bSequenceHealthy && bAccepted && bPickupBound && bBreakdownProven && bPatchAvailable
        && bPatchRequested && bPatchCompleted && bIdentityPreserved && bTimerContinued && bIntegrityNotImproved
        && bBodyDamagePreserved && bLimpHomeFloorsApplied && bWrongVehicleRejected && bHillHandoff
        && bFinalHandoff && bSaveVerified && bAuthorityCleared && PatchCostDelta > 0 && PayoutDelta > 0
        && CargoRunsDelta == 1 && ReputationDelta > 0;

    UE_LOG(LogGTT, Log,
        TEXT("FARM_CARGO_PATCH_RUNTIME_COMPLETE result=%s route=feed-breakdown-patch-hill-wood accepted=%d pickup=%d breakdown=%d patch_available=%d patch_requested=%d patch_complete=%d identity_preserved=%d timer_continued=%d integrity_not_improved=%d body_preserved=%d limp_home_floors=%d wrong_vehicle_rejected=%d hill=%d final=%d save=%d authority_cleared=%d patch_cost_delta=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d vehicle=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bAccepted ? 1 : 0, bPickupBound ? 1 : 0,
        bBreakdownProven ? 1 : 0, bPatchAvailable ? 1 : 0, bPatchRequested ? 1 : 0, bPatchCompleted ? 1 : 0,
        bIdentityPreserved ? 1 : 0, bTimerContinued ? 1 : 0, bIntegrityNotImproved ? 1 : 0,
        bBodyDamagePreserved ? 1 : 0, bLimpHomeFloorsApplied ? 1 : 0, bWrongVehicleRejected ? 1 : 0,
        bHillHandoff ? 1 : 0, bFinalHandoff ? 1 : 0, bSaveVerified ? 1 : 0, bAuthorityCleared ? 1 : 0,
        PatchCostDelta, PayoutDelta, CargoRunsDelta, ReputationDelta, *LoadedVehicleId.ToString(),
        Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = EPatchEvidencePhase::Complete;
    bFinished = true;
}

void UGTTFarmCargoPatchEvidenceSubsystem::Tick(float DeltaTime)
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
            MarkFailure(TEXT("patch-evidence-world-actors-unavailable"));
            FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    UWorld* World = GetWorld();
    AGTTGameMode* GameMode = World ? World->GetAuthGameMode<AGTTGameMode>() : nullptr;

    switch (Phase)
    {
    case EPatchEvidencePhase::Waiting:
        Phase = EPatchEvidencePhase::Prepare;
        break;

    case EPatchEvidencePhase::Prepare:
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
            MarkFailure(TEXT("tier2-stock-backed-patch-route-unavailable"));
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
            TEXT("FARM_CARGO_PATCH_RUNTIME phase=PREPARE result=PASS native_ready=1 takeover=1 route_tier=%d active_order_tier=%d cash_seeded=%d hour=%.2f"),
            Logistics->GetCargoRouteTier(), Logistics->GetActiveCargoOrderTier(), Economy->GetCash(), DayNight->GetTimeOfDayHours());
        Phase = EPatchEvidencePhase::AcceptContract;
        break;
    }

    case EPatchEvidencePhase::AcceptContract:
        StartTerminal->Interact_Implementation(PlayerPawn.Get());
        bAccepted = Director->GetStage() == EGTTFarmJobStage::ReachPickup;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_PATCH_RUNTIME phase=ACCEPT result=%s stage=%s"),
            bAccepted ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()));
        if (!bAccepted)
        {
            MarkFailure(TEXT("contract-acceptance-failed"));
            FinishScenario(TEXT("accept-failed"));
            return;
        }
        Phase = EPatchEvidencePhase::EnterAndPickup;
        break;

    case EPatchEvidencePhase::EnterAndPickup:
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
            TEXT("FARM_CARGO_PATCH_RUNTIME phase=PICKUP result=%s stage=%s native_controlled=%d vehicle=%s timer=%.2f integrity=%.4f"),
            bPickupBound ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()),
            UGameplayStatics::GetPlayerPawn(World, 0) == NativeMulebox.Get() ? 1 : 0,
            *LoadedVehicleId.ToString(), Director->GetTimeRemaining(), Director->GetCargoIntegrity());
        if (!bPickupBound)
        {
            MarkFailure(TEXT("native-exact-vehicle-binding-failed"));
            FinishScenario(TEXT("pickup-failed"));
            return;
        }
        Phase = EPatchEvidencePhase::DamageAndRequestPatch;
        break;
    }

    case EPatchEvidencePhase::DamageAndRequestPatch:
    {
        StageActor(NativeMulebox.Get(), PickupTerminal->GetActorLocation() + FVector(1400.0f, 500.0f, 80.0f));
        TimerBeforePatch = Director->GetTimeRemaining();
        IntegrityBeforePatch = Director->GetCargoIntegrity();
        CashBeforePatch = Economy->GetCash();

        const bool bDamageApplied = NativeMulebox->ApplyPoliceSpikeDamage(0.96f, 0.08f);
        const FGTTRoadVehicleMigrationSnapshot DamagedState = NativeMulebox->GetMigrationSnapshot();
        TireIntegrityBeforePatch = DamagedState.TireIntegrity;
        ConditionBeforePatch = DamagedState.ConditionPercent;
        FuelBeforePatch = DamagedState.FuelLiters;
        BodyBeforePatch = NativeMulebox->GetBodyDamageSnapshot();
        DetachedMaskBeforePatch = NativeMulebox->GetDetachedPanelMask();

        const FGTTBreakdownAssessment Assessment = Breakdown->AssessVehicle(NativeMulebox.Get());
        bBreakdownProven = bDamageApplied && DamagedState.TireIntegrity <= 0.08f
            && (Assessment.Recommendation == EGTTBreakdownRecommendation::TowRecommended
                || Assessment.Recommendation == EGTTBreakdownRecommendation::Immobilized);
        bPatchAvailable = bBreakdownProven && Assessment.bEmergencyPatchPossible
            && Breakdown->CanEmergencyPatch(NativeMulebox.Get());
        bPatchRequested = bPatchAvailable && Roadside->RequestEmergencyRoadsidePatch(NativeMulebox.Get());
        PatchRequestedAt = Elapsed;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_PATCH_RUNTIME phase=BREAKDOWN_PATCH_REQUEST result=%s breakdown=%d patch_available=%d patch_requested=%d severity=%.4f tire_before=%.4f condition_before=%.4f fuel_before=%.3f patch_quote=%d timer_before=%.2f integrity_before=%.4f cash_before=%d vehicle=%s"),
            (bBreakdownProven && bPatchAvailable && bPatchRequested) ? TEXT("PASS") : TEXT("FAIL"),
            bBreakdownProven ? 1 : 0, bPatchAvailable ? 1 : 0, bPatchRequested ? 1 : 0,
            Assessment.Severity, TireIntegrityBeforePatch, ConditionBeforePatch, FuelBeforePatch,
            Assessment.EmergencyPatchEstimate, TimerBeforePatch, IntegrityBeforePatch, CashBeforePatch,
            *LoadedVehicleId.ToString());
        if (!bBreakdownProven || !bPatchAvailable || !bPatchRequested)
        {
            MarkFailure(TEXT("native-breakdown-or-paid-patch-request-failed"));
            FinishScenario(TEXT("patch-request-failed"));
            return;
        }
        Phase = EPatchEvidencePhase::AwaitPatch;
        break;
    }

    case EPatchEvidencePhase::AwaitPatch:
    {
        if (Elapsed - PatchRequestedAt < 3.25f) break;
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
        const int32 DetachedMaskAfterPatch = NativeMulebox->GetDetachedPanelMask();

        bPatchCompleted = PatchCostDelta > 0;
        bIdentityPreserved = Authority->GetBoundCargoVehicle() == NativeMulebox.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId
            && NativeMulebox->GetPersistentVehicleId() == LoadedVehicleId;
        bTimerContinued = TimerAfterPatch < TimerBeforePatch;
        bIntegrityNotImproved = IntegrityAfterPatch <= IntegrityBeforePatch + KINDA_SMALL_NUMBER;
        bBodyDamagePreserved = BodySnapshotMatches(BodyBeforePatch, BodyAfterPatch, DetachedMaskBeforePatch, DetachedMaskAfterPatch);
        const float RequiredFuelFloor = FMath::Min(NativeMulebox->GetFuelCapacityLiters(), 5.0f);
        bLimpHomeFloorsApplied = AfterPatch.ConditionPercent + KINDA_SMALL_NUMBER >= FMath::Max(ConditionBeforePatch, 0.30f)
            && AfterPatch.TireIntegrity + KINDA_SMALL_NUMBER >= FMath::Max(TireIntegrityBeforePatch, 0.32f)
            && AfterPatch.FuelLiters + KINDA_SMALL_NUMBER >= FMath::Max(FuelBeforePatch, RequiredFuelFloor);

        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_PATCH_RUNTIME phase=PATCH_COMPLETE result=%s patch_complete=%d identity_preserved=%d timer_continued=%d integrity_not_improved=%d body_preserved=%d limp_home_floors=%d patch_cost_delta=%d timer_before=%.2f timer_after=%.2f integrity_before=%.4f integrity_after=%.4f tire_before=%.4f tire_after=%.4f condition_after=%.4f fuel_after=%.3f vehicle=%s"),
            (bPatchCompleted && bIdentityPreserved && bTimerContinued && bIntegrityNotImproved && bBodyDamagePreserved && bLimpHomeFloorsApplied) ? TEXT("PASS") : TEXT("FAIL"),
            bPatchCompleted ? 1 : 0, bIdentityPreserved ? 1 : 0, bTimerContinued ? 1 : 0,
            bIntegrityNotImproved ? 1 : 0, bBodyDamagePreserved ? 1 : 0, bLimpHomeFloorsApplied ? 1 : 0,
            PatchCostDelta, TimerBeforePatch, TimerAfterPatch, IntegrityBeforePatch, IntegrityAfterPatch,
            TireIntegrityBeforePatch, AfterPatch.TireIntegrity, AfterPatch.ConditionPercent, AfterPatch.FuelLiters,
            *LoadedVehicleId.ToString());
        if (!bPatchCompleted || !bIdentityPreserved || !bTimerContinued || !bIntegrityNotImproved
            || !bBodyDamagePreserved || !bLimpHomeFloorsApplied)
        {
            MarkFailure(TEXT("post-patch-contract-continuity-proof-failed"));
            FinishScenario(TEXT("patch-proof-failed"));
            return;
        }
        Phase = EPatchEvidencePhase::WrongVehicleAfterPatch;
        break;
    }

    case EPatchEvidencePhase::WrongVehicleAfterPatch:
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
            TEXT("FARM_CARGO_PATCH_RUNTIME phase=WRONG_VEHICLE_AFTER_PATCH result=%s wrong_vehicle_rejected=%d stage=%s vehicle=%s"),
            bWrongVehicleRejected ? TEXT("PASS") : TEXT("FAIL"), bWrongVehicleRejected ? 1 : 0,
            StageLabel(Director->GetStage()), *LoadedVehicleId.ToString());
        if (!bWrongVehicleRejected)
        {
            MarkFailure(TEXT("wrong-vehicle-after-patch-was-not-rejected"));
            FinishScenario(TEXT("wrong-vehicle-failed"));
            return;
        }
        Phase = EPatchEvidencePhase::HillHandoff;
        break;
    }

    case EPatchEvidencePhase::HillHandoff:
    {
        const FVector HillLocation = HillTerminal->GetActorLocation();
        StageActor(NativeMulebox.Get(), HillLocation + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        if (!EnsureNativeDriver())
        {
            MarkFailure(TEXT("native-driver-reentry-after-patch-failed"));
            FinishScenario(TEXT("hill-failed"));
            return;
        }
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bHillHandoff = Director->GetStage() == EGTTFarmJobStage::DeliverFinalStop
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_PATCH_RUNTIME phase=HILL_HANDOFF result=%s hill=%d stage=%s same_vehicle=%d timer=%.2f integrity=%.4f"),
            bHillHandoff ? TEXT("PASS") : TEXT("FAIL"), bHillHandoff ? 1 : 0, StageLabel(Director->GetStage()),
            Authority->GetBoundCargoVehicleId() == LoadedVehicleId ? 1 : 0, Director->GetTimeRemaining(), Director->GetCargoIntegrity());
        if (!bHillHandoff)
        {
            MarkFailure(TEXT("hill-handoff-after-patch-failed"));
            FinishScenario(TEXT("hill-failed"));
            return;
        }
        Phase = EPatchEvidencePhase::FinalHandoff;
        break;
    }

    case EPatchEvidencePhase::FinalHandoff:
    {
        const FVector FinalLocation = FinalTerminal->GetActorLocation();
        StageActor(NativeMulebox.Get(), FinalLocation + FVector(HandoffParkingOffsetCm, 0.0f, 80.0f));
        FinalTerminal->Interact_Implementation(PlayerPawn.Get());
        bFinalHandoff = Director->GetStage() == EGTTFarmJobStage::Idle;
        PayoutDelta = Economy->GetCash() - CashAfterPatch;
        CargoRunsDelta = Logistics->GetCargoCompletedRuns() - EvidenceCargoRunsBefore;
        ReputationDelta = Logistics->GetReputation() - EvidenceReputationBefore;
        const bool bAuthorityCleared = !Authority->HasBoundCargoVehicle() && Authority->GetBoundCargoVehicleId().IsNone();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_PATCH_RUNTIME phase=FINAL_HANDOFF result=%s final=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d authority_cleared=%d"),
            (bFinalHandoff && bAuthorityCleared && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0) ? TEXT("PASS") : TEXT("FAIL"),
            bFinalHandoff ? 1 : 0, PayoutDelta, CargoRunsDelta, ReputationDelta, bAuthorityCleared ? 1 : 0);
        if (!bFinalHandoff || !bAuthorityCleared || PayoutDelta <= 0 || CargoRunsDelta != 1 || ReputationDelta <= 0)
        {
            MarkFailure(TEXT("final-payout-reputation-or-authority-proof-failed"));
            FinishScenario(TEXT("final-failed"));
            return;
        }
        Phase = EPatchEvidencePhase::VerifyPersistence;
        break;
    }

    case EPatchEvidencePhase::VerifyPersistence:
        bSaveVerified = GameMode && GameMode->SaveProgress();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_PATCH_RUNTIME phase=PERSISTENCE result=%s explicit_save=%d recovery_state=%s"),
            bSaveVerified ? TEXT("PASS") : TEXT("FAIL"), bSaveVerified ? 1 : 0,
            CargoRecovery.IsValid() ? *CargoRecovery->GetRecoveryStateLabel() : TEXT("unavailable"));
        if (!bSaveVerified) MarkFailure(TEXT("post-patch-route-save-failed"));
        FinishScenario(TEXT("sequence-complete"));
        break;

    case EPatchEvidencePhase::Complete:
        break;
    }
}