#include "Core/GTTFarmCargoEvidenceScenarioSubsystem.h"

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
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTLogisticsReputationSubsystem.h"
#include "GTT.h"

namespace
{
constexpr float StartDelaySeconds = 180.0f;
constexpr float GlobalDeadlineSeconds = 196.0f;
constexpr float LoadedVehicleOffsetCm = 1400.0f;
constexpr float EvidenceParkingOffsetCm = 120.0f;
constexpr int32 MinimumEvidenceRouteTier = 2;

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

void UGTTFarmCargoEvidenceScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bEnabled = FParse::Param(FCommandLine::Get(), TEXT("GTTDemoSmokeScenario"))
        && FParse::Param(FCommandLine::Get(), TEXT("GTTFarmCargoRuntimeScenario"));
    if (bEnabled)
    {
        GTT_LOG( Log,
            TEXT("FARM_CARGO_RUNTIME_BEGIN version=1 route=feed-hill-wood start_delay=%.1f deadline=%.1f exact_vehicle=required wrong_vehicle_probe=required"),
            StartDelaySeconds, GlobalDeadlineSeconds);
    }
}

TStatId UGTTFarmCargoEvidenceScenarioSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmCargoEvidenceScenarioSubsystem, STATGROUP_Tickables);
}

bool UGTTFarmCargoEvidenceScenarioSubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return bEnabled && !bFinished && World && World->IsGameWorld();
}

APawn* UGTTFarmCargoEvidenceScenarioSubsystem::ResolvePlayerPawn() const
{
    const UWorld* World = GetWorld();
    if (!World) return nullptr;
    const APlayerController* PC = World->GetFirstPlayerController();
    return PC ? PC->GetPawn() : nullptr;
}

AGTTFarmJobTerminal* UGTTFarmCargoEvidenceScenarioSubsystem::FindTerminal(uint8 TerminalTypeValue) const
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

AGTTFarmVanPawn* UGTTFarmCargoEvidenceScenarioSubsystem::SpawnEvidenceVan(const TCHAR* Label, const FVector& Location)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    FActorSpawnParameters Params;
    Params.Name = MakeUniqueObjectName(World, AGTTFarmVanPawn::StaticClass(), FName(Label));
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGTTFarmVanPawn* Van = World->SpawnActor<AGTTFarmVanPawn>(Location, FRotator::ZeroRotator, Params);
    if (Van)
    {
        GTT_LOG( Log, TEXT("FARM_CARGO_RUNTIME event=SPAWN_EVIDENCE_VAN label=%s actor=%s"), Label, *Van->GetName());
    }
    return Van;
}

void UGTTFarmCargoEvidenceScenarioSubsystem::StagePawn(AActor* Actor, const FVector& Location, const FRotator& Rotation) const
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

bool UGTTFarmCargoEvidenceScenarioSubsystem::ResolveScenarioActors()
{
    UWorld* World = GetWorld();
    if (!World) return false;

    if (!Director.IsValid())
    {
        Director = Cast<AGTTFarmJobDirector>(UGameplayStatics::GetActorOfClass(World, AGTTFarmJobDirector::StaticClass()));
    }
    if (!PlayerPawn.IsValid()) PlayerPawn = ResolvePlayerPawn();
    if (!StartTerminal.IsValid()) StartTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::Start));
    if (!PickupTerminal.IsValid()) PickupTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::Pickup));
    if (!HillTerminal.IsValid()) HillTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::Finish));
    if (!FinalTerminal.IsValid()) FinalTerminal = FindTerminal(static_cast<uint8>(EGTTFarmJobTerminalType::FinalFinish));
    if (!Authority.IsValid()) Authority = World->GetSubsystem<UGTTFarmCargoAuthoritySubsystem>();
    if (!Logistics.IsValid()) Logistics = World->GetSubsystem<UGTTLogisticsReputationSubsystem>();

    if (!Economy.IsValid() && PlayerPawn.IsValid()) Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn.Get());
    if (!Wanted.IsValid() && PlayerPawn.IsValid()) Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn.Get());

    if (!DayNight.IsValid())
    {
        if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>()) DayNight = GameMode->GetDayNightCycle();
    }

    return Director.IsValid() && PlayerPawn.IsValid() && StartTerminal.IsValid() && PickupTerminal.IsValid()
        && HillTerminal.IsValid() && FinalTerminal.IsValid() && Authority.IsValid() && Logistics.IsValid()
        && Economy.IsValid() && DayNight.IsValid();
}

