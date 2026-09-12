#include "Police/GTTPoliceDirector.h"

#include "Core/GTTGameplayStatics.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Police/GTTPolicePawn.h"
#include "Police/GTTPolicePursuitVehicle.h"
#include "Police/GTTRoadblock.h"
#include "TimerManager.h"

AGTTPoliceDirector::AGTTPoliceDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    PolicePawnClass = AGTTPolicePawn::StaticClass();
    PursuitVehicleClass = AGTTPolicePursuitVehicle::StaticClass();
    RoadblockClass = AGTTRoadblock::StaticClass();
}

void AGTTPoliceDirector::BeginPlay()
{
    Super::BeginPlay();
    BuildRuntimeRoadNetwork();
    GetWorldTimerManager().SetTimer(EvaluationTimer, this, &AGTTPoliceDirector::EvaluatePoliceResponse, EvaluationInterval, true, 0.25f);
}

void AGTTPoliceDirector::BuildRuntimeRoadNetwork()
{
    RoadNodes = {
        FVector(-3300.0f,-1800.0f,100.0f), FVector(0.0f,-1800.0f,100.0f), FVector(3300.0f,-1800.0f,100.0f),
        FVector(3300.0f,0.0f,100.0f), FVector(3300.0f,1800.0f,100.0f), FVector(0.0f,1800.0f,100.0f),
        FVector(-3300.0f,1800.0f,100.0f), FVector(-3300.0f,0.0f,100.0f), FVector(4350.0f,1900.0f,100.0f),
        FVector(5200.0f,2550.0f,100.0f), FVector(6100.0f,650.0f,100.0f), FVector(7350.0f,650.0f,100.0f),
        FVector(4700.0f,-500.0f,100.0f), FVector(1850.0f,3400.0f,100.0f), FVector(-650.0f,2400.0f,100.0f)
    };
    RoadNodeLabels = {
        TEXT("WEST SOUTH JUNCTION"), TEXT("VILLAGE SOUTH"), TEXT("POLICE SOUTH"), TEXT("EAST CROSSROAD"),
        TEXT("EAST ROAD"), TEXT("VILLAGE NORTH"), TEXT("FARM NORTH"), TEXT("FARM CROSSROAD"), TEXT("NEIGHBOR BEND"),
        TEXT("HILL FARM TURN"), TEXT("FOREST TRACK"), TEXT("NORTH WOOD TURN"), TEXT("PRIVATE LAKE ROAD"),
        TEXT("FEED DEPOT ROAD"), TEXT("WORKSHOP ROAD")
    };
}

void AGTTPoliceDirector::EvaluatePoliceResponse()
{
    RemoveInvalidUnits();
    const int32 WantedLevel = GetPlayerWantedLevel();
    const int32 DesiredFootUnits = WantedLevel > 0 ? FMath::Clamp(WantedLevel * UnitsPerWantedLevel - FMath::Max(0, WantedLevel - 2), 0, MaxPoliceUnits) : 0;
    const int32 DesiredVehicles = WantedLevel >= VehicleEscalationWantedLevel ? FMath::Clamp(WantedLevel - VehicleEscalationWantedLevel + 1, 1, MaxPursuitVehicles) : 0;
    const int32 DesiredRoadblocks = WantedLevel >= RoadblockEscalationWantedLevel ? FMath::Clamp(WantedLevel - RoadblockEscalationWantedLevel + 1, 1, MaxRoadblocks) : 0;

    if (WantedLevel != LastResponseLevel)
    {
        LastResponseLevel = WantedLevel;
        if (WantedLevel < RoadblockEscalationWantedLevel) LastInterceptionNodeIndex = INDEX_NONE;
        OnResponseLevelChanged(WantedLevel, DesiredFootUnits + DesiredVehicles + DesiredRoadblocks);
    }

    DespawnExcessUnits(DesiredFootUnits, DesiredVehicles, DesiredRoadblocks);
    if (WantedLevel > 0 && ActivePoliceUnits.Num() < DesiredFootUnits) SpawnPoliceUnit();
    if (DesiredVehicles > 0 && ActivePursuitVehicles.Num() < DesiredVehicles) SpawnPursuitVehicle(WantedLevel);
    if (DesiredRoadblocks > 0 && ActiveRoadblocks.Num() < DesiredRoadblocks) SpawnRoadblock(WantedLevel);
}

