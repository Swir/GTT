#include "Activities/GTTFieldCheckpoint.h"

#include "Activities/GTTRuralWorkDirector.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTTractorPawn.h"

AGTTFieldCheckpoint::AGTTFieldCheckpoint()
{
    PrimaryActorTick.bCanEverTick = false;
    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(300.0f, 430.0f, 180.0f));
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGTTFieldCheckpoint::HandleOverlap);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    GateLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateLeft"));
    GateLeft->SetupAttachment(Trigger);
    GateRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateRight"));
    GateRight->SetupAttachment(Trigger);
    for (UStaticMeshComponent* Gate : {GateLeft.Get(), GateRight.Get()})
    {
        if (CubeFinder.Succeeded()) Gate->SetStaticMesh(CubeFinder.Object);
        Gate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Gate->SetRelativeScale3D(FVector(0.12f, 0.12f, 2.2f));
    }
    GateLeft->SetRelativeLocation(FVector(0.0f, -360.0f, 170.0f));
    GateRight->SetRelativeLocation(FVector(0.0f, 360.0f, 170.0f));
}

void AGTTFieldCheckpoint::HandleOverlap(UPrimitiveComponent* Overlapped, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AGTTTractorPawn* Tractor = Cast<AGTTTractorPawn>(OtherActor);
    if (!Tractor || !Tractor->GetDriverPawn()) return;
    AGTTRuralWorkDirector* Director = Cast<AGTTRuralWorkDirector>(UGameplayStatics::GetActorOfClass(this, AGTTRuralWorkDirector::StaticClass()));
    if (Director) Director->TryMowingPass(Tractor->GetDriverPawn(), PassIndex);
}
