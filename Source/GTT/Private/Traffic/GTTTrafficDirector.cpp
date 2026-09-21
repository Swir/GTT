#include "Traffic/GTTTrafficDirector.h"

#include "Algo/Reverse.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Traffic/GTTTrafficCarPawn.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTRoadGraph.h"
#include "GTT.h"

namespace
{
TArray<FVector> MakeRoundTrip(const TArray<FVector>& Outbound)
{
    TArray<FVector> Route = Outbound;
    for (int32 Index = Outbound.Num() - 2; Index > 0; --Index)
    {
        Route.Add(Outbound[Index]);
    }
    return Route;
}

void OffsetRouteToDrivingLane(TArray<FVector>& Route, float LaneOffsetCm)
{
    if (Route.Num() < 2 || FMath::IsNearlyZero(LaneOffsetCm))
    {
        return;
    }

    const TArray<FVector> CenterLine = Route;
    for (int32 Index = 0; Index < CenterLine.Num(); ++Index)
    {
        const int32 PreviousIndex = (Index - 1 + CenterLine.Num()) % CenterLine.Num();
        const int32 NextIndex = (Index + 1) % CenterLine.Num();
        const FVector Tangent = (CenterLine[NextIndex] - CenterLine[PreviousIndex]).GetSafeNormal2D();
        if (Tangent.IsNearlyZero())
        {
            continue;
        }
        const FVector Right = FVector::CrossProduct(FVector::UpVector, Tangent).GetSafeNormal2D();
        Route[Index] += Right * LaneOffsetCm;
    }
}
}

AGTTTrafficDirector::AGTTTrafficDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    TrafficCarClass = AGTTTrafficCarPawn::StaticClass();
}

void AGTTTrafficDirector::BeginPlay()
{
    Super::BeginPlay();
    CacheDayNightCycle();
    SetActorTickInterval(PopulationUpdateIntervalSeconds);
    ReconcileTrafficPopulation(true);
}

void AGTTTrafficDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!DayNightCycle.IsValid())
    {
        CacheDayNightCycle();
    }
    ReconcileTrafficPopulation(false);
}

void AGTTTrafficDirector::CacheDayNightCycle()
{
    DayNightCycle.Reset();
    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<AGTTDayNightCycle> It(GetWorld()); It; ++It)
    {
        DayNightCycle = *It;
        break;
    }
}

void AGTTTrafficDirector::CompactManagedTraffic()
{
    ManagedTrafficCars.RemoveAllSwap([](const TWeakObjectPtr<AGTTTrafficCarPawn>& TrafficCar)
    {
        return !TrafficCar.IsValid();
    });
}

int32 AGTTTrafficDirector::CalculateTargetTrafficCount(FString& OutProfileText) const
{
    const int32 BasePopulation = FMath::Max(1, TrafficCarCount) + FMath::Max(0, RuralCommuterCount);
    float Hour = 12.0f;
    bool bNight = false;

    if (const AGTTDayNightCycle* Clock = DayNightCycle.Get())
    {
        Hour = Clock->GetTimeOfDayHours();
        bNight = Clock->IsNight();
    }

    const bool bMorningRush = Hour >= 6.0f && Hour < 9.0f;
    const bool bEveningRush = Hour >= 16.0f && Hour < 19.5f;

    int32 DesiredPopulation = BasePopulation;
    if (bNight)
    {
        OutProfileText = TEXT("NIGHT");
        DesiredPopulation -= FMath::Max(0, NightTrafficReduction);
    }
    else if (bMorningRush || bEveningRush)
    {
        OutProfileText = TEXT("RUSH");
        DesiredPopulation += FMath::Max(0, RushHourBonus);
    }
    else
    {
        OutProfileText = TEXT("DAY");
    }

    return FMath::Clamp(DesiredPopulation, 2, FMath::Max(2, MaximumManagedTraffic));
}

TArray<FVector> AGTTTrafficDirector::BuildSpawnRoute(int32 SpawnSerial, int32& OutStartIndex) const
{
    TArray<FVector> Route;
    const int32 RouteKind = FMath::Abs(SpawnSerial) % 5;

    if (RouteKind == 0)
    {
        const int32 VillageSouth = FGTTRoadGraph::FindClosestNode(FVector(0.0f, -1800.0f, 100.0f));
        const int32 NorthWood = FGTTRoadGraph::FindClosestNode(FVector(7850.0f, 900.0f, 100.0f));
        Route = MakeRoundTrip(FGTTRoadGraph::BuildRoute(VillageSouth, NorthWood));
    }
    else if (RouteKind == 1)
    {
        const int32 FarmNorth = FGTTRoadGraph::FindClosestNode(FVector(-3300.0f, 1800.0f, 100.0f));
        const int32 HillFarm = FGTTRoadGraph::FindClosestNode(FVector(5850.0f, 3100.0f, 100.0f));
        Route = MakeRoundTrip(FGTTRoadGraph::BuildRoute(FarmNorth, HillFarm));
    }
    else
    {
        Route = FGTTRoadGraph::GetVillageLoop();
    }

    if (Route.Num() < 2)
    {
        OutStartIndex = 0;
        return Route;
    }

    if ((SpawnSerial % 2) != 0)
    {
        Algo::Reverse(Route);
    }

    // Both travel directions use their own right-hand lane. Reversing the route
    // reverses the tangent, so this single positive offset separates opposing cars.
    OffsetRouteToDrivingLane(Route, 95.0f);
    OutStartIndex = (FMath::Abs(SpawnSerial) * 3) % Route.Num();
    return Route;
}

