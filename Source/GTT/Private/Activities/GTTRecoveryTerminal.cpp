#include "Activities/GTTRecoveryTerminal.h"

#include "Activities/GTTRecoveryDirector.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGTTRecoveryTerminal::AGTTRecoveryTerminal()
{
    PrimaryActorTick.bCanEverTick = false;

    InteractionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBounds"));
    SetRootComponent(InteractionBounds);
    InteractionBounds->SetBoxExtent(FVector(120.0f, 120.0f, 110.0f));
    InteractionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
    TerminalMesh->SetupAttachment(InteractionBounds);
    TerminalMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 1.25f));
    TerminalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) TerminalMesh->SetStaticMesh(CubeFinder.Object);

    TerminalLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TerminalLabel"));
    TerminalLabel->SetupAttachment(InteractionBounds);
    TerminalLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 165.0f));
    TerminalLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    TerminalLabel->SetHorizontalAlignment(EHTA_Center);
    TerminalLabel->SetWorldSize(34.0f);
    TerminalLabel->SetTextRenderColor(FColor(255, 170, 70));
    TerminalLabel->SetText(FText::FromString(TEXT("RURAL RECOVERY - E")));
}

void AGTTRecoveryTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* PlayerPawn = Cast<APawn>(Interactor);
    if (!PlayerPawn) return;

    AGTTRecoveryDirector* Director = Cast<AGTTRecoveryDirector>(UGameplayStatics::GetActorOfClass(this, AGTTRecoveryDirector::StaticClass()));
    if (!Director) return;

    if (Director->IsRecoveryActive())
    {
        Director->TryFinishRecovery(PlayerPawn, GetActorLocation());
    }
    else
    {
        Director->TryStartRecovery(PlayerPawn);
    }
}

FText AGTTRecoveryTerminal::GetInteractionText_Implementation() const
{
    const AGTTRecoveryDirector* Director = Cast<AGTTRecoveryDirector>(UGameplayStatics::GetActorOfClass(this, AGTTRecoveryDirector::StaticClass()));
    if (Director && Director->IsRecoveryActive())
    {
        return NSLOCTEXT("GTT", "FinishRecoveryJob", "Deliver recovered vehicle");
    }
    return NSLOCTEXT("GTT", "StartRecoveryJob", "Start vehicle recovery contract");
}
