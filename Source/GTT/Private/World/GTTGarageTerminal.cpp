#include "World/GTTGarageTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "World/GTTGarageSlotTerminal.h"

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

    if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
    {
        const int32 WorkshopHoldCount = Fleet->GetWorkshopHoldCount(FleetSlotCount);
        FString Summary = Fleet->BuildFleetSummary(FleetSlotCount);
        Summary += TEXT("\nUse a numbered bay to dispatch a vehicle. Dispatch sets it ACTIVE and never repairs damage.");
        if (WorkshopHoldCount > 0)
        {
            Summary += FString::Printf(
                TEXT("\nWORKSHOP HOLD: %d vehicle%s cannot be recalled until repair/service clears TOW/IMMOBILE status."),
                WorkshopHoldCount,
                WorkshopHoldCount == 1 ? TEXT("") : TEXT("s"));
        }
        Economy->PushMessage(Summary, 9.0f);
        return;
    }

    Economy->PushMessage(TEXT("No vehicle at registration desk. Use GARAGE SLOT 1-4 selectors to dispatch a specific owned vehicle."), 5.0f);
}

FText AGTTGarageTerminal::GetInteractionText_Implementation() const
{
    int32 OwnedCount = 0;
    int32 WorkshopHoldCount = 0;
    if (GetWorld())
    {
        if (const UGTTGarageFleetSubsystem* Fleet = GetWorld()->GetSubsystem<UGTTGarageFleetSubsystem>())
        {
            OwnedCount = Fleet->BuildFleetSnapshot(FleetSlotCount).Num();
            WorkshopHoldCount = Fleet->GetWorkshopHoldCount(FleetSlotCount);
        }
    }

    return FText::FromString(FString::Printf(
        TEXT("Garage office: fleet %d/%d | workshop holds %d | register nearby vehicle ($%d)"),
        OwnedCount,
        FleetSlotCount,
        WorkshopHoldCount,
        RegistrationCost));
}
