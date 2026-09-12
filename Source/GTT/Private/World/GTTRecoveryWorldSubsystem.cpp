#include "World/GTTRecoveryWorldSubsystem.h"

#include "Activities/GTTRecoveryDirector.h"
#include "Activities/GTTRecoveryTerminal.h"
#include "Engine/World.h"
#include "World/GTTMudZone.h"

void UGTTRecoveryWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!InWorld.IsGameWorld()) return;

    AGTTRecoveryDirector* Director = InWorld.SpawnActor<AGTTRecoveryDirector>();
    if (Director)
    {
        if (AGTTRecoveryTerminal* Workshop = InWorld.SpawnActor<AGTTRecoveryTerminal>(FVector(-400.0f, 2250.0f, 55.0f), FRotator::ZeroRotator))
            Workshop->Configure(EGTTRecoveryTerminalType::Workshop, Director);
        if (AGTTRecoveryTerminal* Hook = InWorld.SpawnActor<AGTTRecoveryTerminal>(FVector(4300.0f, 1650.0f, 55.0f), FRotator::ZeroRotator))
            Hook->Configure(EGTTRecoveryTerminalType::Hook, Director);
    }

    if (AGTTMudZone* HillFarmMud = InWorld.SpawnActor<AGTTMudZone>(FVector(6650.0f, 4450.0f, 20.0f), FRotator::ZeroRotator))
        HillFarmMud->Configure(FVector(1450.0f, 1250.0f, 140.0f), 2.2f, 0.010f);

    if (AGTTMudZone* ForestTrackMud = InWorld.SpawnActor<AGTTMudZone>(FVector(6250.0f, -850.0f, 20.0f), FRotator(0.0f, 18.0f, 0.0f)))
        ForestTrackMud->Configure(FVector(1100.0f, 420.0f, 140.0f), 2.8f, 0.014f);
}
