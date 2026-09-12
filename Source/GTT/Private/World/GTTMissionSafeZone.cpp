#include "World/GTTMissionSafeZone.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"

AGTTMissionSafeZone::AGTTMissionSafeZone()
{
    PrimaryActorTick.bCanEverTick = true;

    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(360.0f, 360.0f, 180.0f));
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Trigger->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    Trigger->SetGenerateOverlapEvents(true);

    GroundMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundMarker"));
    GroundMarker->SetupAttachment(Trigger);
    GroundMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        GroundMarker->SetStaticMesh(CubeFinder.Object);
        GroundMarker->SetRelativeLocation(FVector(0.0f, 0.0f, -165.0f));
        GroundMarker->SetRelativeScale3D(FVector(7.0f, 7.0f, 0.08f));
    }
}

void AGTTMissionSafeZone::BeginPlay()
{
    Super::BeginPlay();

    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGTTMissionSafeZone::HandleBeginOverlap);
    Trigger->OnComponentEndOverlap.AddDynamic(this, &AGTTMissionSafeZone::HandleEndOverlap);
}

void AGTTMissionSafeZone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bMissionFinished)
    {
        return;
    }

    AGTTVehicleBase* Vehicle = TrackedVehicle.Get();
    if (!Vehicle || !Trigger->IsOverlappingActor(Vehicle))
    {
        return;
    }

    AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    if (GameMode && GameMode->TryCompleteBorrowedTractor(Vehicle))
    {
        bMissionFinished = true;
        SetActorTickEnabled(false);
    }
}

void AGTTMissionSafeZone::HandleBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(OtherActor);
    if (Vehicle && Vehicle->WasReportedStolen())
    {
        TrackedVehicle = Vehicle;
    }
}

void AGTTMissionSafeZone::HandleEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (TrackedVehicle.Get() == OtherActor)
    {
        TrackedVehicle.Reset();
    }
}
