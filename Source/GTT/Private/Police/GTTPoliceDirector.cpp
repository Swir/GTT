#include "Police/GTTPoliceDirector.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Wanted/GTTWantedComponent.h"

AGTTPoliceDirector::AGTTPoliceDirector()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGTTPoliceDirector::BeginPlay()
{
    Super::BeginPlay();

    GetWorldTimerManager().SetTimer(
        EvaluationTimer,
        this,
        &AGTTPoliceDirector::EvaluatePoliceResponse,
        EvaluationInterval,
        true,
        0.25f);
}

void AGTTPoliceDirector::EvaluatePoliceResponse()
{
    RemoveInvalidUnits();

    const int32 WantedLevel = GetPlayerWantedLevel();
    const int32 DesiredUnits = FMath::Clamp(WantedLevel * UnitsPerWantedLevel, 0, MaxPoliceUnits);

    if (WantedLevel != LastResponseLevel)
    {
        LastResponseLevel = WantedLevel;
        OnResponseLevelChanged(WantedLevel, DesiredUnits);
    }

    if (WantedLevel > 0 && ActivePoliceUnits.Num() < DesiredUnits)
    {
        SpawnPoliceUnit();
    }
}

void AGTTPoliceDirector::SpawnPoliceUnit()
{
    if (!PolicePawnClass || SpawnPoints.IsEmpty() || !GetWorld())
    {
        return;
    }

    const int32 SpawnIndex = FMath::RandRange(0, SpawnPoints.Num() - 1);
    AActor* SpawnPoint = SpawnPoints[SpawnIndex];
    if (!IsValid(SpawnPoint))
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(
        PolicePawnClass,
        SpawnPoint->GetActorTransform(),
        Params);

    if (SpawnedPawn)
    {
        ActivePoliceUnits.Add(SpawnedPawn);
    }
}

int32 AGTTPoliceDirector::GetPlayerWantedLevel() const
{
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerPawn)
    {
        return 0;
    }

    const UGTTWantedComponent* WantedComponent = PlayerPawn->FindComponentByClass<UGTTWantedComponent>();
    return WantedComponent ? WantedComponent->GetWantedLevel() : 0;
}

void AGTTPoliceDirector::RemoveInvalidUnits()
{
    ActivePoliceUnits.RemoveAll([](const TObjectPtr<APawn>& Unit)
    {
        return !IsValid(Unit);
    });
}
