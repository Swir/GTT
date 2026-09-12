#include "Traffic/GTTTrafficDirector.h"

#include "Algo/Reverse.h"
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

    const TArray<FVector> ClockwiseRoute = {
        FVector(-3100.0f, -1800.0f, 90.0f),
        FVector(0.0f, -1800.0f, 90.0f),
        FVector(3100.0f, -1800.0f, 90.0f),
        FVector(3300.0f, 0.0f, 90.0f),
        FVector(3100.0f, 1800.0f, 90.0f),
        FVector(0.0f, 1800.0f, 90.0f),
        FVector(-3100.0f, 1800.0f, 90.0f),
        FVector(-3300.0f, 0.0f, 90.0f)
    };

    TArray<FVector> CounterClockwiseRoute = ClockwiseRoute;
    Algo::Reverse(CounterClockwiseRoute);

    const int32 SafeCount = FMath::Clamp(TrafficCarCount, 1, ClockwiseRoute.Num());
    for (int32 Index = 0; Index < SafeCount; ++Index)
    {
        const bool bUseClockwise = (Index % 2) == 0;
        const TArray<FVector>& Route = bUseClockwise ? ClockwiseRoute : CounterClockwiseRoute;
        const int32 StartIndex = (Index * Route.Num()) / SafeCount;
        const FVector SpawnLocation = Route[StartIndex] + FVector(0.0f, bUseClockwise ? -95.0f : 95.0f, 0.0f);
        const FRotator SpawnRotation(0.0f, bUseClockwise ? 0.0f : 180.0f, 0.0f);

        if (AGTTTrafficCarPawn* TrafficCar = GetWorld()->SpawnActor<AGTTTrafficCarPawn>(TrafficCarClass, SpawnLocation, SpawnRotation))
        {
            TrafficCar->InitializeRoute(Route, StartIndex);
        }
    }
}
