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
        case EGTTMainStoryTerminalType::WardenOutpost: Director->TryWardenBriefing(Pawn); break;
        case EGTTMainStoryTerminalType::ForestCache: Director->TryForestCache(Pawn); break;
        case EGTTMainStoryTerminalType::HillFarm: Director->TryHillFarmEvidence(Pawn); break;
    }
}

FText AGTTMainStoryTerminal::GetInteractionText_Implementation() const
{
    switch (TerminalType)
    {
        case EGTTMainStoryTerminalType::FarmOffice: return NSLOCTEXT("GTT", "StoryFarm", "Talk about farm business / campaign");
        case EGTTMainStoryTerminalType::NorthWood: return NSLOCTEXT("GTT", "StoryWood", "Collect sealed county ledger");
        case EGTTMainStoryTerminalType::VillageShop: return NSLOCTEXT("GTT", "StoryShop", "Deliver county ledger");
        case EGTTMainStoryTerminalType::Tavern: return NSLOCTEXT("GTT", "StoryTavern", "Meet the backroad contact");
        case EGTTMainStoryTerminalType::EastRoad: return NSLOCTEXT("GTT", "StoryEastRoad", "Take unmarked backroad crate");
        case EGTTMainStoryTerminalType::Workshop: return NSLOCTEXT("GTT", "StoryWorkshop", "Deliver backroad crate");
        case EGTTMainStoryTerminalType::WardenOutpost: return NSLOCTEXT("GTT", "StoryWarden", "Meet warden about Timber Ghosts");
        case EGTTMainStoryTerminalType::ForestCache: return NSLOCTEXT("GTT", "StoryForestCache", "Inspect illegal timber evidence cache");
        case EGTTMainStoryTerminalType::HillFarm: return NSLOCTEXT("GTT", "StoryHillFarm", "Secure Timber Ghosts evidence with tractor");
    }
    return FText::GetEmpty();
}
