#include "World/GTTMudZone.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
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

    TArray<AActor*> LegacyOverlaps;
    MudVolume->GetOverlappingActors(LegacyOverlaps, AGTTVehicleBase::StaticClass());
    for (AActor* Actor : LegacyOverlaps)
    {
        AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(Actor);
        UPrimitiveComponent* Root = Vehicle ? Cast<UPrimitiveComponent>(Vehicle->GetRootComponent()) : nullptr;
        if (!Vehicle || !Root || !Root->IsSimulatingPhysics()) continue;

        FVector HorizontalVelocity = Root->GetPhysicsLinearVelocity();
        HorizontalVelocity.Z = 0.0f;

        Vehicle->ApplyTerrainDynamicsModifier(0.52f, DragStrength * 18.0f, 0.28f);
        Root->AddForce(-HorizontalVelocity * Root->GetMass() * DragStrength * 0.35f, NAME_None, false);

        if (HorizontalVelocity.SizeSquared() > FMath::Square(250.0f))
        {
            Vehicle->ApplyTireDamage(TireWearPerSecond * DeltaSeconds);
        }
    }

    // Native Chaos takeover must consume the same authored mud volumes as the legacy
    // drivetrain so legal farm work, heavy haul and countryside routes do not become
    // easier simply because the Fieldmaster migrated to AWheeledVehiclePawn.
    TArray<AActor*> NativeOverlaps;
    MudVolume->GetOverlappingActors(NativeOverlaps, AGTTFieldmasterNativePawn::StaticClass());
    for (AActor* Actor : NativeOverlaps)
    {
        AGTTFieldmasterNativePawn* Fieldmaster = Cast<AGTTFieldmasterNativePawn>(Actor);
        if (!Fieldmaster || !Fieldmaster->IsNativeFieldmasterReady() || !Fieldmaster->IsLegacyTakeoverActive())
        {
            continue;
        }

        Fieldmaster->ApplyNativeMudResponse(DragStrength, TireWearPerSecond, DeltaSeconds);
    }
}
