#include "Traffic/GTTTrafficDirector.h"

#include "Algo/Reverse.h"
#include "Engine/World.h"
#include "Traffic/GTTTrafficCarPawn.h"
#include "World/GTTRoadGraph.h"

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
    if (!GetWorld() || !TrafficCarClass) return;

    const TArray<FVector> ClockwiseRoute = FGTTRoadGraph::GetVillageLoop();
    TArray<FVector> CounterClockwiseRoute = ClockwiseRoute;
    Algo::Reverse(CounterClockwiseRoute);

    const int32 SafeCount = FMath::Clamp(TrafficCarCount, 1, FMath::Max(1, ClockwiseRoute.Num()));
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

    // Two rural commuters now use the same graph as police and mission routing.
    const int32 VillageSouth = FGTTRoadGraph::FindClosestNode(FVector(0,-1800,100));
    const int32 NorthWood = FGTTRoadGraph::FindClosestNode(FVector(7850,900,100));
    const int32 FarmNorth = FGTTRoadGraph::FindClosestNode(FVector(-3300,1800,100));
    const int32 HillFarm = FGTTRoadGraph::FindClosestNode(FVector(5850,3100,100));
    const TArray<TArray<FVector>> RuralRoutes = {
        FGTTRoadGraph::BuildRoute(VillageSouth, NorthWood),
        FGTTRoadGraph::BuildRoute(FarmNorth, HillFarm)
    };
    for (int32 RouteIndex=0; RouteIndex<RuralRoutes.Num(); ++RouteIndex)
    {
        const TArray<FVector>& Route = RuralRoutes[RouteIndex];
        if (Route.Num() < 2) continue;
        const FVector Spawn = Route[0] + FVector(0, RouteIndex == 0 ? 85.0f : -85.0f, 0);
        const FRotator Rot = (Route[1] - Route[0]).Rotation();
        if (AGTTTrafficCarPawn* TrafficCar = GetWorld()->SpawnActor<AGTTTrafficCarPawn>(TrafficCarClass, Spawn, Rot))
        {
            TrafficCar->InitializeRoute(Route, 0);
        }
    }
}
