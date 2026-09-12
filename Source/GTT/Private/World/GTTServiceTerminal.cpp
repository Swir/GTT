#include "World/GTTServiceTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"

AGTTServiceTerminal::AGTTServiceTerminal()
{
    PrimaryActorTick.bCanEverTick = false;

    TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
    SetRootComponent(TerminalMesh);
    TerminalMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TerminalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    TerminalMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TerminalMesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.9f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        TerminalMesh->SetStaticMesh(CubeFinder.Object);
    }
}

void AGTTServiceTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    UGTTPlayerEconomyComponent* Economy = Pawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn) : nullptr;
    if (!Economy)
    {
        return;
    }

    if (ServiceType == EGTTServiceType::FishBuyer)
    {
        Economy->SellAllFish(FishPricePerKg);
        return;
    }

    AGTTVehicleBase* Vehicle = FindNearestVehicle();
    if (!Vehicle)
    {
        Economy->PushMessage(TEXT("Workshop: park a vehicle nearby first."));
        return;
    }

    const bool bNeedsRepair = Vehicle->GetConditionPercent() < 0.999f;
    const bool bNeedsFuel = Vehicle->GetFuelPercent() < 0.999f;
    if (!bNeedsRepair && !bNeedsFuel)
    {
        Economy->PushMessage(TEXT("Workshop: that machine is already ready to go."));
        return;
    }

    if (!Economy->SpendCash(WorkshopServiceCost, FString::Printf(TEXT("Workshop service - $%d"), WorkshopServiceCost)))
    {
        return;
    }

    Vehicle->RepairVehicle(100000.0f);
    Vehicle->RefuelVehicle(100000.0f);
    Economy->PushMessage(FString::Printf(TEXT("%s repaired and refuelled."), *Vehicle->GetVehicleDisplayName().ToString()), 5.0f);
}

FText AGTTServiceTerminal::GetInteractionText_Implementation() const
{
    return ServiceType == EGTTServiceType::FishBuyer
        ? NSLOCTEXT("GTT", "SellFish", "Sell all fish")
        : NSLOCTEXT("GTT", "WorkshopService", "Repair + refuel nearby vehicle ($75)");
}

AGTTVehicleBase* AGTTServiceTerminal::FindNearestVehicle() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    AGTTVehicleBase* BestVehicle = nullptr;
    float BestDistanceSquared = FMath::Square(VehicleSearchRadius);

    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!IsValid(Vehicle))
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistanceSquared <= BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestVehicle = Vehicle;
        }
    }

    return BestVehicle;
}
