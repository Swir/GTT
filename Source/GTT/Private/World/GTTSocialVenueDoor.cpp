#include "World/GTTSocialVenueDoor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AGTTSocialVenueDoor::AGTTSocialVenueDoor()
{
    PrimaryActorTick.bCanEverTick = false;

    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    SetRootComponent(DoorMesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) DoorMesh->SetStaticMesh(CubeFinder.Object);
    DoorMesh->SetRelativeScale3D(FVector(0.18f, 0.85f, 1.35f));
    DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DoorMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    DoorMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    SignText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SignText"));
    SignText->SetupAttachment(DoorMesh);
    SignText->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
    SignText->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    SignText->SetHorizontalAlignment(EHTA_Center);
    SignText->SetWorldSize(34.0f);
    SignText->SetText(FText::FromString(TEXT("ENTER")));
}

void AGTTSocialVenueDoor::ConfigureDoor(const FString& InVenueName, const FVector& InDestination, const FRotator& InDestinationRotation, bool bInInteriorExit)
{
    VenueName = InVenueName;
    Destination = InDestination;
    DestinationRotation = InDestinationRotation;
    bInteriorExit = bInInteriorExit;
    if (SignText) SignText->SetText(FText::FromString(bInteriorExit ? FString::Printf(TEXT("EXIT %s"), *VenueName) : FString::Printf(TEXT("ENTER %s"), *VenueName)));
}

void AGTTSocialVenueDoor::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    if (!Pawn) return;
    Pawn->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
    Pawn->SetActorRotation(DestinationRotation);
}

FText AGTTSocialVenueDoor::GetInteractionText_Implementation() const
{
    return FText::FromString(bInteriorExit ? FString::Printf(TEXT("Leave %s"), *VenueName) : FString::Printf(TEXT("Enter %s"), *VenueName));
}
