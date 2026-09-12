#include "Police/GTTPoliceDirector.h"

#include "Core/GTTGameplayStatics.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Police/GTTPolicePawn.h"
#include "Police/GTTPolicePursuitVehicle.h"
#include "TimerManager.h"

AGTTPoliceDirector::AGTTPoliceDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    PolicePawnClass = AGTTPolicePawn::StaticClass();
    PursuitVehicleClass = AGTTPolicePursuitVehicle::StaticClass();
}

void AGTTPoliceDirector::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimer(EvaluationTimer, this, &AGTTPoliceDirector::EvaluatePoliceResponse, EvaluationInterval, true, 0.25f);
}

void AGTTPoliceDirector::EvaluatePoliceResponse()
{
    RemoveInvalidUnits();

    const int32 WantedLevel = GetPlayerWantedLevel();
    const int32 DesiredFootUnits = WantedLevel > 0
        ? FMath::Clamp(WantedLevel * UnitsPerWantedLevel - FMath::Max(0, WantedLevel - 2), 0, MaxPoliceUnits)
        : 0;
    const int32 DesiredVehicles = WantedLevel >= VehicleEscalationWantedLevel
        ? FMath::Clamp(WantedLevel - VehicleEscalationWantedLevel + 1, 1, MaxPursuitVehicles)
        : 0;

    if (WantedLevel != LastResponseLevel)
    {
        LastResponseLevel = WantedLevel;
        OnResponseLevelChanged(WantedLevel, DesiredFootUnits + DesiredVehicles);
    }

    DespawnExcessUnits(DesiredFootUnits, DesiredVehicles);

    if (WantedLevel > 0 && ActivePoliceUnits.Num() < DesiredFootUnits)
    {
        SpawnPoliceUnit();
    }
    if (DesiredVehicles > 0 && ActivePursuitVehicles.Num() < DesiredVehicles)
    {
        SpawnPursuitVehicle(WantedLevel);
    }
}

void AGTTPoliceDirector::SpawnPoliceUnit()
{
    if (!PolicePawnClass || !GetWorld()) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(PolicePawnClass, SelectSpawnTransform(), Params);
    if (SpawnedPawn) ActivePoliceUnits.Add(SpawnedPawn);
}

void AGTTPoliceDirector::SpawnPursuitVehicle(int32 WantedLevel)
{
    if (!PursuitVehicleClass || !GetWorld()) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AGTTPolicePursuitVehicle* Vehicle = GetWorld()->SpawnActor<AGTTPolicePursuitVehicle>(
        PursuitVehicleClass,
        SelectSpawnTransform(1.35f),
        Params);
    if (Vehicle)
    {
        Vehicle->SetResponseTier(FMath::Clamp(WantedLevel - 2, 1, 3));
        ActivePursuitVehicles.Add(Vehicle);
    }
}

int32 AGTTPoliceDirector::GetPlayerWantedLevel() const
{
    return UGTTGameplayStatics::GetPlayerWantedLevel(this, 0);
}

void AGTTPoliceDirector::RemoveInvalidUnits()
{
    ActivePoliceUnits.RemoveAll([](const TObjectPtr<APawn>& Unit){ return !IsValid(Unit.Get()); });
    ActivePursuitVehicles.RemoveAll([](const TObjectPtr<AGTTPolicePursuitVehicle>& Unit){ return !IsValid(Unit.Get()); });
}

void AGTTPoliceDirector::DespawnExcessUnits(int32 DesiredUnits, int32 DesiredVehicles)
{
    while (ActivePoliceUnits.Num() > DesiredUnits)
    {
        TObjectPtr<APawn> Unit = ActivePoliceUnits.Pop();
        if (IsValid(Unit.Get())) Unit->Destroy();
    }
    while (ActivePursuitVehicles.Num() > DesiredVehicles)
    {
        TObjectPtr<AGTTPolicePursuitVehicle> Unit = ActivePursuitVehicles.Pop();
        if (IsValid(Unit.Get())) Unit->Destroy();
    }
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
    const float FacingYaw = FMath::RadiansToDegrees(AngleRadians) + 180.0f;
    return FTransform(FRotator(0.0f, FacingYaw, 0.0f), PlayerLocation + Offset);
}
