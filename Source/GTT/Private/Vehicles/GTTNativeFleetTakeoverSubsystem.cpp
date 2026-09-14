#include "Vehicles/GTTNativeFleetTakeoverSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTT.h"

void UGTTNativeFleetTakeoverSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (InWorld.WorldType != EWorldType::Game && InWorld.WorldType != EWorldType::PIE) return;

    if (!UGameplayStatics::GetActorOfClass(&InWorld, AGTTRattlebackNativePawn::StaticClass()))
    {
        InWorld.SpawnActor<AGTTRattlebackNativePawn>(AGTTRattlebackNativePawn::StaticClass(), FTransform::Identity);
    }
    if (!UGameplayStatics::GetActorOfClass(&InWorld, AGTTMuleboxNativePawn::StaticClass()))
    {
        InWorld.SpawnActor<AGTTMuleboxNativePawn>(AGTTMuleboxNativePawn::StaticClass(), FTransform::Identity);
    }

    UE_LOG(LogGTT, Log, TEXT("NATIVE_FLEET_TAKEOVER_BOOT vehicles=Rattleback82,Mulebox1200 mode=acceptance-gated"));
}
