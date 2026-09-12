#include "World/GTTGarageTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"

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
    UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Pawn);
    if (!Economy)
    {
        return;
    }

    if (Wanted && Wanted->GetWantedLevel() > 0)
    {
        Economy->PushMessage(TEXT("Garage refuses service while police are looking for you."), 4.0f);
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

    if (!NearestVehicle->IsOwnedByPlayer())
    {
        if (!Economy->SpendCash(RegistrationCost, FString::Printf(TEXT("Vehicle registration: -$%d"), RegistrationCost)))
        {
            return;
        }

        NearestVehicle->MarkOwnedByPlayer();
        Economy->PushMessage(TEXT("Vehicle registered to your garage. It will now persist in saves."), 5.0f);
    }
    else
    {
        Economy->PushMessage(TEXT("Owned vehicle parked. Saving progress..."), 3.0f);
    }

    if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
    {
        GameMode->SaveProgress();
    }
}

FText AGTTGarageTerminal::GetInteractionText_Implementation() const
{
    return FText::Format(
        NSLOCTEXT("GTT", "GarageRegister", "Register / save nearby vehicle (${0})"),
        FText::AsNumber(RegistrationCost));
}
