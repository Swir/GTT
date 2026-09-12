#include "Missions/GTTMainStoryTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTMainStoryDirector.h"
#include "UObject/ConstructorHelpers.h"

AGTTMainStoryTerminal::AGTTMainStoryTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.62f, 0.62f, 1.05f));
}

void AGTTMainStoryTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    AGTTMainStoryDirector* Director = Cast<AGTTMainStoryDirector>(UGameplayStatics::GetActorOfClass(this, AGTTMainStoryDirector::StaticClass()));
    if (!Pawn || !Director) return;

    switch (TerminalType)
    {
        case EGTTMainStoryTerminalType::FarmOffice: Director->TryFarmContact(Pawn); break;
        case EGTTMainStoryTerminalType::NorthWood: Director->TryNorthWoodPickup(Pawn); break;
        case EGTTMainStoryTerminalType::VillageShop: Director->TryShopDelivery(Pawn); break;
        case EGTTMainStoryTerminalType::Tavern: Director->TryTavernMeet(Pawn); break;
        case EGTTMainStoryTerminalType::EastRoad: Director->TryEastRoadPickup(Pawn); break;
        case EGTTMainStoryTerminalType::Workshop: Director->TryWorkshopDelivery(Pawn); break;
    }
}

FText AGTTMainStoryTerminal::GetInteractionText_Implementation() const
{
    switch (TerminalType)
    {
        case EGTTMainStoryTerminalType::FarmOffice: return NSLOCTEXT("GTT", "StoryFarm", "Talk about farm business / finish story arc");
        case EGTTMainStoryTerminalType::NorthWood: return NSLOCTEXT("GTT", "StoryWood", "Collect sealed county ledger");
        case EGTTMainStoryTerminalType::VillageShop: return NSLOCTEXT("GTT", "StoryShop", "Deliver county ledger");
        case EGTTMainStoryTerminalType::Tavern: return NSLOCTEXT("GTT", "StoryTavern", "Meet the backroad contact");
        case EGTTMainStoryTerminalType::EastRoad: return NSLOCTEXT("GTT", "StoryEastRoad", "Take unmarked backroad crate");
        case EGTTMainStoryTerminalType::Workshop: return NSLOCTEXT("GTT", "StoryWorkshop", "Deliver backroad crate");
    }
    return FText::GetEmpty();
}
