#include "World/GTTVillageEventMarker.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGTTVillageEventMarker::AGTTVillageEventMarker()
{
    PrimaryActorTick.bCanEverTick = false;

    MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
    SetRootComponent(MarkerMesh);
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MarkerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    MarkerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    MarkerMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.35f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderFinder.Succeeded())
    {
        MarkerMesh->SetStaticMesh(CylinderFinder.Object);
    }

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(MarkerMesh);
    Label->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(42.0f);
    Label->SetTextRenderColor(FColor(255, 190, 70));
    Label->SetCastShadow(true);
}

void AGTTVillageEventMarker::ConfigureEvent(EGTTVillageNightEventType NewType)
{
    EventType = NewType;
    RefreshPresentation();
}

void AGTTVillageEventMarker::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    UGTTPlayerEconomyComponent* Economy = Pawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn) : nullptr;
    if (!Pawn || !Economy)
    {
        return;
    }

    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    bool bResolveEvent = true;

    switch (EventType)
    {
    case EGTTVillageNightEventType::BrokenDownNeighbor:
        Economy->AddCash(90, TEXT("Neighbor rescue: +$90"));
        Economy->PushMessage(TEXT("You got the old machine running. The neighbor pays cash and promises not to ask how."), 5.0f);
        break;

    case EGTTVillageNightEventType::TractorMeet:
        if (!Economy->SpendCash(25, TEXT("Midnight tractor meet entry: -$25")))
        {
            bResolveEvent = false;
            break;
        }
        if (FMath::FRand() < 0.65f)
        {
            Economy->AddCash(150, TEXT("Midnight tractor meet win: +$150"));
            Economy->PushMessage(TEXT("Your machine wins the loudest-rattle contest. Somehow that is a trophy category."), 5.0f);
        }
        else
        {
            Economy->PushMessage(TEXT("You lose the tractor meet. Someone with three exhaust pipes was impossible to beat."), 5.0f);
        }
        break;

    case EGTTVillageNightEventType::BonfireRun:
        Economy->AddCash(70, TEXT("Bonfire supply run: +$70"));
        if (GameMode)
        {
            GameMode->ReportWildlifeCrime(Pawn, 18.0f);
        }
        Economy->PushMessage(TEXT("You delivered questionable bonfire supplies. The ranger definitely noticed the smoke."), 5.0f);
        break;

    case EGTTVillageNightEventType::LostCrate:
        Economy->AddCash(55, TEXT("Returned lost crate: +$55"));
        Economy->PushMessage(TEXT("Mystery crate returned. Nobody explains what was inside. Probably better that way."), 5.0f);
        break;
    }

    if (bResolveEvent)
    {
        Destroy();
    }
}

FText AGTTVillageEventMarker::GetInteractionText_Implementation() const
{
    switch (EventType)
    {
    case EGTTVillageNightEventType::BrokenDownNeighbor:
        return NSLOCTEXT("GTT", "NightEventNeighbor", "Help broken-down neighbor");
    case EGTTVillageNightEventType::TractorMeet:
        return NSLOCTEXT("GTT", "NightEventTractorMeet", "Enter midnight tractor meet ($25)");
    case EGTTVillageNightEventType::BonfireRun:
        return NSLOCTEXT("GTT", "NightEventBonfire", "Deliver suspicious bonfire supplies");
    case EGTTVillageNightEventType::LostCrate:
        return NSLOCTEXT("GTT", "NightEventCrate", "Return mystery crate");
    }
    return NSLOCTEXT("GTT", "NightEventInteract", "Join night event");
}

void AGTTVillageEventMarker::RefreshPresentation()
{
    switch (EventType)
    {
    case EGTTVillageNightEventType::BrokenDownNeighbor:
        EventTitle = TEXT("BROKEN-DOWN NEIGHBOR");
        break;
    case EGTTVillageNightEventType::TractorMeet:
        EventTitle = TEXT("MIDNIGHT TRACTOR MEET");
        break;
    case EGTTVillageNightEventType::BonfireRun:
        EventTitle = TEXT("SUSPICIOUS BONFIRE RUN");
        break;
    case EGTTVillageNightEventType::LostCrate:
        EventTitle = TEXT("MYSTERY CRATE");
        break;
    }

    if (Label)
    {
        Label->SetText(FText::FromString(EventTitle + TEXT("\nE - INTERACT")));
    }
}