void UGTTFarmCargoEvidenceScenarioSubsystem::MarkFailure(const TCHAR* Reason)
{
    bSequenceHealthy = false;
    GTT_LOG( Error,
        TEXT("FARM_CARGO_RUNTIME phase=DIAGNOSTIC result=FAIL reason=%s elapsed=%.2f"),
        Reason ? Reason : TEXT("unknown"), Elapsed);
}

void UGTTFarmCargoEvidenceScenarioSubsystem::RestoreBaselineState()
{
    UWorld* World = GetWorld();
    if (!World || !bBaselineCaptured) return;

    if (Logistics.IsValid() && BaselineLogisticsSave)
    {
        Logistics->RestoreFromSave(BaselineLogisticsSave);
    }
    if (Economy.IsValid())
    {
        Economy->RestoreState(BaselineCash, BaselineFishCount, BaselineFishWeightKg);
    }
    if (DayNight.IsValid())
    {
        DayNight->RestoreTime(BaselineDay, BaselineHour);
    }
    if (Wanted.IsValid())
    {
        Wanted->ClearWanted();
        if (BaselineWantedHeat > 0.0f) Wanted->AddHeat(BaselineWantedHeat);
    }
    if (Authority.IsValid()) Authority->ClearLoadedVehicle(TEXT("runtime-evidence-cleanup"));

    if (SpawnedPickupVan.IsValid()) SpawnedPickupVan->Destroy();
    if (SpawnedDecoyVan.IsValid()) SpawnedDecoyVan->Destroy();

    if (AGTTGameMode* GameMode = World->GetAuthGameMode<AGTTGameMode>())
    {
        GameMode->SaveProgress();
    }
}

void UGTTFarmCargoEvidenceScenarioSubsystem::FinishScenario(const TCHAR* Reason)
{
    const bool bAuthorityCleared = Authority.IsValid() && !Authority->HasBoundCargoVehicle();
    const bool bPass = bSequenceHealthy
        && bAccepted
        && bPickupBound
        && bWrongVehicleRejected
        && bHillHandoff
        && bFinalHandoff
        && bSameVehicleMaintained
        && bSaveVerified
        && bAuthorityCleared
        && PayoutDelta > 0
        && CargoRunsDelta == 1
        && ReputationDelta > 0;

    GTT_LOG( Log,
        TEXT("FARM_CARGO_RUNTIME_COMPLETE result=%s route=feed-hill-wood accepted=%d pickup=%d wrong_vehicle_rejected=%d hill=%d final=%d same_vehicle=%d payout_delta=%d cargo_runs_delta=%d reputation_delta=%d save=%d authority_cleared=%d vehicle=%s reason=%s elapsed=%.2f"),
        bPass ? TEXT("PASS") : TEXT("FAIL"), bAccepted ? 1 : 0, bPickupBound ? 1 : 0,
        bWrongVehicleRejected ? 1 : 0, bHillHandoff ? 1 : 0, bFinalHandoff ? 1 : 0,
        bSameVehicleMaintained ? 1 : 0, PayoutDelta, CargoRunsDelta, ReputationDelta,
        bSaveVerified ? 1 : 0, bAuthorityCleared ? 1 : 0,
        *LoadedVehicleId.ToString(), Reason ? Reason : TEXT("unknown"), Elapsed);

    RestoreBaselineState();
    Phase = EFarmCargoEvidencePhase::Complete;
    bFinished = true;
}

