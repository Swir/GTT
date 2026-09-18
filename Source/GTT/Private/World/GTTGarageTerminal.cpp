#include "World/GTTGarageTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTGarageSlotTerminal.h"
#include "World/GTTWorkshopHoursPolicy.h"
#include "World/GTTWorkshopRepairQueueSubsystem.h"

namespace
{
    const AGTTDayNightCycle* FindGarageWorldClock(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AGTTDayNightCycle> It(World); It; ++It)
        {
            if (IsValid(*It)) return *It;
        }
        return nullptr;
    }

    bool IsGarageWorkshopOpen(UWorld* World)
    {
        const AGTTDayNightCycle* Clock = FindGarageWorldClock(World);
        return !Clock || GTTWorkshopHoursPolicy::IsOpen(Clock->GetTimeOfDayHours());
    }
}

AGTTGarageTerminal::AGTTGarageTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.9f, 0.9f, 1.25f));
}

void AGTTGarageTerminal::BeginPlay()
{
    Super::BeginPlay();
    if (!GetWorld()) return;

    for (int32 Slot = 0; Slot < FleetSlotCount; ++Slot)
    {
        const FVector Offset(-1300.0f + Slot * 420.0f, -630.0f, 55.0f);
        AGTTGarageSlotTerminal* Selector = GetWorld()->SpawnActor<AGTTGarageSlotTerminal>(GetActorLocation() + Offset, FRotator::ZeroRotator);
        if (Selector) Selector->SetSlotIndex(Slot);
    }
}

void AGTTGarageTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn || !GetWorld()) return;

    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn);
    if (!Economy) return;

    AGTTVehicleBase* NearestUnownedVehicle = nullptr;
    float BestDistSq = FMath::Square(VehicleSearchRadius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!Vehicle || Vehicle->IsOwnedByPlayer() || Vehicle->GetPersistentVehicleId().IsNone()) continue;
        const float DistSq = FVector::DistSquared2D(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            NearestUnownedVehicle = Vehicle;
        }
    }

    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GameMode)
    {
        Economy->PushMessage(TEXT("Garage manager unavailable."));
        return;
    }

    if (NearestUnownedVehicle)
    {
        GameMode->TryRegisterVehicle(NearestUnownedVehicle, Pawn, RegistrationCost);
        return;
    }

    const bool bWorkshopOpen = IsGarageWorkshopOpen(GetWorld());
    UGTTWorkshopRepairQueueSubsystem* RepairQueue = GetWorld()->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>();
    if (!bWorkshopOpen && RepairQueue && RepairQueue->GetQueuedRepairCount() < RepairQueue->GetQueueCapacity())
    {
        FString QueueSummary;
        if (RepairQueue->TryQueueNearestEligibleNativeRoadVehicle(GetActorLocation(), VehicleSearchRadius, QueueSummary))
        {
            Economy->PushMessage(QueueSummary, 9.0f);
            return;
        }
    }

    if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
    {
        const int32 WorkshopHoldCount = Fleet->GetWorkshopHoldCount(FleetSlotCount);
        const AGTTDayNightCycle* Clock = FindGarageWorldClock(GetWorld());
        const FString ClockSuffix = Clock
            ? FString::Printf(TEXT(" | %s"), *Clock->GetClockText())
            : FString(TEXT(" | world clock unavailable: fail-open"));
        FString Summary = Fleet->BuildFleetSummary(FleetSlotCount);
        Summary += TEXT("\nUse a numbered bay to dispatch a vehicle. Dispatch sets it ACTIVE and never repairs damage.");
        Summary += FString::Printf(
            TEXT("\nWORKSHOP %s | hours %s%s."),
            bWorkshopOpen ? TEXT("OPEN") : TEXT("CLOSED"),
            *GTTWorkshopHoursPolicy::GetScheduleText(),
            *ClockSuffix);
        if (RepairQueue && RepairQueue->HasQueuedRepair())
        {
            Summary += TEXT("\n");
            Summary += RepairQueue->GetQueueStatusText().ToString();
            Summary += TEXT(" | each appointment keeps its own exact vehicle ID and locked quote; payment occurs only when that vehicle is serviced.");
            if (!bWorkshopOpen && RepairQueue->GetQueuedRepairCount() < RepairQueue->GetQueueCapacity())
            {
                Summary += TEXT(" Interact again with another eligible damaged vehicle nearby to fill the next appointment slot.");
            }
        }
        else if (!bWorkshopOpen)
        {
            Summary += FString::Printf(
                TEXT("\nAfter-hours garage desk can reserve up to %d damaged/mobile native road vehicles for deterministic next-opening service slots, each with an exact-ID locked quote and no pre-charge."),
                RepairQueue ? RepairQueue->GetQueueCapacity() : UGTTWorkshopRepairQueueSubsystem::MaxQueuedRepairs);
        }

        if (WorkshopHoldCount > 0)
        {
            Summary += FString::Printf(
                TEXT("\nWORKSHOP HOLD: %d vehicle%s cannot be recalled until repair/service clears TOW/IMMOBILE status."),
                WorkshopHoldCount,
                WorkshopHoldCount == 1 ? TEXT("") : TEXT("s"));
            if (!bWorkshopOpen)
            {
                Summary += FString::Printf(
                    TEXT(" Emergency recovery remains available after hours at +%d%%; hard holds never enter the deferred appointment book."),
                    GTTWorkshopHoursPolicy::AfterHoursRecoverySurchargePercent);
            }
        }
        else if (!bWorkshopOpen)
        {
            Summary += TEXT("\nOrdinary repair/refuel waits for opening and then its appointment; garage dispatch remains available for serviceable vehicles.");
        }
        Economy->PushMessage(Summary, 10.0f);
        return;
    }

    Economy->PushMessage(TEXT("No vehicle at registration desk. Use GARAGE SLOT 1-4 selectors to dispatch a specific owned vehicle."), 5.0f);
}

FText AGTTGarageTerminal::GetInteractionText_Implementation() const
{
    int32 OwnedCount = 0;
    int32 WorkshopHoldCount = 0;
    int32 QueuedCount = 0;
    int32 QueueCapacity = UGTTWorkshopRepairQueueSubsystem::MaxQueuedRepairs;
    if (GetWorld())
    {
        if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
        {
            OwnedCount = Fleet->BuildFleetSnapshot(FleetSlotCount).Num();
            WorkshopHoldCount = Fleet->GetWorkshopHoldCount(FleetSlotCount);
        }
        if (const UGTTWorkshopRepairQueueSubsystem* RepairQueue = GetWorld()->GetSubsystem<UGTTWorkshopRepairQueueSubsystem>())
        {
            QueuedCount = RepairQueue->GetQueuedRepairCount();
            QueueCapacity = RepairQueue->GetQueueCapacity();
        }
    }

    return FText::FromString(FString::Printf(
        TEXT("Garage office: fleet %d/%d | holds %d | appointments %d/%d | workshop %s %s | register ($%d)"),
        OwnedCount,
        FleetSlotCount,
        WorkshopHoldCount,
        QueuedCount,
        QueueCapacity,
        IsGarageWorkshopOpen(GetWorld()) ? TEXT("OPEN") : TEXT("CLOSED"),
        *GTTWorkshopHoursPolicy::GetScheduleText(),
        RegistrationCost));
}
