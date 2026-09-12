#include "Activities/GTTHeavyHaulTerminal.h"

#include "Activities/GTTHeavyHaulDirector.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"

AGTTHeavyHaulTerminal::AGTTHeavyHaulTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(95.0f, 95.0f, 90.0f));
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Trigger);
    Label->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));
    Label->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(34.0f);
    Label->SetTextRenderColor(FColor(255, 196, 70));
}

void AGTTHeavyHaulTerminal::Configure(EGTTHeavyHaulTerminalType InType, AGTTHeavyHaulDirector* InDirector)
{
    TerminalType = InType;
    Director = InDirector;
    if (!Label) return;
    switch (TerminalType)
    {
        case EGTTHeavyHaulTerminalType::ContractBoard: Label->SetText(FText::FromString(TEXT("HEAVY HAUL CONTRACT\nTRACTOR + TRAILER"))); break;
        case EGTTHeavyHaulTerminalType::Hitch: Label->SetText(FText::FromString(TEXT("TRAILER HITCH\nE"))); break;
        case EGTTHeavyHaulTerminalType::Load: Label->SetText(FText::FromString(TEXT("HEAVY TIMBER LOAD\nE"))); break;
        case EGTTHeavyHaulTerminalType::Deliver: Label->SetText(FText::FromString(TEXT("HILL FARM HEAVY BAY\nE"))); break;
    }
}

void AGTTHeavyHaulTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn || !Director) return;
    switch (TerminalType)
    {
        case EGTTHeavyHaulTerminalType::ContractBoard: Director->TryStartContract(Pawn); break;
        case EGTTHeavyHaulTerminalType::Hitch: Director->TryHitchTrailer(Pawn); break;
        case EGTTHeavyHaulTerminalType::Load: Director->TryLoadTimber(Pawn); break;
        case EGTTHeavyHaulTerminalType::Deliver: Director->TryDeliverTimber(Pawn); break;
    }
}

FText AGTTHeavyHaulTerminal::GetInteractionText_Implementation() const
{
    switch (TerminalType)
    {
        case EGTTHeavyHaulTerminalType::ContractBoard: return NSLOCTEXT("GTT", "HeavyHaulStart", "Take heavy timber haul contract");
        case EGTTHeavyHaulTerminalType::Hitch: return NSLOCTEXT("GTT", "HeavyHaulHitch", "Hitch / re-hitch farm trailer");
        case EGTTHeavyHaulTerminalType::Load: return NSLOCTEXT("GTT", "HeavyHaulLoad", "Load heavy timber");
        case EGTTHeavyHaulTerminalType::Deliver: return NSLOCTEXT("GTT", "HeavyHaulDeliver", "Deliver heavy timber trailer");
        default: return NSLOCTEXT("GTT", "HeavyHaulInteract", "Heavy haul");
    }
}
