#include "World/GTTGarageTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"

AGTTGarageTerminal::AGTTGarageTerminal()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        Mesh->SetStaticMesh(CubeFinder.Object);
    }

    Mesh->SetRelativeScale3D(FVector(0.9f, 0.9f, 1.25f));
}

void AGTTGarageTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn || !GetWorld())
    {
        return;
    }

    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn);
    if (!Economy)
    {
        return;
    }

    AGTTVehicleBase* NearestVehicle = nullptr;
    float BestDistSq = FMath::Square(VehicleSearchRadius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        const float DistSq = FVector::DistSquared2D(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            BestDistSq = DistSq;
            NearestVehicle = Vehicle;
        }
    }

    if (!NearestVehicle)
    {
        Economy->PushMessage(TEXT("Park a vehicle near the garage first."));
        return;
    }

    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    if (!GameMode)
    {
        Economy->PushMessage(TEXT("Garage manager unavailable."));
        return;
    }

    GameMode->TryRegisterVehicle(NearestVehicle, Pawn, RegistrationCost);
}

FText AGTTGarageTerminal::GetInteractionText_Implementation() const
{
    return FText::Format(
        NSLOCTEXT("GTT", "GarageRegisterMulti", "Register / save nearby vehicle (${0})"),
        FText::AsNumber(RegistrationCost));
}
