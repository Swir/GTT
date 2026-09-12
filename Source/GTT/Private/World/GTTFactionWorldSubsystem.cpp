#include "World/GTTFactionWorldSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NPC/GTTRuralFactionDirector.h"

void UGTTFactionWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if(!InWorld.IsGameWorld()) return;
    if(!UGameplayStatics::GetActorOfClass(&InWorld,AGTTRuralFactionDirector::StaticClass()))
    {
        InWorld.SpawnActor<AGTTRuralFactionDirector>();
    }
}
