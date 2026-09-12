#include "Activities/GTTFarmJobTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGTTFarmJobTerminal::AGTTFarmJobTerminal()
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

    Mesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.2f));
}

void AGTTFarmJobTerminal::Interact_Implementation(AActor* Interactor)
{
    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!GameMode || !Pawn)
    {
        return;
    }

    if (TerminalType == EGTTFarmJobTerminalType::Start)
    {
        GameMode->StartFarmJob(Pawn);
    }
    else
    {
        GameMode->CompleteFarmJob(Pawn);
    }
}

FText AGTTFarmJobTerminal::GetInteractionText_Implementation() const
{
    return TerminalType == EGTTFarmJobTerminalType::Start
        ? NSLOCTEXT("GTT", "FarmJobStart", "Start legal field job")
        : NSLOCTEXT("GTT", "FarmJobFinish", "Deliver field job");
}
