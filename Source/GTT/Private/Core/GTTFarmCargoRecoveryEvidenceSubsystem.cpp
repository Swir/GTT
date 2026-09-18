#include "Core/GTTFarmCargoRecoveryEvidenceSubsystem.h"

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
#include "Vehicles/GTTFarmVanPawn.h"
#include "Vehicles/GTTVehicleIdentitySubsystem.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTLogisticsReputationSubsystem.h"
#include "GTT.h"

namespace
{
constexpr float StartDelaySeconds = 200.0f;
constexpr float GlobalDeadlineSeconds = 224.0f;
constexpr float FarVehicleOffsetCm = 1400.0f;
constexpr float ParkingOffsetCm = 120.0f;
constexpr int32 MinimumEvidenceRouteTier = 2;
const FName PrimaryEvidenceVehicleId(TEXT("GTTFarmCargoRecoveryPrimary_0_1_31"));
const FName DecoyEvidenceVehicleId(TEXT("GTTFarmCargoRecoveryDecoy_0_1_31"));

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

bool NearlyRestored(float Actual, float Expected, float Tolerance)
{
    return FMath::Abs(Actual - Expected) <= Tolerance;
}
}

void UGTTFarmCargoRecoveryEvidenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRuntimeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRecoveryScenario"));
    if (bEnabled)
    {
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME_BEGIN version=1 route=feed-hill-wood start_delay=%.1f deadline=%.1f save_load=loaded+relay actor_recreation=required wrong_vehicle_after_reload=required"),
            StartDelaySeconds, GlobalDeadlineSeconds);
    }
}

TStatId UGTTFarmCargoRecoveryEvidenceSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmCargoRecoveryEvidenceSubsystem, STATGROUP_Tickables);
}

bool UGTTFarmCargoRecoveryEvidenceSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

APawn* UGTTFarmCargoRecoveryEvidenceSubsystem::ResolvePlayerPawn() const
{
    const UWorld* World = GetWorld();
    if (!World) return nullptr;
    const APlayerController* PC = World->GetFirstPlayerController();
    return PC ? PC->GetPawn() : nullptr;
}

AGTTFarmJobTerminal* UGTTFarmCargoRecoveryEvidenceSubsystem::FindTerminal(uint8 TerminalTypeValue) const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    const EGTTFarmJobTerminalType DesiredType = static_cast<EGTTFarmJobTerminalType>(TerminalTypeValue);
    for (TActorIterator<AGTTFarmJobTerminal> It(World); It; ++It)
    {
        if (It->GetTerminalType() == DesiredType) return *It;
    }
    return nullptr;
}

AGTTFarmVanPawn* UGTTFarmCargoRecoveryEvidenceSubsystem::SpawnEvidenceVan(
    const TCHAR* Label,
    FName VehicleId,
    const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    FActorSpawnParameters Params;
    Params.Name = MakeUniqueObjectName(World, AGTTFarmVanPawn::StaticClass(), FName(Label));
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGTTFarmVanPawn* Van = World->SpawnActor<AGTTFarmVanPawn>(Location, FRotator::ZeroRotator, Params);
    if (!Van) return nullptr;

    if (!Van->AssignPersistentVehicleIdForInstance(VehicleId))
    {
        UE_LOG(LogGTT, Error,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME event=SPAWN_EVIDENCE_VAN result=FAIL label=%s requested_id=%s reason=id_assignment_rejected"),
            Label, *VehicleId.ToString());
        Van->Destroy();
        return nullptr;
    }

    UE_LOG(LogGTT, Log,
        TEXT("FARM_CARGO_RECOVERY_RUNTIME event=SPAWN_EVIDENCE_VAN result=PASS label=%s actor=%s vehicle=%s"),
        Label, *Van->GetName(), *Van->GetPersistentVehicleId().ToString());
    return Van;
}

