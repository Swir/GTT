#include "Ranger/GTTRangerDirector.h"

#include "Core/GTTGameMode.h"
#include "Engine/World.h"
#include "Ranger/GTTRangerPawn.h"
#include "TimerManager.h"

AGTTRangerDirector::AGTTRangerDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    RangerClass = AGTTRangerPawn::StaticClass();
}

void AGTTRangerDirector::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimer(ResponseTimer, this, &AGTTRangerDirector::UpdateResponse, ResponseInterval, true, 0.35f);
}

void AGTTRangerDirector::CleanupInvalidRangers()
{
    ActiveRangers.RemoveAll([](const TWeakObjectPtr<AGTTRangerPawn>& Ranger)
    {
        return !Ranger.IsValid();
    });
}

void AGTTRangerDirector::UpdateResponse()
{
    AGTTGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGTTGameMode>() : nullptr;
    if (!GameMode || !GetWorld())
    {
        return;
    }

    CleanupInvalidRangers();
    const int32 AlertLevel = GameMode->GetWildlifeAlertLevel();
    const int32 DesiredRangers = AlertLevel > 0 ? FMath::Clamp(AlertLevel, 1, 2) : 0;

    while (ActiveRangers.Num() > DesiredRangers)
    {
        if (AGTTRangerPawn* Ranger = ActiveRangers.Last().Get())
        {
            Ranger->Destroy();
        }
        ActiveRangers.Pop();
    }

    const FVector SpawnLocations[2] = {
        FVector(3850.0f, 450.0f, 120.0f),
        FVector(5200.0f, 300.0f, 120.0f)
    };

    while (ActiveRangers.Num() < DesiredRangers && RangerClass)
    {
        const int32 Index = ActiveRangers.Num();
        if (AGTTRangerPawn* Ranger = GetWorld()->SpawnActor<AGTTRangerPawn>(RangerClass, SpawnLocations[Index], FRotator::ZeroRotator))
        {
            ActiveRangers.Add(Ranger);
        }
        else
        {
            break;
        }
    }
}