void AGTTPoliceDirector::SpawnPoliceUnit()
{
    if (!PolicePawnClass || !GetWorld()) return;
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if (APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PolicePawnClass, SelectSpawnTransform(), Params)) ActivePoliceUnits.Add(SpawnedPawn);
}

void AGTTPoliceDirector::SpawnPursuitVehicle(int32 WantedLevel)
{
    if (!PursuitVehicleClass || !GetWorld()) return;
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    const FTransform SpawnTransform = WantedLevel >= RoadblockEscalationWantedLevel ? SelectPursuitInterceptTransform(WantedLevel) : SelectSpawnTransform(1.35f);
    if (AGTTPolicePursuitVehicle* Vehicle = GetWorld()->SpawnActor<AGTTPolicePursuitVehicle>(PursuitVehicleClass, SpawnTransform, Params))
    {
        Vehicle->SetResponseTier(FMath::Clamp(WantedLevel - 2, 1, 3));
        ActivePursuitVehicles.Add(Vehicle);
    }
}

void AGTTPoliceDirector::SpawnRoadblock(int32 WantedLevel)
{
    if (!RoadblockClass || !GetWorld()) return;
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if (AGTTRoadblock* Roadblock = GetWorld()->SpawnActor<AGTTRoadblock>(RoadblockClass, SelectRoadblockTransform(WantedLevel), Params))
    {
        Roadblock->SetResponseTier(FMath::Clamp(WantedLevel - 3, 1, 2));
        ActiveRoadblocks.Add(Roadblock);
    }
}

int32 AGTTPoliceDirector::GetPlayerWantedLevel() const { return UGTTGameplayStatics::GetPlayerWantedLevel(this, 0); }
FString AGTTPoliceDirector::GetLastInterceptionNodeLabel() const { return RoadNodeLabels.IsValidIndex(LastInterceptionNodeIndex) ? RoadNodeLabels[LastInterceptionNodeIndex] : TEXT("NONE"); }

void AGTTPoliceDirector::RemoveInvalidUnits()
{
    ActivePoliceUnits.RemoveAll([](const TObjectPtr<APawn>& Unit){ return !IsValid(Unit.Get()); });
    ActivePursuitVehicles.RemoveAll([](const TObjectPtr<AGTTPolicePursuitVehicle>& Unit){ return !IsValid(Unit.Get()); });
    ActiveRoadblocks.RemoveAll([](const TObjectPtr<AGTTRoadblock>& Unit){ return !IsValid(Unit.Get()); });
}

void AGTTPoliceDirector::DespawnExcessUnits(int32 DesiredUnits, int32 DesiredVehicles, int32 DesiredRoadblocks)
{
    while (ActivePoliceUnits.Num() > DesiredUnits) { TObjectPtr<APawn> Unit = ActivePoliceUnits.Pop(); if (IsValid(Unit.Get())) Unit->Destroy(); }
    while (ActivePursuitVehicles.Num() > DesiredVehicles) { TObjectPtr<AGTTPolicePursuitVehicle> Unit = ActivePursuitVehicles.Pop(); if (IsValid(Unit.Get())) Unit->Destroy(); }
    while (ActiveRoadblocks.Num() > DesiredRoadblocks) { TObjectPtr<AGTTRoadblock> Unit = ActiveRoadblocks.Pop(); if (IsValid(Unit.Get())) Unit->Destroy(); }
}

int32 AGTTPoliceDirector::SelectInterceptionRoadNode(const APawn* PlayerPawn, bool bPreferFartherNode) const
{
    if (!PlayerPawn || RoadNodes.IsEmpty()) return INDEX_NONE;
    FVector TravelDirection = PlayerPawn->GetVelocity().GetSafeNormal2D();
    if (TravelDirection.IsNearlyZero()) TravelDirection = PlayerPawn->GetActorForwardVector().GetSafeNormal2D();
    if (TravelDirection.IsNearlyZero()) TravelDirection = FVector::ForwardVector;

    const FVector PlayerLocation = PlayerPawn->GetActorLocation();
    const FVector PredictedLocation = PlayerLocation + PlayerPawn->GetVelocity() * InterceptPredictionSeconds;
    const float DesiredLead = bPreferFartherNode ? 2350.0f : 1550.0f;
    int32 BestIndex = INDEX_NONE;
    float BestScore = TNumericLimits<float>::Max();

    for (int32 Index = 0; Index < RoadNodes.Num(); ++Index)
    {
        const FVector ToNode = RoadNodes[Index] - PlayerLocation;
        const float Distance = ToNode.Size2D();
        if (Distance < MinimumInterceptLeadDistance * 0.55f) continue;
        const float AheadDot = FVector::DotProduct(ToNode.GetSafeNormal2D(), TravelDirection);
        const float BehindPenalty = AheadDot < 0.05f ? 2800000.0f * (0.1f - AheadDot) : 0.0f;
        const float PredictionError = FVector::DistSquared2D(RoadNodes[Index], PredictedLocation);
        const float LeadError = FMath::Square(Distance - DesiredLead) * 0.42f;
        const float ReusePenalty = Index == LastInterceptionNodeIndex ? 1800000.0f : 0.0f;
        const float Score = PredictionError + LeadError + BehindPenalty + ReusePenalty;
        if (Score < BestScore) { BestScore = Score; BestIndex = Index; }
    }
    return BestIndex;
}