void UGTTFarmCargoEvidenceScenarioSubsystem::Tick(float DeltaTime)
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
            MarkFailure(TEXT("farm-cargo-world-actors-unavailable"));
            FinishScenario(TEXT("actors-unavailable"));
        }
        return;
    }

    UWorld* World = GetWorld();
    AGTTGameMode* GameMode = World ? World->GetAuthGameMode<AGTTGameMode>() : nullptr;

    switch (Phase)
    {
    case EFarmCargoEvidencePhase::Waiting:
        Phase = EFarmCargoEvidencePhase::Prepare;
        break;

    case EFarmCargoEvidencePhase::Prepare:
    {
        if (!GameMode || Director->GetStage() != EGTTFarmJobStage::Idle)
        {
            MarkFailure(TEXT("farm-director-not-idle"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }
        if (GameMode->GetWildlifeAlertLevel() > 0)
        {
            MarkFailure(TEXT("wildlife-alert-not-clear"));
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
            MarkFailure(TEXT("tier2-stock-backed-evidence-route-unavailable"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }

        const FVector PlayerLocation = PlayerPawn->GetActorLocation();
        SpawnedPickupVan = SpawnEvidenceVan(TEXT("GTTFarmCargoEvidencePrimary"), PlayerLocation + FVector(120.0f, 0.0f, 80.0f));
        SpawnedDecoyVan = SpawnEvidenceVan(TEXT("GTTFarmCargoEvidenceDecoy"), PlayerLocation + FVector(420.0f, 0.0f, 80.0f));
        if (!SpawnedPickupVan.IsValid() || !SpawnedDecoyVan.IsValid())
        {
            MarkFailure(TEXT("evidence-vehicle-spawn-failed"));
            FinishScenario(TEXT("prepare-failed"));
            return;
        }

        EvidenceCashBefore = Economy->GetCash();
        EvidenceCargoRunsBefore = Logistics->GetCargoCompletedRuns();
        EvidenceReputationBefore = Logistics->GetReputation();
        GTT_LOG( Log,
            TEXT("FARM_CARGO_RUNTIME phase=PREPARE result=PASS route_tier=%d active_order_tier=%d depot_stock=%d hill_demand=%d wood_demand=%d seeded_runs=%d hour=%.2f"),
            Logistics->GetCargoRouteTier(), Logistics->GetActiveCargoOrderTier(), Logistics->GetFeedDepotStock(),
            Logistics->GetHillFarmDemand(), Logistics->GetWoodYardDemand(), SeedRuns, DayNight->GetTimeOfDayHours());
        Phase = EFarmCargoEvidencePhase::AcceptContract;
        break;
    }

    case EFarmCargoEvidencePhase::AcceptContract:
        StartTerminal->Interact_Implementation(PlayerPawn.Get());
        bAccepted = Director->GetStage() == EGTTFarmJobStage::ReachPickup;
        GTT_LOG( Log, TEXT("FARM_CARGO_RUNTIME phase=ACCEPT result=%s stage=%s"),
            bAccepted ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()));
        if (!bAccepted)
        {
            MarkFailure(TEXT("contract-acceptance-failed"));
            FinishScenario(TEXT("accept-failed"));
            return;
        }
        Phase = EFarmCargoEvidencePhase::PickupCargo;
        break;

    case EFarmCargoEvidencePhase::PickupCargo:
    {
        StagePawn(SpawnedPickupVan.Get(), PlayerPawn->GetActorLocation() + FVector(120.0f, 0.0f, 80.0f));
        PickupTerminal->Interact_Implementation(PlayerPawn.Get());
        LoadedVehicle = Authority->GetBoundCargoVehicle();
        LoadedVehicleId = Authority->GetBoundCargoVehicleId();
        bPickupBound = Director->GetStage() == EGTTFarmJobStage::DeliverCargo
            && LoadedVehicle.IsValid() && !LoadedVehicleId.IsNone();
        GTT_LOG( Log,
            TEXT("FARM_CARGO_RUNTIME phase=PICKUP result=%s stage=%s bound_vehicle=%s actor=%s timer=%.1f cargo_integrity=%.3f"),
            bPickupBound ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()), *LoadedVehicleId.ToString(),
            LoadedVehicle.IsValid() ? *LoadedVehicle->GetName() : TEXT("none"), Director->GetTimeRemaining(), Director->GetCargoIntegrity());
        if (!bPickupBound)
        {
            MarkFailure(TEXT("physical-cargo-binding-failed"));
            FinishScenario(TEXT("pickup-failed"));
            return;
        }
        Phase = EFarmCargoEvidencePhase::WrongVehicleProbe;
        break;
    }

    case EFarmCargoEvidencePhase::WrongVehicleProbe:
    {
        const FVector HillLocation = HillTerminal->GetActorLocation();
        StagePawn(LoadedVehicle.Get(), HillLocation + FVector(LoadedVehicleOffsetCm, 0.0f, 80.0f));
        StagePawn(SpawnedDecoyVan.Get(), HillLocation + FVector(EvidenceParkingOffsetCm, 0.0f, 80.0f));
        if (PlayerPawn.Get() != LoadedVehicle.Get()) StagePawn(PlayerPawn.Get(), HillLocation + FVector(0.0f, 180.0f, 80.0f));

        const EGTTFarmJobStage Before = Director->GetStage();
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bWrongVehicleRejected = Before == EGTTFarmJobStage::DeliverCargo
            && Director->GetStage() == EGTTFarmJobStage::DeliverCargo
            && Authority->GetBoundCargoVehicle() == LoadedVehicle.Get();
        GTT_LOG( Log,
            TEXT("FARM_CARGO_RUNTIME phase=WRONG_VEHICLE result=%s decoy=%s bound_vehicle=%s stage=%s"),
            bWrongVehicleRejected ? TEXT("PASS") : TEXT("FAIL"), *SpawnedDecoyVan->GetName(),
            *LoadedVehicleId.ToString(), StageLabel(Director->GetStage()));
        if (!bWrongVehicleRejected)
        {
            MarkFailure(TEXT("wrong-vehicle-handoff-was-not-rejected"));
            FinishScenario(TEXT("wrong-vehicle-probe-failed"));
            return;
        }
        Phase = EFarmCargoEvidencePhase::HillHandoff;
        break;
    }

    case EFarmCargoEvidencePhase::HillHandoff:
    {
        const FVector HillLocation = HillTerminal->GetActorLocation();
        StagePawn(LoadedVehicle.Get(), HillLocation + FVector(EvidenceParkingOffsetCm, 0.0f, 80.0f));
        if (PlayerPawn.Get() != LoadedVehicle.Get()) StagePawn(PlayerPawn.Get(), HillLocation + FVector(0.0f, 180.0f, 80.0f));
        HillTerminal->Interact_Implementation(PlayerPawn.Get());
        bHillHandoff = Director->GetStage() == EGTTFarmJobStage::DeliverFinalStop;
        bSameVehicleMaintained = Authority->GetBoundCargoVehicle() == LoadedVehicle.Get()
            && Authority->GetBoundCargoVehicleId() == LoadedVehicleId;
        GTT_LOG( Log,
            TEXT("FARM_CARGO_RUNTIME phase=HILL_HANDOFF result=%s stage=%s same_vehicle=%d vehicle=%s timer=%.1f cargo_integrity=%.3f"),
            (bHillHandoff && bSameVehicleMaintained) ? TEXT("PASS") : TEXT("FAIL"), StageLabel(Director->GetStage()),
            bSameVehicleMaintained ? 1 : 0, *LoadedVehicleId.ToString(), Director->GetTimeRemaining(), Director->GetCargoIntegrity());
        if (!bHillHandoff || !bSameVehicleMaintained)
        {
            MarkFailure(TEXT("hill-relay-handoff-failed"));
            FinishScenario(TEXT("hill-failed"));
            return;
        }
        Phase = EFarmCargoEvidencePhase::FinalHandoff;
        break;
    }

    case EFarmCargoEvidencePhase::FinalHandoff:
    {
        const FVector FinalLocation = FinalTerminal->GetActorLocation();
        StagePawn(LoadedVehicle.Get(), FinalLocation + FVector(EvidenceParkingOffsetCm, 0.0f, 80.0f));
        if (PlayerPawn.Get() != LoadedVehicle.Get()) StagePawn(PlayerPawn.Get(), FinalLocation + FVector(0.0f, 180.0f, 80.0f));
        FinalTerminal->Interact_Implementation(PlayerPawn.Get());
        bFinalHandoff = Director->GetStage() == EGTTFarmJobStage::Idle;
        PayoutDelta = Economy->GetCash() - EvidenceCashBefore;
        CargoRunsDelta = Logistics->GetCargoCompletedRuns() - EvidenceCargoRunsBefore;
        ReputationDelta = Logistics->GetReputation() - EvidenceReputationBefore;
        const bool bAuthorityCleared = !Authority->HasBoundCargoVehicle();
        GTT_LOG( Log,
            TEXT("FARM_CARGO_RUNTIME phase=FINAL_HANDOFF result=%s stage=%s payout_delta=%d cargo_runs_delta=%d reputation_delta=%d authority_cleared=%d"),
            (bFinalHandoff && bAuthorityCleared && PayoutDelta > 0 && CargoRunsDelta == 1 && ReputationDelta > 0) ? TEXT("PASS") : TEXT("FAIL"),
            StageLabel(Director->GetStage()), PayoutDelta, CargoRunsDelta, ReputationDelta, bAuthorityCleared ? 1 : 0);
        if (!bFinalHandoff || !bAuthorityCleared || PayoutDelta <= 0 || CargoRunsDelta != 1 || ReputationDelta <= 0)
        {
            MarkFailure(TEXT("final-payout-reputation-or-authority-proof-failed"));
            FinishScenario(TEXT("final-failed"));
            return;
        }
        Phase = EFarmCargoEvidencePhase::VerifyPersistence;
        break;
    }

    case EFarmCargoEvidencePhase::VerifyPersistence:
        bSaveVerified = GameMode && GameMode->SaveProgress();
        GTT_LOG( Log, TEXT("FARM_CARGO_RUNTIME phase=PERSISTENCE result=%s explicit_save=%d"),
            bSaveVerified ? TEXT("PASS") : TEXT("FAIL"), bSaveVerified ? 1 : 0);
        if (!bSaveVerified) MarkFailure(TEXT("post-delivery-save-failed"));
        FinishScenario(TEXT("sequence-complete"));
        break;

    case EFarmCargoEvidencePhase::Complete:
        break;
    }
}
