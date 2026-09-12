#include "World/GTTHeavyHaulWorldSubsystem.h"

#include "Activities/GTTHeavyHaulDirector.h"
#include "Activities/GTTHeavyHaulTerminal.h"
#include "Engine/World.h"

void UGTTHeavyHaulWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!InWorld.IsGameWorld()) return;

    AGTTHeavyHaulDirector* Director = InWorld.SpawnActor<AGTTHeavyHaulDirector>();
    if (!Director) return;

    if (AGTTHeavyHaulTerminal* Board = InWorld.SpawnActor<AGTTHeavyHaulTerminal>(FVector(-2780.0f, -1170.0f, 55.0f), FRotator::ZeroRotator))
        Board->Configure(EGTTHeavyHaulTerminalType::ContractBoard, Director);

    if (AGTTHeavyHaulTerminal* Hitch = InWorld.SpawnActor<AGTTHeavyHaulTerminal>(FVector(-3200.0f, -1500.0f, 55.0f), FRotator::ZeroRotator))
        Hitch->Configure(EGTTHeavyHaulTerminalType::Hitch, Director);

    if (AGTTHeavyHaulTerminal* Load = InWorld.SpawnActor<AGTTHeavyHaulTerminal>(FVector(7850.0f, 450.0f, 55.0f), FRotator::ZeroRotator))
        Load->Configure(EGTTHeavyHaulTerminalType::Load, Director);

    if (AGTTHeavyHaulTerminal* Deliver = InWorld.SpawnActor<AGTTHeavyHaulTerminal>(FVector(5850.0f, 2550.0f, 55.0f), FRotator::ZeroRotator))
        Deliver->Configure(EGTTHeavyHaulTerminalType::Deliver, Director);
}
