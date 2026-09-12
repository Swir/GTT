#include "World/GTTMudZone.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Vehicles/GTTVehicleBase.h"

AGTTMudZone::AGTTMudZone()
{
    PrimaryActorTick.bCanEverTick = true;
    MudVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("MudVolume"));
    RootComponent = MudVolume;
    MudVolume->SetBoxExtent(FVector(700.0f, 700.0f, 120.0f));
    MudVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MudVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void AGTTMudZone::Configure(const FVector& HalfExtent, float InDragStrength, float InDamagePerSecond)
{
    MudVolume->SetBoxExtent(HalfExtent);
    DragStrength = FMath::Max(0.0f, InDragStrength);
    TireWearPerSecond = FMath::Max(0.0f, InDamagePerSecond);
}

void AGTTMudZone::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TArray<AActor*> Overlaps;
    MudVolume->GetOverlappingActors(Overlaps, AGTTVehicleBase::StaticClass());
    for (AActor* Actor : Overlaps)
    {
        AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(Actor);
        UPrimitiveComponent* Root = Vehicle ? Cast<UPrimitiveComponent>(Vehicle->GetRootComponent()) : nullptr;
        if (!Vehicle || !Root || !Root->IsSimulatingPhysics()) continue;

        FVector HorizontalVelocity = Root->GetPhysicsLinearVelocity();
        HorizontalVelocity.Z = 0.0f;

        // The dynamics component owns most of the muddy handling response: reduced grip,
        // increased rolling resistance and vehicle-profile-specific off-road recovery.
        Vehicle->ApplyTerrainDynamicsModifier(0.52f, DragStrength * 18.0f, 0.28f);

        // Keep a smaller physical drag impulse so deep mud still visibly bleeds momentum
        // even when the driver is coasting or a recovery target is being towed.
        Root->AddForce(-HorizontalVelocity * Root->GetMass() * DragStrength * 0.35f, NAME_None, false);

        if (HorizontalVelocity.SizeSquared() > FMath::Square(250.0f))
            Vehicle->ApplyTireDamage(TireWearPerSecond * DeltaSeconds);
    }
}
