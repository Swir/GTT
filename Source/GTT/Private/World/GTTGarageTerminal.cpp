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

    AGTTVehicleBase* NearestVehicle = nullptr;
    float BestDistSq = FMath::Square(VehicleSearchRadius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!Vehicle || Vehicle->GetPersistentVehicleId().IsNone()) continue;
        const float DistSq = FVector::DistSquared2D(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            NearestVehicle = Vehicle;
        }
    }

    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GameMode)
    {
        Economy->PushMessage(TEXT("Garage manager unavailable."));
        return;
    }

    if (NearestVehicle)
    {
        GameMode->TryRegisterVehicle(NearestVehicle, Pawn, RegistrationCost);
        return;
    }

    Economy->PushMessage(TEXT("No vehicle at registration desk. Use GARAGE SLOT 1-4 selectors to recall a specific owned vehicle."), 5.0f);
}

FText AGTTGarageTerminal::GetInteractionText_Implementation() const
{
    return FText::Format(
        NSLOCTEXT("GTT", "GarageRegisterExplicitSlots", "Register nearby vehicle (${0}) / use numbered slots to recall"),
        FText::AsNumber(RegistrationCost));
}
