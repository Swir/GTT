#include "World/GTTContractBoardSubsystem.h"

#include "Activities/GTTFarmJobDirector.h"
#include "Activities/GTTHeavyHaulDirector.h"
#include "Activities/GTTRuralWorkDirector.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTContractBoardTerminal.h"

namespace
{
    constexpr int32 ContractDispatchCost = 15;
    constexpr int32 LegacyWorkshopCost = 75;

    const FName FarmCargoJob(TEXT("FarmCargo"));
    const FName HeavyHaulJob(TEXT("HeavyHaul"));
    const FName TimberHaulJob(TEXT("TimberHaul"));
    const FName FieldMowingJob(TEXT("FieldMowing"));
    const FName FieldmasterId(TEXT("RustyFieldmaster60"));

    AGTTFieldmasterNativePawn* ResolveNativeFieldmaster(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AGTTFieldmasterNativePawn> It(World); It; ++It)
        {
            AGTTFieldmasterNativePawn* Native = *It;
            if (IsValid(Native) && Native->IsLegacyTakeoverActive()) return Native;
        }
        return nullptr;
    }
}

void UGTTContractBoardSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!InWorld.IsGameWorld()) return;

    struct FBoardSpawn { FName JobTag; FVector Location; };
    const FBoardSpawn Boards[] =
    {
        { FarmCargoJob, FVector(-2850.0f, -470.0f, 55.0f) },
        { HeavyHaulJob, FVector(-2650.0f, -470.0f, 55.0f) },
        { TimberHaulJob, FVector(-2450.0f, -470.0f, 55.0f) },
        { FieldMowingJob, FVector(-2250.0f, -470.0f, 55.0f) }
    };

    for (const FBoardSpawn& Entry : Boards)
    {
        if (AGTTContractBoardTerminal* Board = InWorld.SpawnActor<AGTTContractBoardTerminal>(Entry.Location, FRotator::ZeroRotator))
        {
            Board->Configure(Entry.JobTag);
        }
    }
}

FString UGTTContractBoardSubsystem::ContractTitle(FName JobTag)
{
    if (JobTag == FarmCargoJob) return TEXT("Feed Cargo Run");
    if (JobTag == HeavyHaulJob) return TEXT("Heavy Timber Haul");
    if (JobTag == TimberHaulJob) return TEXT("Timber Delivery");
    if (JobTag == FieldMowingJob) return TEXT("Field Mowing");
    return TEXT("Unknown Contract");
}

int32 UGTTContractBoardSubsystem::ContractBaseReward(FName JobTag)
{
    if (JobTag == FarmCargoJob) return 220;
    if (JobTag == HeavyHaulJob) return 900;
    if (JobTag == TimberHaulJob) return 340;
    if (JobTag == FieldMowingJob) return 390;
    return 0;
}

int32 UGTTContractBoardSubsystem::ContractMaximumReward(FName JobTag)
{
    if (JobTag == FarmCargoJob) return 355; // base + fast + Mulebox role bonus
    if (JobTag == HeavyHaulJob) return 1150;
    if (JobTag == TimberHaulJob) return 450;
    if (JobTag == FieldMowingJob) return 470;
    return 0;
}

bool UGTTContractBoardSubsystem::IsHardFleetRequirement(FName JobTag)
{
    return JobTag == HeavyHaulJob || JobTag == FieldMowingJob;
}

int32 UGTTContractBoardSubsystem::CalculateServiceEstimate(const FGTTGarageFleetSnapshot& Snapshot)
{
    const bool bMechanicalService = Snapshot.RepairEstimate > 0 || Snapshot.ConditionPercent < 0.999f ||
        Snapshot.BodyHealth < 0.999f || Snapshot.ServiceStatus == TEXT("SERVICE") || Snapshot.ServiceStatus == TEXT("LIMP") ||
        Snapshot.ServiceStatus == TEXT("TOW") || Snapshot.ServiceStatus == TEXT("IMMOBILE");

    if (bMechanicalService)
    {
        return Snapshot.RepairEstimate > 0 ? Snapshot.RepairEstimate : LegacyWorkshopCost;
    }

    return FMath::Max(0, Snapshot.FuelEstimate) + FMath::Max(0, Snapshot.TireServiceEstimate);
}

