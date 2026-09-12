#include "Activities/GTTBrawlTerminal.h"

#include "Activities/GTTBrawlDirector.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
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

    Label=CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Mesh);
    Label->SetRelativeLocation(FVector(0,0,145));
    Label->SetRelativeRotation(FRotator(0,180,0));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(36.0f);
    Label->SetTextRenderColor(FColor(255,85,55));
    Label->SetText(FText::FromString(TEXT("BENT AXLE BRAWL\n18:30-02:30 | E")));
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
