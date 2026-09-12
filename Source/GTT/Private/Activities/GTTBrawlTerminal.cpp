#include "Activities/GTTBrawlTerminal.h"

#include "Activities/GTTBrawlDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGTTBrawlTerminal::AGTTBrawlTerminal()
{
    PrimaryActorTick.bCanEverTick=false;
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if(CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(.7f,.7f,1.1f));
}

void AGTTBrawlTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn=Cast<APawn>(Interactor);
    if(!Pawn) return;
    if(AGTTBrawlDirector* Director=Cast<AGTTBrawlDirector>(UGameplayStatics::GetActorOfClass(this,AGTTBrawlDirector::StaticClass()))) Director->TryStartBrawl(Pawn);
}

FText AGTTBrawlTerminal::GetInteractionText_Implementation() const
{
    return NSLOCTEXT("GTT","BentAxleBrawl","Enter Bent Axle Brawl");
}