FGTTContractBoardOffer UGTTContractBoardSubsystem::BuildOffer(FName JobTag) const
{
    FGTTContractBoardOffer Offer;
    Offer.JobTag = JobTag;
    Offer.Title = ContractTitle(JobTag);
    Offer.RequiredRole = UGTTGarageFleetSubsystem::RecommendedRoleForJob(JobTag);
    Offer.bHardFleetRequirement = IsHardFleetRequirement(JobTag);
    Offer.BaseReward = ContractBaseReward(JobTag);
    Offer.MaximumReward = ContractMaximumReward(JobTag);
    Offer.MaximumNetReward = Offer.MaximumReward;

    const UGTTGarageFleetSubsystem* Fleet = GetWorld() ? GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>() : nullptr;
    if (!Fleet || Offer.RequiredRole == EGTTGarageFleetRole::Utility)
    {
        Offer.Fleet.JobTag = JobTag;
        Offer.Fleet.Readiness = EGTTFleetMissionReadiness::Unavailable;
        Offer.Fleet.Reason = TEXT("Fleet registry unavailable for this contract.");
        return Offer;
    }

    Offer.Fleet = Fleet->AssessJobReadiness(JobTag);
    FGTTGarageFleetSnapshot Snapshot;
    if (Offer.Fleet.AssignedSlot >= 0 && Fleet->GetSlotSnapshot(Offer.Fleet.AssignedSlot, Snapshot))
    {
        Offer.ServiceEstimate = CalculateServiceEstimate(Snapshot);
        Offer.DispatchEstimate = Snapshot.bPreferredDispatch ? 0 : ContractDispatchCost;
    }

    Offer.PreparationEstimate = Offer.ServiceEstimate + Offer.DispatchEstimate;
    Offer.MaximumNetReward = FMath::Max(0, Offer.MaximumReward - Offer.PreparationEstimate);
    Offer.bCanAcceptNow = Offer.Fleet.Readiness == EGTTFleetMissionReadiness::Ready && Offer.DispatchEstimate == 0;
    Offer.bNeedsPreparation = !Offer.bCanAcceptNow && Offer.Fleet.Readiness != EGTTFleetMissionReadiness::Unavailable;
    return Offer;
}

bool UGTTContractBoardSubsystem::IsLegalWorkLocked(APawn* PlayerPawn, FString& OutReason) const
{
    OutReason.Reset();
    if (!PlayerPawn)
    {
        OutReason = TEXT("Player unavailable.");
        return true;
    }
    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        if (Wanted->GetWantedLevel() > 0)
        {
            OutReason = TEXT("Lose the police before using the legal contract board.");
            return true;
        }
    }
    if (const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(PlayerPawn)))
    {
        if (GameMode->GetWildlifeAlertLevel() > 0)
        {
            OutReason = TEXT("Clear the game-warden alert before using the legal contract board.");
            return true;
        }
    }
    return false;
}

bool UGTTContractBoardSubsystem::DispatchAssignedVehicle(const FGTTGarageFleetSnapshot& Snapshot, const FTransform& Destination, FString& OutFailure) const
{
    OutFailure.Reset();
    if (!GetWorld())
    {
        OutFailure = TEXT("World unavailable.");
        return false;
    }

    UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>();
    AGTTVehicleBase* Legacy = Fleet ? Fleet->ResolveLegacyVehicleForSlot(Snapshot.SlotIndex) : nullptr;
    if (!Fleet || !Legacy)
    {
        OutFailure = TEXT("Assigned garage vehicle could not be resolved.");
        return false;
    }

    if (AGTTRoadVehicleNativePawn* NativeRoad = Fleet->FindActiveNativeRoadVehicle(Snapshot.VehicleId))
    {
        if (NativeRoad->GetDriverPawn())
        {
            OutFailure = TEXT("Assigned vehicle is currently occupied.");
            return false;
        }
        if (UChaosWheeledVehicleMovementComponent* Movement = Cast<UChaosWheeledVehicleMovementComponent>(NativeRoad->GetVehicleMovementComponent()))
        {
            Movement->SetThrottleInput(0.0f);
            Movement->SetSteeringInput(0.0f);
            Movement->SetBrakeInput(1.0f);
        }
        NativeRoad->SetActorTransform(Destination, false, nullptr, ETeleportType::TeleportPhysics);
        if (USkeletalMeshComponent* Mesh = NativeRoad->GetMesh())
        {
            Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
            Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        }
        NativeRoad->FlushNativePersistenceMirror();
        return true;
    }

    if (Snapshot.VehicleId == FieldmasterId)
    {
        if (AGTTFieldmasterNativePawn* NativeFieldmaster = ResolveNativeFieldmaster(GetWorld()))
        {
            if (NativeFieldmaster->IsOccupied())
            {
                OutFailure = TEXT("Assigned tractor is currently occupied.");
                return false;
            }
            return NativeFieldmaster->RecallToTransform(Destination);
        }
    }

    if (Legacy->IsOccupied())
    {
        OutFailure = TEXT("Assigned vehicle is currently occupied.");
        return false;
    }
    if (!Legacy->RecallToTransform(Destination))
    {
        OutFailure = TEXT("Assigned vehicle could not be staged.");
        return false;
    }
    return true;
}

