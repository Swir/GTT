#include "World/GTTWorkshopJobBoardTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "World/GTTWorkshopHoursPolicy.h"
#include "World/GTTWorkshopPriorityDeskTerminal.h"
#include "World/GTTWorkshopRepairQueueSubsystem.h"
#include "GTT.h"

namespace
{
    FString ResolveBoardAction(const FGTTWorkshopRepairQueueSnapshot& Snapshot)
    {
        if (Snapshot.State.Equals(TEXT("READY_FOR_PICKUP"), ESearchCase::IgnoreCase)) return TEXT("PICKUP");
        if (Snapshot.State.Equals(TEXT("AWAITING_PAYMENT"), ESearchCase::IgnoreCase)) return TEXT("PAYMENT");
        if (Snapshot.State.Equals(TEXT("IN_SERVICE"), ESearchCase::IgnoreCase)) return TEXT("WORKING");
        if (Snapshot.State.Equals(TEXT("READY"), ESearchCase::IgnoreCase)) return TEXT("CHECK-IN");
        return TEXT("SCHEDULED");
    }

    bool HasNearbyPriorityDesk(UWorld* World, const FVector& Origin)
    {
        if (!World) return false;
        for (TActorIterator<AGTTWorkshopPriorityDeskTerminal> It(World); It; ++It)
        {
            const AGTTWorkshopPriorityDeskTerminal* Desk = *It;
            if (IsValid(Desk) && FVector::DistSquared2D(Desk->GetActorLocation(), Origin) <= FMath::Square(1200.0f))
                return true;
        }
        return false;
    }
}

AGTTWorkshopJobBoardTerminal::AGTTWorkshopJobBoardTerminal()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Mesh->SetRelativeScale3D(FVector(0.42f, 0.18f, 1.20f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
}

void AGTTWorkshopJobBoardTerminal::BeginPlay()
{
    Super::BeginPlay();
    UWorld* World = GetWorld();
    if (!World || HasNearbyPriorityDesk(World, GetActorLocation())) return;

    // Source-built maps get a separate deliberate priority counter so cancellation/pickup on the
    // main board never shares an interaction gesture with a quote-changing URGENT promotion.
    const FVector DeskOffset(260.0f, 40.0f, 0.0f);
    World->SpawnActor<AGTTWorkshopPriorityDeskTerminal>(
        GetActorLocation() + DeskOffset,
        GetActorRotation());
}

FName AGTTWorkshopJobBoardTerminal::FindNearestQueuedVehicle() const
{
    UWorld* World = GetWorld();
    const UGTTWorkshopRepairQueueSubsystem* Queue = World ? World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>() : nullptr;
    if (!World || !Queue || !Queue->HasQueuedRepair()) return NAME_None;

    FName BestId = NAME_None;
    float BestDistanceSquared = FMath::Square(FMath::Max(100.0f, NearbyVehicleRadius));
    for (TActorIterator<AGTTRoadVehicleNativePawn> It(World); It; ++It)
    {
        const AGTTRoadVehicleNativePawn* Vehicle = *It;
        if (!IsValid(Vehicle) || !Vehicle->IsLegacyTakeoverActive()) continue;
        const FName VehicleId = Vehicle->GetPersistentVehicleId();
        if (VehicleId.IsNone() || !Queue->HasQueuedRepairForVehicle(VehicleId)) continue;

        const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistanceSquared <= BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestId = VehicleId;
        }
    }
    return BestId;
}

FString AGTTWorkshopJobBoardTerminal::BuildBoardSummary() const
{
    UWorld* World = GetWorld();
    const UGTTWorkshopRepairQueueSubsystem* Queue = World ? World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>() : nullptr;
    if (!Queue || !Queue->HasQueuedRepair())
    {
        return TEXT("WORKSHOP JOB BOARD | no active appointments. After-hours bookings are created at the garage office without pre-charge. The adjacent priority desk can promote a waiting STANDARD job to URGENT.");
    }

    const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
    FString Summary = FString::Printf(TEXT("WORKSHOP JOB BOARD | %d/%d appointments"), Snapshots.Num(), Queue->GetQueueCapacity());

    for (const FGTTWorkshopRepairQueueSnapshot& Snapshot : Snapshots)
    {
        const FString Action = ResolveBoardAction(Snapshot);
        FString Timing;
        if (Snapshot.State.Equals(TEXT("READY_FOR_PICKUP"), ESearchCase::IgnoreCase))
        {
            Timing = FString::Printf(TEXT("paid $%d / collect exact vehicle"), Snapshot.PaidAmount);
        }
        else if (Snapshot.State.Equals(TEXT("IN_SERVICE"), ESearchCase::IgnoreCase))
        {
            Timing = FString::Printf(TEXT("service %.1fh"), Snapshot.HoursUntilServiceComplete);
        }
        else if (Snapshot.State.Equals(TEXT("AWAITING_PAYMENT"), ESearchCase::IgnoreCase))
        {
            Timing = TEXT("service complete");
        }
        else
        {
            Timing = FString::Printf(TEXT("day %d %s / %.1fh"), Snapshot.ReadyDay,
                *GTTWorkshopHoursPolicy::FormatHour(Snapshot.ReadyHour), Snapshot.HoursUntilReady);
        }

        Summary += FString::Printf(
            TEXT("\n#%d %s | %s | %s | exact %s | locked $%d | %s"),
            Snapshot.QueuePosition,
            *Snapshot.State,
            *Action,
            *Snapshot.Priority,
            *Snapshot.PersistentVehicleId.ToString(),
            Snapshot.LockedQuote,
            *Timing);
    }

    Summary += FString::Printf(
        TEXT("\nAuthority: the board never charges or repairs. Bring the exact READY vehicle to the workshop; IN_SERVICE must remain parked; PAYMENT uses the locked quote; READY_FOR_PICKUP must be collected here. A nearby waiting appointment needs two interactions to cancel. Adjacent PRIORITY DESK promotes eligible STANDARD work to URGENT at +%d%% checkout with x%.2f service time and no pre-charge."),
        UGTTWorkshopRepairQueueSubsystem::UrgentQuoteSurchargePercent,
        UGTTWorkshopRepairQueueSubsystem::UrgentServiceDurationMultiplier);
    return Summary;
}

void AGTTWorkshopJobBoardTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    UGTTPlayerEconomyComponent* Economy = Pawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn) : nullptr;
    UWorld* World = GetWorld();
    UGTTWorkshopRepairQueueSubsystem* Queue = World ? World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>() : nullptr;
    if (!Economy || !World || !Queue) return;

    const float Now = World->GetTimeSeconds();
    if (PendingCancelExpiresAt >= 0.0f && Now > PendingCancelExpiresAt)
    {
        PendingCancelVehicleId = NAME_None;
        PendingCancelExpiresAt = -1.0f;
    }

    const FName NearbyQueuedId = FindNearestQueuedVehicle();
    if (!NearbyQueuedId.IsNone())
    {
        const TArray<FGTTWorkshopRepairQueueSnapshot> Snapshots = Queue->GetQueueSnapshots();
        const FGTTWorkshopRepairQueueSnapshot* Snapshot = Snapshots.FindByPredicate(
            [NearbyQueuedId](const FGTTWorkshopRepairQueueSnapshot& Candidate)
            {
                return Candidate.PersistentVehicleId == NearbyQueuedId;
            });

        if (Snapshot && Snapshot->bReadyForPickup)
        {
            FString PickupSummary;
            const bool bReleased = Queue->ReleaseCompletedRepairForPickup(NearbyQueuedId, PickupSummary);
            PendingCancelVehicleId = NAME_None;
            PendingCancelExpiresAt = -1.0f;
            Economy->PushMessage(PickupSummary, 9.0f);
            UE_LOG(LogGTT, Display,
                TEXT("WORKSHOP_JOB_BOARD_PICKUP vehicle=%s success=%s board_authority=RELEASE_ONLY charged=NO repair_mutation=NO"),
                *NearbyQueuedId.ToString(), bReleased ? TEXT("YES") : TEXT("NO"));
            return;
        }

        if (Snapshot && !Snapshot->bCheckedIn)
        {
            if (PendingCancelVehicleId == NearbyQueuedId && Now <= PendingCancelExpiresAt)
            {
                FString CancelSummary;
                const bool bCancelled = Queue->CancelQueuedRepair(NearbyQueuedId, CancelSummary);
                PendingCancelVehicleId = NAME_None;
                PendingCancelExpiresAt = -1.0f;
                Economy->PushMessage(CancelSummary, 8.0f);
                UE_LOG(LogGTT, Display,
                    TEXT("WORKSHOP_JOB_BOARD_CANCEL_CONFIRMED vehicle=%s success=%s board_authority=PRESENTATION_ONLY charged=NO mutation=NO"),
                    *NearbyQueuedId.ToString(), bCancelled ? TEXT("YES") : TEXT("NO"));
                return;
            }

            PendingCancelVehicleId = NearbyQueuedId;
            PendingCancelExpiresAt = Now + CancelConfirmationSeconds;
            Economy->PushMessage(FString::Printf(
                TEXT("%s\n\nCancel guard ARMED for exact vehicle %s. Interact again within %.0f seconds to confirm cancellation. No charge or repair mutation occurs."),
                *BuildBoardSummary(), *NearbyQueuedId.ToString(), CancelConfirmationSeconds), 11.0f);
            UE_LOG(LogGTT, Display,
                TEXT("WORKSHOP_JOB_BOARD_CANCEL_ARMED vehicle=%s timeout=%.1f exact_id=YES charged=NO mutation=NO"),
                *NearbyQueuedId.ToString(), CancelConfirmationSeconds);
            return;
        }
    }

    Economy->PushMessage(BuildBoardSummary(), 11.0f);
}

FText AGTTWorkshopJobBoardTerminal::GetInteractionText_Implementation() const
{
    UWorld* World = GetWorld();
    const UGTTWorkshopRepairQueueSubsystem* Queue = World ? World->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>() : nullptr;
    const int32 Count = Queue ? Queue->GetQueuedRepairCount() : 0;
    const int32 Capacity = Queue ? Queue->GetQueueCapacity() : UGTTWorkshopRepairQueueSubsystem::MaxQueuedRepairs;
    int32 PickupCount = 0;
    if (Queue)
    {
        for (const FGTTWorkshopRepairQueueSnapshot& Snapshot : Queue->GetQueueSnapshots())
        {
            PickupCount += Snapshot.bReadyForPickup ? 1 : 0;
        }
    }
    return FText::FromString(FString::Printf(
        TEXT("Workshop job board: %d/%d | pickup %d | view exact-ID jobs / collect completed / guarded cancel"),
        Count, Capacity, PickupCount));
}
