#include "Activities/GTTFarmRouteGuidanceSubsystem.h"

#include "Activities/GTTFarmRouteBeacon.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void UGTTFarmRouteGuidanceSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!InWorld.IsGameWorld()) return;

    for (TActorIterator<AGTTFarmRouteBeacon> It(&InWorld); It; ++It)
    {
        if (IsValid(*It))
        {
            RouteBeacon = *It;
            return;
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    RouteBeacon = InWorld.SpawnActor<AGTTFarmRouteBeacon>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
}
