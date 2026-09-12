#include "Missions/GTTNightFavorTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTNightFavorDirector.h"
#include "UObject/ConstructorHelpers.h"

AGTTNightFavorTerminal::AGTTNightFavorTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.05f));
}

void AGTTNightFavorTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    AGTTNightFavorDirector* Director = Cast<AGTTNightFavorDirector>(UGameplayStatics::GetActorOfClass(this, AGTTNightFavorDirector::StaticClass()));
    if (!Pawn || !Director) return;

    switch (TerminalType)
    {
        case EGTTNightFavorTerminalType::Tavern:
            if (Director->GetStage() == EGTTNightFavorStage::ReturnToTavern) Director->TryFinish(Pawn);
            else Director->TryStart(Pawn);
            break;
        case EGTTNightFavorTerminalType::Workshop:
            Director->TryCollectParts(Pawn);
            break;
        case EGTTNightFavorTerminalType::Neighbor:
            Director->TryHelpNeighbor(Pawn);
            break;
    }
}

FText AGTTNightFavorTerminal::GetInteractionText_Implementation() const
{
    switch (TerminalType)
    {
        case EGTTNightFavorTerminalType::Tavern: return NSLOCTEXT("GTT", "NightFavorTavern", "Ask about the night favor / collect payment");
        case EGTTNightFavorTerminalType::Workshop: return NSLOCTEXT("GTT", "NightFavorWorkshop", "Collect emergency parts crate");
        case EGTTNightFavorTerminalType::Neighbor: return NSLOCTEXT("GTT", "NightFavorNeighbor", "Install emergency parts for neighbor");
    }
    return FText::GetEmpty();
}
