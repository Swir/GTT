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
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.9f, 0.9f, 1.25f));
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

    RecallNextOwnedVehicle(Pawn);
}

bool AGTTGarageTerminal::RecallNextOwnedVehicle(APawn* Pawn)
{
    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn);
    if (!Economy || !GetWorld()) return false;

    TArray<AGTTVehicleBase*> OwnedVehicles;
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (Vehicle && Vehicle->IsOwnedByPlayer() && !Vehicle->GetPersistentVehicleId().IsNone()) OwnedVehicles.Add(Vehicle);
    }

    if (OwnedVehicles.Num() == 0)
    {
        Economy->PushMessage(TEXT("Garage empty. Register a vehicle first."), 3.0f);
        return false;
    }

    RecallCursor = FMath::Abs(RecallCursor) % OwnedVehicles.Num();
    AGTTVehicleBase* Vehicle = OwnedVehicles[RecallCursor];
    RecallCursor = (RecallCursor + 1) % OwnedVehicles.Num();

    const FVector BayOffset(260.0f, -390.0f, 70.0f);
    FTransform Destination(FRotator(0.0f, 90.0f, 0.0f), GetActorLocation() + BayOffset);
    if (!Vehicle->RecallToTransform(Destination))
    {
        Economy->PushMessage(TEXT("Cannot recall that vehicle while it is occupied."), 3.0f);
        return false;
    }

    Economy->PushMessage(
        FString::Printf(TEXT("GARAGE RECALL: %s arrived. Use terminal again to cycle the fleet."), *Vehicle->GetVehicleDisplayName().ToString()),
        4.0f);
    return true;
}

FText AGTTGarageTerminal::GetInteractionText_Implementation() const
{
    return FText::Format(
        NSLOCTEXT("GTT", "GarageRegisterRecall", "Register nearby (${0}) / recall next owned vehicle"),
        FText::AsNumber(RegistrationCost));
}
