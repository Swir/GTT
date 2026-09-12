#include "NPC/GTTCitizenPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"

AGTTCitizenPawn::AGTTCitizenPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->MaxWalkSpeed = WanderSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 260.0f, 0.0f);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(RootComponent);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
    BodyMesh->SetRelativeScale3D(FVector(0.28f, 0.28f, 0.85f));
    if (CylinderFinder.Succeeded())
    {
        BodyMesh->SetStaticMesh(CylinderFinder.Object);
    }

    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(RootComponent);
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
    HeadMesh->SetRelativeScale3D(FVector(0.22f));
    if (SphereFinder.Succeeded())
    {
        HeadMesh->SetStaticMesh(SphereFinder.Object);
    }
}

void AGTTCitizenPawn::BeginPlay()
{
    Super::BeginPlay();
    HomeLocation = GetActorLocation();
    GetCharacterMovement()->MaxWalkSpeed = WanderSpeed;
    ChooseNewWanderTarget();
}

void AGTTCitizenPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    RetargetTimeRemaining -= DeltaSeconds;
    FVector ToTarget = WanderTarget - GetActorLocation();
    ToTarget.Z = 0.0f;

    if (RetargetTimeRemaining <= 0.0f || ToTarget.SizeSquared2D() < FMath::Square(90.0f))
    {
        ChooseNewWanderTarget();
        ToTarget = WanderTarget - GetActorLocation();
        ToTarget.Z = 0.0f;
    }

    if (!ToTarget.IsNearlyZero())
    {
        AddMovementInput(ToTarget.GetSafeNormal2D(), 1.0f);
    }
}

bool AGTTCitizenPawn::TryWitnessVehicleTheft(AGTTVehicleBase* Vehicle, APawn* Offender)
{
    if (!Vehicle || !Offender || LastWitnessedVehicle.Get() == Vehicle || !GetWorld())
    {
        return false;
    }

    if (FVector::DistSquared2D(GetActorLocation(), Vehicle->GetActorLocation()) > FMath::Square(WitnessRadius))
    {
        return false;
    }

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTWitnessSight), false, this);
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(Offender);

    const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
    const FVector End = Vehicle->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    if (bBlocked && Hit.GetActor() != Vehicle)
    {
        return false;
    }

    LastWitnessedVehicle = Vehicle;

    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Offender))
    {
        Wanted->AddHeat(WitnessHeat);
    }

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Offender))
    {
        Economy->PushMessage(TEXT("A villager saw the theft and called the police!"), 5.0f);
    }

    return true;
}

void AGTTCitizenPawn::ChooseNewWanderTarget()
{
    const FVector2D Offset = FMath::RandPointInCircle(WanderRadius);
    WanderTarget = HomeLocation + FVector(Offset.X, Offset.Y, 0.0f);
    RetargetTimeRemaining = FMath::FRandRange(3.0f, 8.0f);
}