FTransform AGTTPoliceDirector::MakeRoadNodeTransform(int32 NodeIndex, const APawn* PlayerPawn) const
{
    if (!RoadNodes.IsValidIndex(NodeIndex)) return SelectSpawnTransform(1.0f);
    FVector Location = RoadNodes[NodeIndex]; Location.Z = FMath::Max(Location.Z, 100.0f);
    FVector Facing = PlayerPawn ? (PlayerPawn->GetActorLocation() - Location).GetSafeNormal2D() : FVector::ForwardVector;
    if (Facing.IsNearlyZero()) Facing = FVector::ForwardVector;
    return FTransform(Facing.Rotation(), Location);
}

FTransform AGTTPoliceDirector::SelectPursuitInterceptTransform(int32 WantedLevel) const
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn) return SelectSpawnTransform(1.35f);
    const int32 NodeIndex = SelectInterceptionRoadNode(PlayerPawn, WantedLevel >= 5);
    if (NodeIndex == INDEX_NONE) return SelectSpawnTransform(1.35f);
    FTransform Transform = MakeRoadNodeTransform(NodeIndex, PlayerPawn);
    FVector Location = Transform.GetLocation();
    Location -= Transform.GetRotation().GetForwardVector() * 420.0f;
    Transform.SetLocation(Location);
    return Transform;
}

FTransform AGTTPoliceDirector::SelectSpawnTransform(float DistanceScale) const
{
    if (!SpawnPoints.IsEmpty())
    {
        for (int32 Attempt = 0; Attempt < SpawnPoints.Num(); ++Attempt)
        {
            const int32 Index = FMath::RandRange(0, SpawnPoints.Num() - 1);
            if (const AActor* SpawnPoint = SpawnPoints[Index]) return SpawnPoint->GetActorTransform();
        }
    }
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    const FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : GetActorLocation();
    const float AngleRadians = FMath::FRandRange(0.0f, 2.0f * PI);
    const float Distance = FMath::FRandRange(MinFallbackSpawnDistance, MaxFallbackSpawnDistance) * FMath::Max(0.5f, DistanceScale);
    const FVector Offset(FMath::Cos(AngleRadians) * Distance, FMath::Sin(AngleRadians) * Distance, 140.0f);
    return FTransform(FRotator(0.0f, FMath::RadiansToDegrees(AngleRadians) + 180.0f, 0.0f), PlayerLocation + Offset);
}

FTransform AGTTPoliceDirector::SelectRoadblockTransform(int32 WantedLevel)
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn) return SelectSpawnTransform(1.0f);
    const int32 NodeIndex = SelectInterceptionRoadNode(PlayerPawn, WantedLevel >= 5 || ActiveRoadblocks.Num() > 0);
    if (NodeIndex != INDEX_NONE)
    {
        LastInterceptionNodeIndex = NodeIndex;
        return MakeRoadNodeTransform(NodeIndex, PlayerPawn);
    }

    FVector Direction = PlayerPawn->GetVelocity().GetSafeNormal2D();
    if (Direction.IsNearlyZero()) Direction = PlayerPawn->GetActorForwardVector().GetSafeNormal2D();
    if (Direction.IsNearlyZero()) Direction = FVector::ForwardVector;
    const FVector Right = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal();
    const float Distance = 1350.0f + WantedLevel * 180.0f + FMath::FRandRange(-120.0f, 260.0f);
    FVector Location = PlayerPawn->GetActorLocation() + Direction * Distance + Right * FMath::FRandRange(-180.0f, 180.0f);
    Location.Z = FMath::Max(Location.Z, 80.0f);
    return FTransform(Direction.Rotation(), Location);
}
