#include "Activities/GTTRecoveryTerminal.h"

#include "Activities/GTTRecoveryDirector.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"

AGTTRecoveryTerminal::AGTTRecoveryTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    RootComponent = Trigger;
    Trigger->SetBoxExtent(FVector(110.0f, 110.0f, 90.0f));
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(RootComponent);
    Label->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(34.0f);
}

void AGTTRecoveryTerminal::Configure(EGTTRecoveryTerminalType InType, AGTTRecoveryDirector* InDirector)
{
    TerminalType = InType;
    Director = InDirector;
    Label->SetText(FText::FromString(TerminalType == EGTTRecoveryTerminalType::Workshop ? TEXT("RECOVERY DESK / DROP BAY") : TEXT("RECOVERY HOOK")));
}

void AGTTRecoveryTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn || !Director) return;

    if (TerminalType == EGTTRecoveryTerminalType::Hook)
    {
        Director->TryHookRecoveryVehicle(Pawn);
        return;
    }

    if (Director->GetStage() == EGTTRecoveryStage::Idle || Director->GetStage() == EGTTRecoveryStage::Completed)
        Director->TryStartRecovery(Pawn);
    else
        Director->TryFinishRecovery(Pawn);
}

FText AGTTRecoveryTerminal::GetInteractionText_Implementation() const
{
    if (!Director) return FText::FromString(TEXT("Recovery service"));
    if (TerminalType == EGTTRecoveryTerminalType::Hook) return FText::FromString(TEXT("Attach tow line"));
    return FText::FromString(Director->GetStage() == EGTTRecoveryStage::Idle || Director->GetStage() == EGTTRecoveryStage::Completed
        ? TEXT("Start roadside recovery contract")
        : TEXT("Deliver disabled vehicle"));
}
