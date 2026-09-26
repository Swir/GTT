#include "Vehicles/GTTNativeFleetTakeoverSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "GTT.h"

void UGTTNativeFleetTakeoverSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (InWorld.WorldType != EWorldType::Game && InWorld.WorldType != EWorldType::PIE) return;

    if (!UGameplayStatics::GetActorOfClass(&InWorld, AGTTRattlebackNativePawn::StaticClass()))
    {
        InWorld.SpawnActor<AGTTRattlebackNativePawn>(AGTTRattlebackNativePawn::StaticClass(), FTransform::Identity);
    }
    if (!UGameplayStatics::GetActorOfClass(&InWorld, AGTTFieldmasterNativePawn::StaticClass()))
    {
        InWorld.SpawnActor<AGTTFieldmasterNativePawn>(AGTTFieldmasterNativePawn::StaticClass(), FTransform::Identity);
    }
    if (!UGameplayStatics::GetActorOfClass(&InWorld, AGTTMuleboxNativePawn::StaticClass()))
    {
        InWorld.SpawnActor<AGTTMuleboxNativePawn>(AGTTMuleboxNativePawn::StaticClass(), FTransform::Identity);
    }

    GTT_LOG( Log, TEXT("NATIVE_FLEET_TAKEOVER_BOOT vehicles=RustyFieldmaster60,Rattleback82,Mulebox1200 mode=acceptance-gated"));
}