void UGTTFarmCargoRecoveryEvidenceSubsystem::StagePawn(
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

bool UGTTFarmCargoRecoveryEvidenceSubsystem::ResolveScenarioActors()
{
    UWorld* World = GetWorld();
    if (!World) return false;

    if (!Director.IsValid())
        Director = Cast<AGTTFarmJobDirector>(UGameplayStatics::GetActorOfClass(World, AGTTFarmJobDirector::StaticClass()));
    if (!PlayerPawn.IsValid()) PlayerPawn = ResolvePlayerPawn();
    if (!StartTerminal.IsValid()) StartTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::Start));
    if (!PickupTerminal.IsValid()) PickupTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::Pickup));
    if (!HillTerminal.IsValid()) HillTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::Finish));
    if (!FinalTerminal.IsValid()) FinalTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::FinalFinish));
    if (!Authority.IsValid()) Authority = World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>();
    if (!Logistics.IsValid()) Logistics = World->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    if (!VehicleIdentity.IsValid()) VehicleIdentity = World->GetSubsystem<UGTTVehicleIdentitySubsystem>();

    if (!Economy.IsValid() && PlayerPawn.IsValid()) Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn.Get());
    if (!Wanted.IsValid() && PlayerPawn.IsValid()) Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn.Get());
    if (!DayNight.IsValid())
    {
        if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) DayNight = GameMode->GetDayNightCycle();
    }

    return Director.IsValid() && PlayerPawn.IsValid() && StartTerminal.IsValid() && PickupTerminal.IsValid()
        && HillTerminal.IsValid() && FinalTerminal.IsValid() && Authority.IsValid() && Logistics.IsValid()
        && Economy.IsValid() && DayNight.IsValid() && VehicleIdentity.IsValid();
}

void UGTTFarmCargoRecoveryEvidenceSubsystem::DisturbCargoRuntimeState()
{
    UGTTSaveGame* EmptySnapshot = NewObject<UGTTSaveGame>(this);
    if (!EmptySnapshot) return;
    Director->RestoreActiveCargoFromSave(EmptySnapshot);
    Authority->RestoreFromSave(EmptySnapshot);
}

void UGTTFarmCargoRecoveryEvidenceSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    UE_LOG(LogGTT, Error,
        TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTFarmCargoRecoveryEvidenceSubsystem::RestoreBaselineState()
{
    UWorld* World = GetWorld();
    if (!World || !bBaselineCaptured) return;

    if (Logistics.IsValid() && BaselineLogisticsSave) Logistics->RestoreFromSave(BaselineLogisticsSave);
    if (Director.IsValid() && BaselineLogisticsSave) Director->RestoreActiveCargoFromSave(BaselineLogisticsSave);
    if (Authority.IsValid()) Authority->ClearLoadedVehicle(TEXT("recovery-runtime-evidence-cleanup"));
    if (Economy.IsValid()) Economy->RestoreState(BaselineCash, BaselineFishCount, BaselineFishWeightKg);
    if (DayNight.IsValid()) DayNight->RestoreTime(BaselineDay, BaselineHour);
    if (Wanted.IsValid())
    {
        Wanted->ClearWanted();
        if (BaselineWantedHeat > 0.0f) Wanted->AddHeat(BaselineWantedHeat);
    }

    if (SpawnedPickupVan.IsValid()) SpawnedPickupVan->Destroy();
    if (SpawnedRecoveredVan.IsValid()) SpawnedRecoveredVan->Destroy();
    if (SpawnedDecoyVan.IsValid()) SpawnedDecoyVan->Destroy();

    if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) GameMode->SaveProgress();
}

