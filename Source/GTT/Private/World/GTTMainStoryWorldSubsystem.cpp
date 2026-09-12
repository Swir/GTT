#include "World/GTTMainStoryWorldSubsystem.h"

#include "Engine/World.h"
#include "Missions/GTTMainStoryDirector.h"
#include "Missions/GTTMainStoryTerminal.h"

void UGTTMainStoryWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!InWorld.IsGameWorld()) return;

    InWorld.SpawnActor<AGTTMainStoryDirector>();

    struct FStoryTerminalSpawn
    {
        FVector Location;
        EGTTMainStoryTerminalType Type;
    };

    const FStoryTerminalSpawn Terminals[] =
    {
        {FVector(-2720.0f, -650.0f, 55.0f), EGTTMainStoryTerminalType::FarmOffice},
        {FVector(7550.0f, 760.0f, 55.0f), EGTTMainStoryTerminalType::NorthWood},
        {FVector(1120.0f, -2200.0f, 55.0f), EGTTMainStoryTerminalType::VillageShop},
        {FVector(1650.0f, 2250.0f, 55.0f), EGTTMainStoryTerminalType::Tavern},
        {FVector(4700.0f, 1650.0f, 55.0f), EGTTMainStoryTerminalType::EastRoad},
        {FVector(100.0f, 2250.0f, 55.0f), EGTTMainStoryTerminalType::Workshop}
    };

    for (const FStoryTerminalSpawn& Spawn : Terminals)
    {
        if (AGTTMainStoryTerminal* Terminal = InWorld.SpawnActor<AGTTMainStoryTerminal>(Spawn.Location, FRotator::ZeroRotator))
        {
            Terminal->SetTerminalType(Spawn.Type);
        }
    }
}