bool AGTTTrafficDirector::SpawnManagedTrafficCar()
{
    if (!GetWorld() || !TrafficCarClass)
    {
        return false;
    }

    int32 StartIndex = 0;
    TArray<FVector> Route = BuildSpawnRoute(SpawnSequence, StartIndex);
    ++SpawnSequence;
    if (Route.Num() < 2)
    {
        return false;
    }

    const int32 NextIndex = (StartIndex + 1) % Route.Num();
    const FVector SpawnLocation = Route[StartIndex] + FVector(0.0f, 0.0f, 90.0f);
    const FRotator SpawnRotation = (Route[NextIndex] - Route[StartIndex]).Rotation();

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AGTTTrafficCarPawn* TrafficCar = GetWorld()->SpawnActor<AGTTTrafficCarPawn>(TrafficCarClass, SpawnLocation, SpawnRotation, SpawnParameters);
    if (!TrafficCar)
    {
        return false;
    }

    TrafficCar->Tags.AddUnique(FName(TEXT("GTT.ManagedTraffic")));
    TrafficCar->InitializeRoute(Route, StartIndex);
    ManagedTrafficCars.Add(TrafficCar);
    return true;
}

bool AGTTTrafficDirector::CanCullManagedCar(const AGTTTrafficCarPawn* TrafficCar) const
{
    return IsValid(TrafficCar)
        && !TrafficCar->IsOccupied()
        && !TrafficCar->IsIncidentDisabled()
        && !TrafficCar->IsRoadsideAssistanceActive()
        && !TrafficCar->IsRoadsideResponderSceneAuthority()
        && !TrafficCar->IsYieldingForRangerStop()
        && !TrafficCar->IsHoldingForRangerStop();
}

bool AGTTTrafficDirector::CullOneManagedTrafficCar()
{
    if (!GetWorld())
    {
        return false;
    }

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!PlayerPawn)
    {
        // Population may temporarily remain above target while the player is not
        // present; destroying a potentially possessed/important car is worse.
        return false;
    }

    const float MinimumDistanceSq = FMath::Square(MinimumCullDistanceFromPlayer);
    int32 CandidateIndex = INDEX_NONE;
    float FarthestDistanceSq = MinimumDistanceSq;

    for (int32 Index = 0; Index < ManagedTrafficCars.Num(); ++Index)
    {
        AGTTTrafficCarPawn* TrafficCar = ManagedTrafficCars[Index].Get();
        if (!CanCullManagedCar(TrafficCar))
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared2D(PlayerPawn->GetActorLocation(), TrafficCar->GetActorLocation());
        if (DistanceSq > FarthestDistanceSq)
        {
            CandidateIndex = Index;
            FarthestDistanceSq = DistanceSq;
        }
    }

    if (CandidateIndex == INDEX_NONE)
    {
        return false;
    }

    if (AGTTTrafficCarPawn* TrafficCar = ManagedTrafficCars[CandidateIndex].Get())
    {
        TrafficCar->Destroy();
    }
    ManagedTrafficCars.RemoveAtSwap(CandidateIndex);
    return true;
}

void AGTTTrafficDirector::ReconcileTrafficPopulation(bool bFillImmediately)
{
    CompactManagedTraffic();

    FString NewProfile;
    const int32 NewTarget = CalculateTargetTrafficCount(NewProfile);
    const bool bProfileChanged = NewTarget != TargetTrafficCount || NewProfile != TrafficProfileText;
    TargetTrafficCount = NewTarget;
    TrafficProfileText = MoveTemp(NewProfile);

    int32 Adjustments = 0;
    if (ManagedTrafficCars.Num() < TargetTrafficCount)
    {
        const int32 MissingCars = TargetTrafficCount - ManagedTrafficCars.Num();
        const int32 SpawnBudget = bFillImmediately ? MissingCars : FMath::Min(MissingCars, MaxPopulationAdjustmentPerPass);
        for (int32 Index = 0; Index < SpawnBudget; ++Index)
        {
            if (!SpawnManagedTrafficCar())
            {
                break;
            }
            ++Adjustments;
        }
    }
    else if (ManagedTrafficCars.Num() > TargetTrafficCount)
    {
        const int32 ExcessCars = ManagedTrafficCars.Num() - TargetTrafficCount;
        const int32 CullBudget = FMath::Min(ExcessCars, MaxPopulationAdjustmentPerPass);
        for (int32 Index = 0; Index < CullBudget; ++Index)
        {
            if (!CullOneManagedTrafficCar())
            {
                break;
            }
            ++Adjustments;
        }
    }

    if (bProfileChanged || Adjustments > 0)
    {
        UE_LOG(LogGTT, Log,
            TEXT("TRAFFIC_POPULATION profile=%s target=%d live=%d adjustments=%d clock=%s"),
            *TrafficProfileText,
            TargetTrafficCount,
            ManagedTrafficCars.Num(),
            Adjustments,
            DayNightCycle.IsValid() ? *DayNightCycle->GetClockText() : TEXT("NO_CLOCK"));
    }
}
