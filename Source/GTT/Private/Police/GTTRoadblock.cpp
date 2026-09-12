#include "Police/GTTRoadblock.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"

AGTTRoadblock::AGTTRoadblock()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;

    LeftBarrier = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftBarrier"));
    LeftBarrier->SetupAttachment(SceneRoot);
    LeftBarrier->SetStaticMesh(CubeMesh);
    LeftBarrier->SetRelativeLocation(FVector(0.0f, -220.0f, 60.0f));
    LeftBarrier->SetRelativeScale3D(FVector(0.22f, 1.65f, 0.65f));
    LeftBarrier->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    RightBarrier = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightBarrier"));
    RightBarrier->SetupAttachment(SceneRoot);
    RightBarrier->SetStaticMesh(CubeMesh);
    RightBarrier->SetRelativeLocation(FVector(0.0f, 220.0f, 60.0f));
    RightBarrier->SetRelativeScale3D(FVector(0.22f, 1.65f, 0.65f));
    RightBarrier->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    SpikeStrip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpikeStrip"));
    SpikeStrip->SetupAttachment(SceneRoot);
    SpikeStrip->SetStaticMesh(CubeMesh);
    SpikeStrip->SetRelativeLocation(FVector(95.0f, 0.0f, 8.0f));
    SpikeStrip->SetRelativeScale3D(FVector(0.18f, 4.25f, 0.07f));
    SpikeStrip->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    SpikeStrip->SetNotifyRigidBodyCollision(true);
    SpikeStrip->OnComponentHit.AddDynamic(this, &AGTTRoadblock::HandleSpikeHit);

    Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign"));
    Sign->SetupAttachment(SceneRoot);
    Sign->SetRelativeLocation(FVector(-35.0f, 0.0f, 165.0f));
    Sign->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    Sign->SetHorizontalAlignment(EHTA_Center);
    Sign->SetWorldSize(46.0f);
    Sign->SetTextRenderColor(FColor(255, 70, 55));
    Sign->SetText(FText::FromString(TEXT("POLICE ROADBLOCK\nSPIKE STRIP")));
}

void AGTTRoadblock::SetResponseTier(int32 NewTier)
{
    ResponseTier = FMath::Clamp(NewTier, 1, 2);
    if (Sign)
    {
        Sign->SetText(FText::FromString(FString::Printf(TEXT("POLICE ROADBLOCK T%d\nSPIKE STRIP"), ResponseTier)));
    }
}

void AGTTRoadblock::HandleSpikeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    FVector NormalImpulse, const FHitResult& Hit)
{
    AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(OtherActor);
    if (!Vehicle)
    {
        return;
    }

    Vehicle->ApplyTireDamage(0.18f + ResponseTier * 0.08f);
    Vehicle->ApplyVehicleDamage(1.5f + ResponseTier * 1.5f);
}
