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

    FActorSpawnParameters SpawnParameters;
    // The native pawns begin hidden and collision-disabled, but collision handling is evaluated
    // before BeginPlay. AlwaysSpawn prevents later fleet members from being rejected because all
    // three bootstrap at the identity transform before they mirror their legacy counterpart.
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* Rattleback = UGameplayStatics::GetActorOfClass(&InWorld, AGTTRattlebackNativePawn::StaticClass());
    if (!Rattleback)
    {
        Rattleback = InWorld.SpawnActor<AGTTRattlebackNativePawn>(AGTTRattlebackNativePawn::StaticClass(), FTransform::Identity, SpawnParameters);
    }
    AActor* Fieldmaster = UGameplayStatics::GetActorOfClass(&InWorld, AGTTFieldmasterNativePawn::StaticClass());
    if (!Fieldmaster)
    {
        Fieldmaster = InWorld.SpawnActor<AGTTFieldmasterNativePawn>(AGTTFieldmasterNativePawn::StaticClass(), FTransform::Identity, SpawnParameters);
    }
    AActor* Mulebox = UGameplayStatics::GetActorOfClass(&InWorld, AGTTMuleboxNativePawn::StaticClass());
    if (!Mulebox)
    {
        Mulebox = InWorld.SpawnActor<AGTTMuleboxNativePawn>(AGTTMuleboxNativePawn::StaticClass(), FTransform::Identity, SpawnParameters);
    }

    const int32 SpawnedCount = (Fieldmaster ? 1 : 0) + (Rattleback ? 1 : 0) + (Mulebox ? 1 : 0);
    if (SpawnedCount == 3)
    {
        GTT_LOG(Log, TEXT("NATIVE_FLEET_TAKEOVER_BOOT vehicles=RustyFieldmaster60,Rattleback82,Mulebox1200 mode=acceptance-gated spawned=%d/3 fieldmaster=READY rattleback=READY mulebox=READY"), SpawnedCount);
    }
    else
    {
        GTT_LOG(Error,
            TEXT("NATIVE_FLEET_TAKEOVER_BOOT vehicles=RustyFieldmaster60,Rattleback82,Mulebox1200 mode=acceptance-gated spawned=%d/3 fieldmaster=%s rattleback=%s mulebox=%s"),
            SpawnedCount,
            Fieldmaster ? TEXT("READY") : TEXT("MISSING"),
            Rattleback ? TEXT("READY") : TEXT("MISSING"),
            Mulebox ? TEXT("READY") : TEXT("MISSING"));
    }
}
