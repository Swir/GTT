#include "Traffic/GTTTrafficDirector.h"

#include "Engine/World.h"
#include "Traffic/GTTTrafficCarPawn.h"

AGTTTrafficDirector::AGTTTrafficDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    TrafficCarClass = AGTTTrafficCarPawn::StaticClass();
}

void AGTTTrafficDirector::BeginPlay()
{
    Super::BeginPlay();
    SpawnTrafficLoop();
}

void AGTTTrafficDirector::SpawnTrafficLoop()
{
    if (!GetWorld() || !TrafficCarClass)
    {
        return;
    }

    const TArray<FVector> Route = {
        FVector(-3100.0f, -1800.0f, 90.0f),
        FVector(0.0f, -1800.0f, 90.0f),
        FVector(3100.0f, -1800.0f, 90.0f),
        FVector(3300.0f, 0.0f, 90.0f),
        FVector(3100.0f, 1800.0f, 90.0f),
        FVector(0.0f, 1800.0f, 90.0f),
        FVector(-3100.0f, 1800.0f, 90.0f),
        FVector(-3300.0f, 0.0f, 90.0f)
    };

    const int32 SafeCount = FMath::Clamp(TrafficCarCount, 1, Route.Num());
    for (int32 Index = 0; Index < SafeCount; ++Index)
    {
        const int32 StartIndex = (Index * Route.Num()) / SafeCount;
        const FRotator SpawnRotation(0.0f, Index % 2 == 0 ? 0.0f : 180.0f, 0.0f);
        if (AGTTTrafficCarPawn* TrafficCar = GetWorld()->SpawnActor<AGTTTrafficCarPawn>(TrafficCarClass, Route[StartIndex], SpawnRotation))
        {
            TrafficCar->InitializeRoute(Route, StartIndex);
        }
    }
}
