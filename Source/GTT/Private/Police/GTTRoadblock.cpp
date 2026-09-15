#include "Police/GTTRoadblock.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTT.h"

namespace { constexpr float SpikeRepeatCooldownSeconds = 0.75f; }

AGTTRoadblock::AGTTRoadblock()
{
    PrimaryActorTick.bCanEverTick = false;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")); SetRootComponent(SceneRoot);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
    LeftBarrier = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftBarrier")); LeftBarrier->SetupAttachment(SceneRoot); LeftBarrier->SetStaticMesh(CubeMesh); LeftBarrier->SetRelativeLocation(FVector(0,-220,60)); LeftBarrier->SetRelativeScale3D(FVector(.22f,1.65f,.65f)); LeftBarrier->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RightBarrier = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightBarrier")); RightBarrier->SetupAttachment(SceneRoot); RightBarrier->SetStaticMesh(CubeMesh); RightBarrier->SetRelativeLocation(FVector(0,220,60)); RightBarrier->SetRelativeScale3D(FVector(.22f,1.65f,.65f)); RightBarrier->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    SpikeStrip = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpikeStrip")); SpikeStrip->SetupAttachment(SceneRoot); SpikeStrip->SetStaticMesh(CubeMesh); SpikeStrip->SetRelativeLocation(FVector(95,0,8)); SpikeStrip->SetRelativeScale3D(FVector(.18f,4.25f,.07f)); SpikeStrip->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); SpikeStrip->SetNotifyRigidBodyCollision(true); SpikeStrip->OnComponentHit.AddDynamic(this,&AGTTRoadblock::HandleSpikeHit);
    Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign")); Sign->SetupAttachment(SceneRoot); Sign->SetRelativeLocation(FVector(-35,0,165)); Sign->SetRelativeRotation(FRotator(0,180,0)); Sign->SetHorizontalAlignment(EHTA_Center); Sign->SetWorldSize(46); Sign->SetTextRenderColor(FColor(255,70,55)); Sign->SetText(FText::FromString(TEXT("COUNTY ROADBLOCK\nSPIKE STRIP")));
}

void AGTTRoadblock::SetResponseTier(int32 NewTier)
{
    ResponseTier=FMath::Clamp(NewTier,1,2);
    if(Sign) Sign->SetText(FText::FromString(FString::Printf(TEXT("COUNTY ROADBLOCK T%d\nSPIKE STRIP"),ResponseTier)));
}

void AGTTRoadblock::HandleSpikeHit(UPrimitiveComponent*,AActor* OtherActor,UPrimitiveComponent*,FVector,const FHitResult&)
{
    AGTTVehicleBase* Vehicle=Cast<AGTTVehicleBase>(OtherActor); if(!Vehicle||!GetWorld()) return;
    const float Now=GetWorld()->GetTimeSeconds();
    if(LastSpikedActor.Get()==Vehicle && Now-LastSpikeHitTimeSeconds<SpikeRepeatCooldownSeconds) return;
    LastSpikedActor=Vehicle; LastSpikeHitTimeSeconds=Now;
    LastSpikedVehicleId=Vehicle->GetPersistentVehicleId(); LastTireIntegrityBefore=Vehicle->GetTireIntegrity();
    const float TireDamage=0.18f+ResponseTier*0.08f; const float BodyDamage=1.5f+ResponseTier*1.5f;
    Vehicle->ApplyTireDamage(TireDamage); Vehicle->ApplyVehicleDamage(BodyDamage);
    LastTireIntegrityAfter=Vehicle->GetTireIntegrity(); ++SpikeHitCount;
    UE_LOG(LogGTT,Warning,TEXT("ROADBLOCK_SPIKE_CONSEQUENCE vehicle=%s tier=%d hit=%d tire_before=%.3f tire_after=%.3f tire_delta=%.3f body_damage=%.1f speed_kmh=%.1f"),*LastSpikedVehicleId.ToString(),ResponseTier,SpikeHitCount,LastTireIntegrityBefore,LastTireIntegrityAfter,LastTireIntegrityBefore-LastTireIntegrityAfter,BodyDamage,Vehicle->GetSpeedKmh());
}
