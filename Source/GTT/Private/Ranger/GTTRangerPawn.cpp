#include "Ranger/GTTRangerPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Ranger/GTTRangerAIController.h"
#include "UObject/ConstructorHelpers.h"

AGTTRangerPawn::AGTTRangerPawn()
{
    PrimaryActorTick.bCanEverTick = false;
    AIControllerClass = AGTTRangerAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCharacterMovement()->MaxWalkSpeed = 520.0f;
    GetCharacterMovement()->bOrientRotationToMovement = true;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(RootComponent);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
    BodyMesh->SetRelativeScale3D(FVector(0.32f, 0.32f, 0.9f));
    if (CylinderFinder.Succeeded())
    {
        BodyMesh->SetStaticMesh(CylinderFinder.Object);
    }

    HatMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HatMesh"));
    HatMesh->SetupAttachment(RootComponent);
    HatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HatMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 72.0f));
    HatMesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.08f));
    if (CubeFinder.Succeeded())
    {
        HatMesh->SetStaticMesh(CubeFinder.Object);
    }
}