void UGTTFarmCargoRecoveryEvidenceSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bAuthorityCleared = Authority.IsValid() && !Authority->HasBoundCargoVehicle()
        && Authority->GetBoundCargoVehicleId().IsNone();
    const bool bPass = bSequenceHealthy
        && bIdentityUnique
        && bAccepted
        && bPickupBound
        && bLoadedCheckpointSaved
        && bLoadedReloadRestored
        && bActorRebound
        && bWrongVehicleRejectedAfterReload
        && bHillHandoff
        && bRelayCheckpointSaved
        && bRelayReloadRestored
        && bSameVehicleAfterRelay
        && bFinalHandoff
        && bCompletionReloadStable
        && bSaveVerified
        && bAuthorityCleared
        && PayoutDelta > 0
        && CargoRunsDelta == 1
        && ReputationDelta > 0;

    UE_LOG(LogGTT, Log,
        TEXT("FARM_CARGO_RECOVERY_RUNTIME_COMPLETE result=%s route=feed-hill-wood identity_unique=%d accepted=%d pickup=%d loaded_save=%d loaded_reload=%d actor_rebound=%d wrong_vehicle_after_reload=%d hill=%d relay_save=%d relay_reload=%d relay_same_vehicle=%d final=%d completion_reload=%d final_save=%d authority_cleared=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d vehicle=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bIdentityUnique ? 1 : 0, bAccepted ? 1 : 0, bPickupBound ? 1 : 0,
        bLoadedCheckpointSaved ? 1 : 0, bLoadedReloadRestored ? 1 : 0, bActorRebound ? 1 : 0,
        bWrongVehicleRejectedAfterReload ? 1 : 0, bHillHandoff ? 1 : 0, bRelayCheckpointSaved ? 1 : 0,
        bRelayReloadRestored ? 1 : 0, bSameVehicleAfterRelay ? 1 : 0, bFinalHandoff ? 1 : 0,
        bCompletionReloadStable ? 1 : 0, bSaveVerified ? 1 : 0, bAuthorityCleared ? 1 : 0,
        PayoutDelta, CargoRunsDelta, ReputationDelta, *LoadedVehicleId.ToString(),
        Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = ERecoveryEvidencePhase::Complete;
    bFinished = true;
}

void UGTTFarmCargoRecoveryEvidenceSubsystem::Tick(float DeltaTime)
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
        if (Elapsed > StartDelaySeconds + 5.0f)
        {
            MarkFailure(TEXT("farm-cargo-recovery-world-actors-unavailable"));
            FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    UWorld* World = GetWorld();
    AGTTGameMode* GameMode = World ? World->GetAuthGameMode<AGTTGameMode>() : nullptr;

    switch (Phase)
    {
    case ERecoveryEvidencePhase::Waiting:
        Phase = ERecoveryEvidencePhase::Prepare;
        break;

    case ERecoveryEvidencePhase::Prepare:
    {
        if (!GameMode || Director->GetStage() != EGTTFarmJobStage::Idle || GameMode->GetWildlifeAlertLevel() > 0)
        {
            MarkFailure(TEXT("farm-director-or-authority-state-not-clean"));
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
        bBaselineCaptured = true;

        DayNight->RestoreTime(BaselineDay, 9.0f);
        if (Wanted.IsValid()) Wanted->ClearWanted();

        int32 SeedRuns = 0;
        while (Logistics->GetCargoRouteTier() < MinimumEvidenceRouteTier && SeedRuns < 3)
        {
            Logistics->RecordCargoSuccess(0, 1.0f, true, false, true);
            ++SeedRuns;
        }
        if (!Logistics->IsCargoDepotWindowOpen() || !Logistics->CanAcceptCargoContract()
            || Logistics->GetCargoRouteTier() < MinimumEvidenceRouteTier
            || Logistics->GetActiveCargoOrderTier() < MinimumEvidenceRouteTier)
        {
            MarkFailure(TEXT("tier2-stock-backed-recovery-route-unavailable"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }

        const FVector PlayerLocation = PlayerPawn->GetActorLocation();
        SpawnedPickupVan = SpawnEvidenceVan(TEXT("GTTFarmCargoRecoveryPrimary"), PrimaryEvidenceVehicleId,
            PlayerLocation + FVector(120.0f, 0.0f, 80.0f));
        SpawnedDecoyVan = SpawnEvidenceVan(TEXT("GTTFarmCargoRecoveryDecoy"), DecoyEvidenceVehicleId,
            PlayerLocation + FVector(430.0f, 0.0f, 80.0f));
        if (!SpawnedPickupVan.IsValid() || !SpawnedDecoyVan.IsValid())
        {
            MarkFailure(TEXT("recovery-evidence-vehicle-spawn-failed"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }

        VehicleIdentity->RefreshFleetIdentity();
        bIdentityUnique = SpawnedPickupVan->GetPersistentVehicleId() != SpawnedDecoyVan->GetPersistentVehicleId()
            && !SpawnedPickupVan->GetPersistentVehicleId().IsNone()
            && !SpawnedDecoyVan->GetPersistentVehicleId().IsNone();
        if (!bIdentityUnique)
        {
            MarkFailure(TEXT("same-model-evidence-vehicle-identity-collision"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }

        EvidenceCashBefore = Economy->GetCash();
        EvidenceCargoRunsBefore = Logistics->GetCargoCompletedRuns();
        EvidenceReputationBefore = Logistics->GetReputation();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=PREPARE result=PASS identity_unique=1 primary=%s decoy=%s route_tier=%d active_order_tier=%d stock=%d seeded_runs=%d"),
            *SpawnedPickupVan->GetPersistentVehicleId().ToString(), *SpawnedDecoyVan->GetPersistentVehicleId().ToString(),
            Logistics->GetCargoRouteTier(), Logistics->GetActiveCargoOrderTier(), Logistics->GetFeedDepotStock(), SeedRuns);
        Phase = ERecoveryEvidencePhase::AcceptContract;
        break;
    }

    case ERecoveryEvidencePhase::AcceptContract:
        StartTerminal->Interact_Implementation(PlayerPawn.Get());
        bAccepted = Director->GetStage() == EGTTFarmJobStage::ReachPickup;
        UE_LOG(LogGTT, Log, TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=ACCEPT result=%s stage=%s"),
            bAccepted ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()));
        if (!bAccepted)
        {
            MarkFailure(TEXT("contract-acceptance-failed"));
            FinishScenario(TEXT("accept-failed"));
            return;
        }
        Phase = ERecoveryEvidencePhase::PickupCargo;
        break;

    case ERecoveryEvidencePhase::PickupCargo:
    {
        StagePawn(SpawnedPickupVan.Get(), PlayerPawn->GetActorLocation() + FVector(120.0f, 0.0f, 80.0f));
        PickupTerminal->Interact_Implementation(PlayerPawn.Get());
        LoadedVehicleId = Authority->GetBoundCargoVehicleId();
        bPickupBound = Director->GetStage() == EGTTFarmJobStage::DeliverCargo
            && Authority->GetBoundCargoVehicle() == SpawnedPickupVan.Get()
            && LoadedVehicleId == SpawnedPickupVan->GetPersistentVehicleId();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=PICKUP result=%s stage=%s bound_vehicle=%s timer=%.2f integrity=%.4f stock=%d"),
            bPickupBound ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()), *LoadedVehicleId.ToString(),
            Director->GetTimeRemaining(), Director->GetCargoIntegrity(), Logistics->GetFeedDepotStock());
        if (!bPickupBound)
        {
            MarkFailure(TEXT("exact-vehicle-pickup-binding-failed"));
            FinishScenario(TEXT("pickup-failed"));
            return;
        }
        Phase = ERecoveryEvidencePhase::SaveLoaded;
        break;
    }

    case ERecoveryEvidencePhase::SaveLoaded:
    {
        LoadedCheckpointTime = Director->GetTimeRemaining();
        LoadedCheckpointIntegrity = Director->GetCargoIntegrity();
        LoadedCheckpointStock = Logistics->GetFeedDepotStock();
        bLoadedCheckpointSaved = GameMode && GameMode->SaveProgress();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=SAVE_LOADED result=%s explicit_save=%d stage=%s vehicle=%s timer=%.2f integrity=%.4f stock=%d"),
            bLoadedCheckpointSaved ? TEXT("PASS") : TEXT("FAIL"), bLoadedCheckpointSaved ? 1 : 0,
            StageLabel(Director->GetStage()), *LoadedVehicleId.ToString(), LoadedCheckpointTime,
            LoadedCheckpointIntegrity, LoadedCheckpointStock);
        if (!bLoadedCheckpointSaved)
        {
            MarkFailure(TEXT("loaded-checkpoint-save-failed"));
            FinishScenario(TEXT("loaded-save-failed"));
            return;
        }

        if (SpawnedPickupVan.IsValid()) SpawnedPickupVan->Destroy();
        SpawnedPickupVan.Reset();
        SpawnedRecoveredVan = SpawnEvidenceVan(TEXT("GTTFarmCargoRecoveryRecreated"), LoadedVehicleId,
            PlayerPawn->GetActorLocation() + FVector(850.0f, 0.0f, 80.0f));
        if (!SpawnedRecoveredVan.IsValid())
        {
            MarkFailure(TEXT("cargo-vehicle-recreation-failed"));
            FinishScenario(TEXT("loaded-save-failed"));
            return;
        }
        VehicleIdentity->RefreshFleetIdentity();
        DisturbCargoRuntimeState();
        Phase = ERecoveryEvidencePhase::ReloadLoaded;
        break;
    }

    case ERecoveryEvidencePhase::ReloadLoaded:
    {
        const bool bLoadPass = GameMode && GameMode->LoadProgress();
        const bool bStageRestored = Director->GetStage() == EGTTFarmJobStage::DeliverCargo;
        const bool bIdRestored = Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        bActorRebound = Authority->GetBoundCargoVehicle() == SpawnedRecoveredVan.Get();
        const bool bTimerRestored = NearlyRestored(Director->GetTimeRemaining(), LoadedCheckpointTime, 2.0f);
        const bool bIntegrityRestored = NearlyRestored(Director->GetCargoIntegrity(), LoadedCheckpointIntegrity, 0.01f);
        const bool bStockStable = Logistics->GetFeedDepotStock() == LoadedCheckpointStock;
        bLoadedReloadRestored = bLoadPass && bStageRestored && bIdRestored && bActorRebound
            && bTimerRestored && bIntegrityRestored && bStockStable;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=RELOAD_LOADED result=%s load=%d stage_restored=%d id_restored=%d actor_rebound=%d timer_restored=%d integrity_restored=%d stock_stable=%d vehicle=%s timer=%.2f integrity=%.4f stock=%d"),
            bLoadedReloadRestored ? TEXT("PASS") : TEXT("FAIL"), bLoadPass ? 1 : 0, bStageRestored ? 1 : 0,
            bIdRestored ? 1 : 0, bActorRebound ? 1 : 0, bTimerRestored ? 1 : 0, bIntegrityRestored ? 1 : 0,
            bStockStable ? 1 : 0, *Authority->GetBoundCargoVehicleId().ToString(), Director->GetTimeRemaining(),
            Director->GetCargoIntegrity(), Logistics->GetFeedDepotStock());
        if (!bLoadedReloadRestored)
        {
            MarkFailure(TEXT("loaded-stage-save-load-recovery-failed"));
            FinishScenario(TEXT("loaded-reload-failed"));
            return;
        }
        Phase = ERecoveryEvidencePhase::WrongVehicleAfterReload;
        break;
    }

    case ERecoveryEvidencePhase::WrongVehicleAfterReload:
    {
        const FVector HillLocation = HillTerminal->GetActorLocation();
        StagePawn(SpawnedRecoveredVan.Get(), HillLocation + FVector(FarVehicleOffsetCm, 0.0f, 80.0f));
        StagePawn(SpawnedDecoyVan.Get(), HillLocation + FVector(ParkingOffsetCm, 0.0f, 80.0f));
        if (PlayerPawn.Get() != SpawnedRecoveredVan.Get()) StagePawn(PlayerPawn.Get(), HillLocation + FVector(0.0f, 180.0f, 80.0f));
        const EGTTFarmJobStage Before = Director->GetStage();
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bWrongVehicleRejectedAfterReload = Before == EGTTFarmJobStage::DeliverCargo
            && Director->GetStage() == EGTTFarmJobStage::DeliverCargo
            && Authority->GetBoundCargoVehicle() == SpawnedRecoveredVan.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=WRONG_VEHICLE_AFTER_RELOAD result=%s wrong_vehicle_rejected=%d decoy=%s bound_vehicle=%s stage=%s"),
            bWrongVehicleRejectedAfterReload ? TEXT("PASS") : TEXT("FAIL"),
            bWrongVehicleRejectedAfterReload ? 1 : 0, *SpawnedDecoyVan->GetPersistentVehicleId().ToString(),
            *Authority->GetBoundCargoVehicleId().ToString(), StageLabel(Director->GetStage()));
        if (!bWrongVehicleRejectedAfterReload)
        {
            MarkFailure(TEXT("wrong-vehicle-after-reload-was-not-rejected"));
            FinishScenario(TEXT("wrong-vehicle-after-reload-failed"));
            return;
        }
        Phase = ERecoveryEvidencePhase::HillHandoff;
        break;
    }

    case ERecoveryEvidencePhase::HillHandoff:
    {
        const FVector HillLocation = HillTerminal->GetActorLocation();
        StagePawn(SpawnedRecoveredVan.Get(), HillLocation + FVector(ParkingOffsetCm, 0.0f, 80.0f));
        if (PlayerPawn.Get() != SpawnedRecoveredVan.Get()) StagePawn(PlayerPawn.Get(), HillLocation + FVector(0.0f, 180.0f, 80.0f));
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bHillHandoff = Director->GetStage() == EGTTFarmJobStage::DeliverFinalStop
            && Authority->GetBoundCargoVehicle() == SpawnedRecoveredVan.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=HILL_HANDOFF result=%s stage=%s same_vehicle=%d vehicle=%s timer=%.2f integrity=%.4f"),
            bHillHandoff ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()), bHillHandoff ? 1 : 0,
            *LoadedVehicleId.ToString(), Director->GetTimeRemaining(), Director->GetCargoIntegrity());
        if (!bHillHandoff)
        {
            MarkFailure(TEXT("hill-handoff-after-reload-failed"));
            FinishScenario(TEXT("hill-failed"));
            return;
        }
        Phase = ERecoveryEvidencePhase::SaveRelay;
        break;
    }

    case ERecoveryEvidencePhase::SaveRelay:
    {
        RelayCheckpointTime = Director->GetTimeRemaining();
        RelayCheckpointIntegrity = Director->GetCargoIntegrity();
        RelayCheckpointStock = Logistics->GetFeedDepotStock();
        bRelayCheckpointSaved = GameMode && GameMode->SaveProgress();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=SAVE_RELAY result=%s explicit_save=%d stage=%s vehicle=%s timer=%.2f integrity=%.4f stock=%d"),
            bRelayCheckpointSaved ? TEXT("PASS") : TEXT("FAIL"), bRelayCheckpointSaved ? 1 : 0,
            StageLabel(Director->GetStage()), *LoadedVehicleId.ToString(), RelayCheckpointTime,
            RelayCheckpointIntegrity, RelayCheckpointStock);
        if (!bRelayCheckpointSaved)
        {
            MarkFailure(TEXT("relay-checkpoint-save-failed"));
            FinishScenario(TEXT("relay-save-failed"));
            return;
        }
        DisturbCargoRuntimeState();
        Phase = ERecoveryEvidencePhase::ReloadRelay;
        break;
    }

    case ERecoveryEvidencePhase::ReloadRelay:
    {
        const bool bLoadPass = GameMode && GameMode->LoadProgress();
        const bool bStageRestored = Director->GetStage() == EGTTFarmJobStage::DeliverFinalStop;
        const bool bIdRestored = Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        bSameVehicleAfterRelay = Authority->GetBoundCargoVehicle() == SpawnedRecoveredVan.Get();
        const bool bTimerRestored = NearlyRestored(Director->GetTimeRemaining(), RelayCheckpointTime, 2.0f);
        const bool bIntegrityRestored = NearlyRestored(Director->GetCargoIntegrity(), RelayCheckpointIntegrity, 0.01f);
        const bool bStockStable = Logistics->GetFeedDepotStock() == RelayCheckpointStock;
        bRelayReloadRestored = bLoadPass && bStageRestored && bIdRestored && bSameVehicleAfterRelay
            && bTimerRestored && bIntegrityRestored && bStockStable;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=RELOAD_RELAY result=%s load=%d stage_restored=%d id_restored=%d relay_same_vehicle=%d timer_restored=%d integrity_restored=%d stock_stable=%d vehicle=%s timer=%.2f integrity=%.4f stock=%d"),
            bRelayReloadRestored ? TEXT("PASS") : TEXT("FAIL"), bLoadPass ? 1 : 0, bStageRestored ? 1 : 0,
            bIdRestored ? 1 : 0, bSameVehicleAfterRelay ? 1 : 0, bTimerRestored ? 1 : 0,
            bIntegrityRestored ? 1 : 0, bStockStable ? 1 : 0, *Authority->GetBoundCargoVehicleId().ToString(),
            Director->GetTimeRemaining(), Director->GetCargoIntegrity(), Logistics->GetFeedDepotStock());
        if (!bRelayReloadRestored)
        {
            MarkFailure(TEXT("relay-stage-save-load-recovery-failed"));
            FinishScenario(TEXT("relay-reload-failed"));
            return;
        }
        Phase = ERecoveryEvidencePhase::FinalHandoff;
        break;
    }

    case ERecoveryEvidencePhase::FinalHandoff:
    {
        const FVector FinalLocation = FinalTerminal->GetActorLocation();
        StagePawn(SpawnedRecoveredVan.Get(), FinalLocation + FVector(ParkingOffsetCm, 0.0f, 80.0f));
        if (PlayerPawn.Get() != SpawnedRecoveredVan.Get()) StagePawn(PlayerPawn.Get(), FinalLocation + FVector(0.0f, 180.0f, 80.0f));
        FinalTerminal->Interact_Implementation(PlayerPawn.Get());
        bFinalHandoff = Director->GetStage() == EGTTFarmJobStage::Idle && !Authority->HasBoundCargoVehicle();
        PayoutDelta = Economy->GetCash() - EvidenceCashBefore;
        CargoRunsDelta = Logistics->GetCargoCompletedRuns() - EvidenceCargoRunsBefore;
        ReputationDelta = Logistics->GetReputation() - EvidenceReputationBefore;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=FINAL_HANDOFF result=%s stage=%s final=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d authority_cleared=%d"),
            (bFinalHandoff && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0) ? TEXT("PASS") : TEXT("FAIL"),
            StageLabel(Director->GetStage()), bFinalHandoff ? 1 : 0, PayoutDelta, CargoRunsDelta, ReputationDelta,
            !Authority->HasBoundCargoVehicle() ? 1 : 0);
        if (!bFinalHandoff || PayoutDelta <= 0 || CargoRunsDelta != 1 || ReputationDelta <= 0)
        {
            MarkFailure(TEXT("final-payout-reputation-or-authority-proof-failed"));
            FinishScenario(TEXT("final-failed"));
            return;
        }
        Phase = ERecoveryEvidencePhase::CompletionReload;
        break;
    }

    case ERecoveryEvidencePhase::CompletionReload:
    {
        const int32 CashBeforeReload = Economy->GetCash();
        const int32 RunsBeforeReload = Logistics->GetCargoCompletedRuns();
        const int32 ReputationBeforeReload = Logistics->GetReputation();
        const bool bLoadPass = GameMode && GameMode->LoadProgress();
        bCompletionReloadStable = bLoadPass
            && Director->GetStage() == EGTTFarmJobStage::Idle
            && !Authority->HasBoundCargoVehicle()
            && Authority->GetBoundCargoVehicleId().IsNone()
            && Economy->GetCash() == CashBeforeReload
            && Logistics->GetCargoCompletedRuns() == RunsBeforeReload
            && Logistics->GetReputation() == ReputationBeforeReload;
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=COMPLETION_RELOAD result=%s load=%d completion_reload_stable=%d stage=%s authority_cleared=%d cash=%d cargo_runs=%d reputation=%d"),
            bCompletionReloadStable ? TEXT("PASS") : TEXT("FAIL"), bLoadPass ? 1 : 0,
            bCompletionReloadStable ? 1 : 0, StageLabel(Director->GetStage()),
            !Authority->HasBoundCargoVehicle() ? 1 : 0, Economy->GetCash(),
            Logistics->GetCargoCompletedRuns(), Logistics->GetReputation());
        if (!bCompletionReloadStable)
        {
            MarkFailure(TEXT("completed-contract-reloaded-or-duplicated"));
            FinishScenario(TEXT("completion-reload-failed"));
            return;
        }
        Phase = ERecoveryEvidencePhase::VerifyPersistence;
        break;
    }

    case ERecoveryEvidencePhase::VerifyPersistence:
        bSaveVerified = GameMode && GameMode->SaveProgress();
        UE_LOG(LogGTT, Log,
            TEXT("FARM_CARGO_RECOVERY_RUNTIME phase=PERSISTENCE result=%s explicit_save=%d"),
            bSaveVerified ? TEXT("PASS") : TEXT("FAIL"), bSaveVerified ? 1 : 0);
        if (!bSaveVerified) MarkFailure(TEXT("post-recovery-final-save-failed"));
        FinishScenario(TEXT("sequence-complete"));
        break;

    case ERecoveryEvidencePhase::Complete:
        break;
    }
}
