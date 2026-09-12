#include "Activities/GTTRuralWorkTerminal.h"

#include "Activities/GTTRuralWorkDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGTTRuralWorkTerminal::AGTTRuralWorkTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 1.15f));
}

void AGTTRuralWorkTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    AGTTRuralWorkDirector* Director = Cast<AGTTRuralWorkDirector>(UGameplayStatics::GetActorOfClass(this, AGTTRuralWorkDirector::StaticClass()));
    if (!Pawn || !Director) return;

    switch (TerminalType)
    {
        case EGTTRuralWorkTerminalType::TimberStart: Director->TryStartTimber(Pawn); break;
        case EGTTRuralWorkTerminalType::TimberPickup: Director->TryPickupTimber(Pawn); break;
        case EGTTRuralWorkTerminalType::TimberFinish: Director->TryDeliverTimber(Pawn); break;
        case EGTTRuralWorkTerminalType::MowingStart: Director->TryStartMowing(Pawn); break;
    }
}

FText AGTTRuralWorkTerminal::GetInteractionText_Implementation() const
{
    switch (TerminalType)
    {
        case EGTTRuralWorkTerminalType::TimberStart: return NSLOCTEXT("GTT", "TimberStart", "Take timber-haul contract");
        case EGTTRuralWorkTerminalType::TimberPickup: return NSLOCTEXT("GTT", "TimberPickup", "Load legal timber");
        case EGTTRuralWorkTerminalType::TimberFinish: return NSLOCTEXT("GTT", "TimberFinish", "Unload timber");
        case EGTTRuralWorkTerminalType::MowingStart: return NSLOCTEXT("GTT", "MowingStart", "Take field-mowing contract");
    }
    return FText::GetEmpty();
}