bool UGTTContractBoardSubsystem::ServiceAssignedVehicle(const FGTTGarageFleetSnapshot& Snapshot, FString& OutFailure) const
{
    OutFailure.Reset();
    if (!GetWorld()) return false;
    UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>();
    AGTTVehicleBase* Legacy = Fleet ? Fleet->ResolveLegacyVehicleForSlot(Snapshot.SlotIndex) : nullptr;
    if (!Fleet || !Legacy)
    {
        OutFailure = TEXT("Assigned garage vehicle could not be serviced.");
        return false;
    }

    const bool bMechanicalService = Snapshot.RepairEstimate > 0 || Snapshot.ConditionPercent < 0.999f || Snapshot.BodyHealth < 0.999f ||
        Snapshot.ServiceStatus == TEXT("SERVICE") || Snapshot.ServiceStatus == TEXT("LIMP") || Snapshot.ServiceStatus == TEXT("TOW") || Snapshot.ServiceStatus == TEXT("IMMOBILE");

    if (AGTTRoadVehicleNativePawn* NativeRoad = Fleet->FindActiveNativeRoadVehicle(Snapshot.VehicleId))
    {
        if (bMechanicalService)
        {
            if (!NativeRoad->ApplyNativeWorkshopService())
            {
                OutFailure = TEXT("Native workshop service rejected the assigned road vehicle.");
                return false;
            }
        }
        else
        {
            if (Snapshot.TireServiceEstimate > 0) NativeRoad->RepairNativeTires();
            if (Snapshot.FuelEstimate > 0) NativeRoad->RefuelNativeVehicle(NativeRoad->GetFuelCapacityLiters());
        }
        NativeRoad->FlushNativePersistenceMirror();
        return true;
    }

    if (bMechanicalService || Snapshot.TireServiceEstimate > 0 || Snapshot.FuelEstimate > 0)
    {
        if (bMechanicalService) Legacy->RepairVehicle(100000.0f);
        if (bMechanicalService || Snapshot.TireServiceEstimate > 0) Legacy->RepairTires();
        if (bMechanicalService || Snapshot.FuelEstimate > 0) Legacy->RefuelVehicle(100000.0f);
    }

    if (Snapshot.VehicleId == FieldmasterId)
    {
        if (AGTTFieldmasterNativePawn* NativeFieldmaster = ResolveNativeFieldmaster(GetWorld()))
        {
            FString Summary;
            if (!NativeFieldmaster->ImportLegacyGameplayState(Legacy, Summary))
            {
                OutFailure = FString::Printf(TEXT("Native Fieldmaster rejected prepared mirror state: %s"), *Summary);
                return false;
            }
        }
    }
    return true;
}

