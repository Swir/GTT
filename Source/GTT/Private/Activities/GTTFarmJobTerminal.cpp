#include "Activities/GTTFarmJobTerminal.h"

#include "Activities/GTTFarmJobDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGTTFarmJobTerminal::AGTTFarmJobTerminal()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.2f));
}

void AGTTFarmJobTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    AGTTFarmJobDirector* Director = Cast<AGTTFarmJobDirector>(UGameplayStatics::GetActorOfClass(this, AGTTFarmJobDirector::StaticClass()));
    if (!Pawn || !Director) return;

    switch (TerminalType)
    {
        case EGTTFarmJobTerminalType::Start:
            Director->TryStartJob(Pawn);
            break;
        case EGTTFarmJobTerminalType::Pickup:
            Director->TryPickupCargo(Pawn);
            break;
        case EGTTFarmJobTerminalType::Finish:
            Director->TryCompleteJob(Pawn);
            break;
    }
}

FText AGTTFarmJobTerminal::GetInteractionText_Implementation() const
{
    switch (TerminalType)
    {
        case EGTTFarmJobTerminalType::Start:
            return NSLOCTEXT("GTT", "FarmJobStartV2", "Take farm cargo contract");
        case EGTTFarmJobTerminalType::Pickup:
            return NSLOCTEXT("GTT", "FarmJobPickup", "Load feed cargo");
        case EGTTFarmJobTerminalType::Finish:
            return NSLOCTEXT("GTT", "FarmJobFinishV2", "Deliver farm cargo");
    }
    return FText::GetEmpty();
}
