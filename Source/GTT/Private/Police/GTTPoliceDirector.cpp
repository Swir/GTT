#include "Police/GTTPoliceDirector.h"

#include "Core/GTTGameplayStatics.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Police/GTTPolicePawn.h"
#include "TimerManager.h"

AGTTPoliceDirector::AGTTPoliceDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    PolicePawnClass = AGTTPolicePawn::StaticClass();
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

    DespawnExcessUnits(DesiredUnits);

    if (WantedLevel > 0 && ActivePoliceUnits.Num() < DesiredUnits)
    {
        SpawnPoliceUnit();
    }
}

void AGTTPoliceDirector::SpawnPoliceUnit()
{
    if (!PolicePawnClass || !GetWorld())
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    APawn* SpawnedPawn = GetWorld()->SpawnActor<APawn>(
        PolicePawnClass,
        SelectSpawnTransform(),
        Params);

    if (SpawnedPawn)
    {
        ActivePoliceUnits.Add(SpawnedPawn);
    }
}

int32 AGTTPoliceDirector::GetPlayerWantedLevel() const
{
    return UGTTGameplayStatics::GetPlayerWantedLevel(this, 0);
}

void AGTTPoliceDirector::RemoveInvalidUnits()
{
    ActivePoliceUnits.RemoveAll([](const TObjectPtr<APawn>& Unit)
    {
        return !IsValid(Unit.Get());
    });
}

void AGTTPoliceDirector::DespawnExcessUnits(int32 DesiredUnits)
{
    while (ActivePoliceUnits.Num() > DesiredUnits)
    {
        TObjectPtr<APawn> Unit = ActivePoliceUnits.Pop();
        if (IsValid(Unit.Get()))
        {
            Unit->Destroy();
        }
    }
}

FTransform AGTTPoliceDirector::SelectSpawnTransform() const
{
    if (!SpawnPoints.IsEmpty())
    {
        for (int32 Attempt = 0; Attempt < SpawnPoints.Num(); ++Attempt)
        {
            const int32 Index = FMath::RandRange(0, SpawnPoints.Num() - 1);
            if (const AActor* SpawnPoint = SpawnPoints[Index])
            {
                return SpawnPoint->GetActorTransform();
            }
        }
    }

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    const FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : GetActorLocation();

    const float AngleRadians = FMath::FRandRange(0.0f, 2.0f * PI);
    const float Distance = FMath::FRandRange(MinFallbackSpawnDistance, MaxFallbackSpawnDistance);
    const FVector Offset(FMath::Cos(AngleRadians) * Distance, FMath::Sin(AngleRadians) * Distance, 120.0f);
    const FVector SpawnLocation = PlayerLocation + Offset;

    const float FacingYaw = FMath::RadiansToDegrees(AngleRadians) + 180.0f;
    return FTransform(FRotator(0.0f, FacingYaw, 0.0f), SpawnLocation);
}