bool UGTTContractBoardSubsystem::TryPrepareContract(APawn* PlayerPawn, FName JobTag, const FTransform& StagingTransform, FString& OutSummary)
{
    OutSummary.Reset();
    FString LockReason;
    if (IsLegalWorkLocked(PlayerPawn, LockReason))
    {
        OutSummary = LockReason;
        return false;
    }
    if (IsAnyLegalContractActive())
    {
        OutSummary = TEXT("Finish the active legal contract before preparing another vehicle.");
        return false;
    }

    UGTTGarageFleetSubsystem* Fleet = GetWorld() ? GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>() : nullptr;
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
    if (!Fleet || !Economy)
    {
        OutSummary = TEXT("Fleet or economy service unavailable.");
        return false;
    }

    const FGTTContractBoardOffer Offer = BuildOffer(JobTag);
    if (Offer.Fleet.Readiness == EGTTFleetMissionReadiness::Unavailable || Offer.Fleet.AssignedSlot < 0)
    {
        OutSummary = Offer.Fleet.Reason.IsEmpty() ? TEXT("No assigned fleet vehicle is available.") : Offer.Fleet.Reason;
        return false;
    }

    FGTTGarageFleetSnapshot Snapshot;
    if (!Fleet->GetSlotSnapshot(Offer.Fleet.AssignedSlot, Snapshot))
    {
        OutSummary = TEXT("Assigned fleet slot disappeared; refresh the board.");
        return false;
    }
    if (Snapshot.bOccupied)
    {
        OutSummary = TEXT("Assigned loadout is occupied. Park and exit it before contract preparation.");
        return false;
    }
    if (Offer.PreparationEstimate <= 0)
    {
        OutSummary = TEXT("Assigned loadout is already staged and mission-ready.");
        return true;
    }
    if (Economy->GetCash() < Offer.PreparationEstimate)
    {
        OutSummary = FString::Printf(TEXT("Contract prep needs $%d; available cash $%d."), Offer.PreparationEstimate, Economy->GetCash());
        return false;
    }

    const bool bNeedsDispatch = !Snapshot.bPreferredDispatch;
    FString Failure;
    if (bNeedsDispatch && !DispatchAssignedVehicle(Snapshot, StagingTransform, Failure))
    {
        OutSummary = Failure;
        return false;
    }
    if (!Economy->SpendCash(Offer.PreparationEstimate, FString::Printf(TEXT("%s fleet preparation: -$%d"), *Offer.Title, Offer.PreparationEstimate)))
    {
        OutSummary = TEXT("Fleet preparation payment failed.");
        return false;
    }

    if (Offer.ServiceEstimate > 0 && !ServiceAssignedVehicle(Snapshot, Failure))
    {
        Economy->AddCash(Offer.PreparationEstimate, TEXT("Fleet preparation refund"));
        OutSummary = FString::Printf(TEXT("Preparation failed; payment refunded. %s"), *Failure);
        return false;
    }

    if (!Fleet->SetPreferredVehicleId(Snapshot.VehicleId) || !Fleet->SetRoleLoadoutVehicleId(Snapshot.Role, Snapshot.VehicleId))
    {
        Economy->AddCash(Offer.PreparationEstimate, TEXT("Fleet registry preparation refund"));
        OutSummary = TEXT("Fleet registry rejected the prepared loadout; payment refunded.");
        return false;
    }

    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();

    const FGTTContractBoardOffer Updated = BuildOffer(JobTag);
    OutSummary = FString::Printf(TEXT("%s PREPARED: %s staged%s for $%d. Net best-case contract value $%d. Selection and service state saved."),
        *Offer.Title,
        *Snapshot.DisplayName,
        Offer.ServiceEstimate > 0 ? TEXT(", serviced") : TEXT(""),
        Offer.PreparationEstimate,
        Updated.MaximumReward);
    return true;
}

bool UGTTContractBoardSubsystem::IsAnyLegalContractActive() const
{
    if (!GetWorld()) return false;
    for (TActorIterator<AGTTFarmJobDirector> It(GetWorld()); It; ++It) if (It->IsJobActive()) return true;
    for (TActorIterator<AGTTHeavyHaulDirector> It(GetWorld()); It; ++It) if (It->IsActive()) return true;
    for (TActorIterator<AGTTRuralWorkDirector> It(GetWorld()); It; ++It) if (It->IsWorkActive()) return true;
    return false;
}

bool UGTTContractBoardSubsystem::TryAcceptContract(APawn* PlayerPawn, FName JobTag, FString& OutSummary)
{
    OutSummary.Reset();
    FString LockReason;
    if (IsLegalWorkLocked(PlayerPawn, LockReason))
    {
        OutSummary = LockReason;
        return false;
    }
    if (IsAnyLegalContractActive())
    {
        OutSummary = TEXT("Finish the active legal contract before accepting another one.");
        return false;
    }

    const FGTTContractBoardOffer Offer = BuildOffer(JobTag);
    if (!Offer.bCanAcceptNow)
    {
        OutSummary = FString::Printf(TEXT("%s is not ready: %s Use PREP first (estimate $%d)."),
            *Offer.Title, *Offer.Fleet.Reason, Offer.PreparationEstimate);
        return false;
    }

    bool bStarted = false;
    if (JobTag == FarmCargoJob)
    {
        for (TActorIterator<AGTTFarmJobDirector> It(GetWorld()); It; ++It) { bStarted = It->TryStartJob(PlayerPawn); break; }
    }
    else if (JobTag == HeavyHaulJob)
    {
        for (TActorIterator<AGTTHeavyHaulDirector> It(GetWorld()); It; ++It) { bStarted = It->TryStartContract(PlayerPawn); break; }
    }
    else if (JobTag == TimberHaulJob)
    {
        for (TActorIterator<AGTTRuralWorkDirector> It(GetWorld()); It; ++It) { bStarted = It->TryStartTimber(PlayerPawn); break; }
    }
    else if (JobTag == FieldMowingJob)
    {
        for (TActorIterator<AGTTRuralWorkDirector> It(GetWorld()); It; ++It) { bStarted = It->TryStartMowing(PlayerPawn); break; }
    }

    OutSummary = bStarted
        ? FString::Printf(TEXT("%s accepted with %s [%s] mission-ready."), *Offer.Title, *Offer.Fleet.AssignedVehicleName, *UGTTGarageFleetSubsystem::FleetRoleLabel(Offer.RequiredRole))
        : FString::Printf(TEXT("%s could not start; its director rejected the contract state."), *Offer.Title);
    return bStarted;
}
