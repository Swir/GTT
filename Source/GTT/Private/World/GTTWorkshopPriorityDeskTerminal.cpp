#include "World/GTTWorkshopPriorityDeskTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "World/GTTWorkshopRepairQueueSubsystem.h"
#include "GTT.h"

AGTTWorkshopPriorityDeskTerminal::AGTTWorkshopPriorityDeskTerminal()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Mesh->SetRelativeScale3D(FVector(0.30f, 0.24f, 0.82f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
}

FName AGTTWorkshopPriorityDeskTerminal::FindNearestUpgradeableVehicle() const
{
    UWorld* World = GetWorld();
    const UGTTWorkshopRepairQueueSubsystem* Queue = World ? World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>() : nullptr;
    if (!World || !Queue || !Queue->HasQueuedRepair()) return NAME_None;

    const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
    FName BestId = NAME_None;
    float BestDistanceSquared = FMath::Square(FMath::Max(100.0f, NearbyVehicleRadius));
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        const AGTTRoadVehicleNativePawn* Vehicle = *It;
        if (!IsValid(Vehicle) || !Vehicle->IsLegacyTakeoverActive()) continue;
        const FName VehicleId = Vehicle->GetPersistentVehicleId();
        if (VehicleId.IsNone()) continue;

        const FGTTWorkshopRepairQueueSnapshot* Snapshot = Snapshots.FindByPredicate(
            [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Candidate)
            {
                return Candidate.PersistentVehicleId == VehicleId;
            });
        if (!Snapshot || Snapshot->bUrgent || Snapshot->bCheckedIn || Snapshot->bReadyForPickup) continue;

        const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistanceSquared <= BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestId = VehicleId;
        }
    }
    return BestId;
}

void AGTTWorkshopPriorityDeskTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    UGTTPlayerEconomyComponent* Economy = Pawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn) : nullptr;
    UWorld* World = GetWorld();
    UGTTWorkshopRepairQueueSubsystem* Queue = World ? World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>() : nullptr;
    if (!Economy || !World || !Queue) return;

    const float Now = World->GetTimeSeconds();
    if (PendingExpiresAt >= 0.0f && Now > PendingExpiresAt)
    {
        PendingVehicleId = NAME_None;
        PendingExpiresAt = -1.0f;
    }

    const FName VehicleId = FindNearestUpgradeableVehicle();
    if (VehicleId.IsNone())
    {
        PendingVehicleId = NAME_None;
        PendingExpiresAt = -1.0f;
        Economy->PushMessage(FString::Printf(
            TEXT("WORKSHOP PRIORITY DESK | no nearby waiting STANDARD appointment is eligible. URGENT promotion is only available before check-in, adds +%d%% to the locked checkout quote, shortens service time to x%.2f and never pre-charges."),
            UGTTWorkshopRepairQueueSubsystem::UrgentQuoteSurchargePercent,
            UGTTWorkshopRepairQueueSubsystem::UrgentServiceDurationMultiplier), 9.0f);
        return;
    }

    const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
    const FGTTWorkshopRepairQueueSnapshot* Snapshot = Snapshots.FindByPredicate(
        [VehicleId](const FGTTWorkshopRepairQueueSnapshot& Candidate)
        {
            return Candidate.PersistentVehicleId == VehicleId;
        });
    if (!Snapshot) return;

    const int32 UrgentQuote = Queue->GetUrgentQuoteForVehicle(VehicleId);
    if (PendingVehicleId == VehicleId && Now <= PendingExpiresAt)
    {
        FString Summary;
        const bool bPromoted = Queue->PromoteQueuedRepairToUrgent(VehicleId, Summary);
        PendingVehicleId = NAME_None;
        PendingExpiresAt = -1.0f;
        Economy->PushMessage(Summary, 9.0f);
        GTT_LOG( Display,
            TEXT("WORKSHOP_PRIORITY_DESK_CONFIRMED vehicle=%s success=%s charged=NO repair_mutation=NO"),
            *VehicleId.ToString(), bPromoted ? TEXT("YES") : TEXT("NO"));
        return;
    }

    PendingVehicleId = VehicleId;
    PendingExpiresAt = Now + ConfirmationSeconds;
    Economy->PushMessage(FString::Printf(
        TEXT("URGENT upgrade ARMED for exact vehicle %s | STANDARD locked $%d -> URGENT locked $%d (+%d%%) | service time x%.2f. Interact again within %.0f seconds to confirm. No money is charged now."),
        *VehicleId.ToString(), Snapshot->LockedQuote, UrgentQuote,
        UGTTWorkshopRepairQueueSubsystem::UrgentQuoteSurchargePercent,
        UGTTWorkshopRepairQueueSubsystem::UrgentServiceDurationMultiplier,
        ConfirmationSeconds), 10.0f);
    GTT_LOG( Display,
        TEXT("WORKSHOP_PRIORITY_DESK_ARMED vehicle=%s standard_quote=%d urgent_quote=%d timeout=%.1f exact_id=YES charged=NO mutation=NO"),
        *VehicleId.ToString(), Snapshot->LockedQuote, UrgentQuote, ConfirmationSeconds);
}

FText AGTTWorkshopPriorityDeskTerminal::GetInteractionText_Implementation() const
{
    const FName VehicleId = FindNearestUpgradeableVehicle();
    if (VehicleId.IsNone())
    {
        return FText::FromString(FString::Printf(
            TEXT("Workshop priority desk: no eligible STANDARD job | URGENT +%d%% / x%.2f service"),
            UGTTWorkshopRepairQueueSubsystem::UrgentQuoteSurchargePercent,
            UGTTWorkshopRepairQueueSubsystem::UrgentServiceDurationMultiplier));
    }

    UWorld* World = GetWorld();
    const UGTTWorkshopRepairQueueSubsystem* Queue = World ? World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>() : nullptr;
    const int32 UrgentQuote = Queue ? Queue->GetUrgentQuoteForVehicle(VehicleId) : 0;
    return FText::FromString(FString::Printf(
        TEXT("Priority desk: %s -> URGENT $%d | two-step confirm | no pre-charge"),
        *VehicleId.ToString(), UrgentQuote));
}
