#include "World/GTTVillageEventDirector.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NPC/GTTCitizenPawn.h"
#include "TimerManager.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTVillageEventMarker.h"

AGTTVillageEventDirector::AGTTVillageEventDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    EventMarkerClass = AGTTVillageEventMarker::StaticClass();
}

void AGTTVillageEventDirector::BeginPlay()
{
    Super::BeginPlay();
    DayNightCycle = Cast<AGTTDayNightCycle>(UGameplayStatics::GetActorOfClass(this, AGTTDayNightCycle::StaticClass()));
    EventCountdown = 6.0f;
    GetWorldTimerManager().SetTimer(EvaluationTimer, this, &AGTTVillageEventDirector::EvaluateNightlife, EvaluationInterval, true, 1.0f);
}

bool AGTTVillageEventDirector::IsNightlifeOpen() const
{
    const AGTTDayNightCycle* Cycle = DayNightCycle.Get();
    if (!Cycle)
    {
        return false;
    }
    const float Hour = Cycle->GetTimeOfDayHours();
    return Hour >= 18.5f || Hour < 2.5f;
}

FString AGTTVillageEventDirector::GetNightlifeSummary() const
{
    if (!IsNightlifeOpen())
    {
        return TEXT("NIGHTLIFE CLOSED | opens 18:30");
    }

    if (const AGTTVillageEventMarker* Event = ActiveEvent.Get())
    {
        return FString::Printf(TEXT("VILLAGE NIGHT | COMMUNITY HALL PARTY | EVENT: %s"), *Event->GetEventTitle());
    }
    return TEXT("VILLAGE NIGHT | COMMUNITY HALL PARTY | waiting for the next bad idea...");
}

void AGTTVillageEventDirector::EvaluateNightlife()
{
    if (!DayNightCycle.IsValid())
    {
        DayNightCycle = Cast<AGTTDayNightCycle>(UGameplayStatics::GetActorOfClass(this, AGTTDayNightCycle::StaticClass()));
    }

    CleanupInvalidCrowd();
    const bool bOpen = IsNightlifeOpen();
    EnsurePartyCrowd(bOpen);

    if (!bOpen)
    {
        if (AGTTVillageEventMarker* Event = ActiveEvent.Get())
        {
            Event->Destroy();
        }
        ActiveEvent.Reset();
        EventCountdown = 8.0f;
        return;
    }

    if (ActiveEvent.IsValid())
    {
        return;
    }

    EventCountdown -= EvaluationInterval;
    if (EventCountdown <= 0.0f)
    {
        SpawnNightEvent();
        EventCountdown = FMath::FRandRange(MinSecondsBetweenEvents, MaxSecondsBetweenEvents);
    }
}

void AGTTVillageEventDirector::SpawnNightEvent()
{
    if (!GetWorld() || !EventMarkerClass || ActiveEvent.IsValid())
    {
        return;
    }

    static const FVector EventLocations[] = {
        FVector(2050.0f, 2300.0f, 70.0f),
        FVector(1450.0f, 2250.0f, 70.0f),
        FVector(1100.0f, -2050.0f, 70.0f),
        FVector(5650.0f, -850.0f, 70.0f)
    };

    const int32 LocationIndex = FMath::RandRange(0, UE_ARRAY_COUNT(EventLocations) - 1);
    const int32 EventIndex = FMath::RandRange(0, 3);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AGTTVillageEventMarker* Marker = GetWorld()->SpawnActor<AGTTVillageEventMarker>(
        EventMarkerClass,
        EventLocations[LocationIndex],
        FRotator::ZeroRotator,
        Params);

    if (Marker)
    {
        Marker->ConfigureEvent(static_cast<EGTTVillageNightEventType>(EventIndex));
        ActiveEvent = Marker;
    }
}

void AGTTVillageEventDirector::EnsurePartyCrowd(bool bShouldExist)
{
    if (!GetWorld())
    {
        return;
    }

    if (!bShouldExist)
    {
        for (TObjectPtr<AGTTCitizenPawn>& Citizen : PartyCrowd)
        {
            if (IsValid(Citizen.Get()))
            {
                Citizen->Destroy();
            }
        }
        PartyCrowd.Reset();
        return;
    }

    while (PartyCrowd.Num() < PartyCrowdSize)
    {
        const int32 Index = PartyCrowd.Num();
        const float Angle = (2.0f * PI * Index) / FMath::Max(1, PartyCrowdSize);
        const FVector Offset(FMath::Cos(Angle) * 430.0f, FMath::Sin(Angle) * 430.0f, 0.0f);
        const FVector SpawnLocation = FVector(2400.0f, 2700.0f, 120.0f) + Offset;
        if (AGTTCitizenPawn* Citizen = GetWorld()->SpawnActor<AGTTCitizenPawn>(SpawnLocation, FRotator::ZeroRotator))
        {
            PartyCrowd.Add(Citizen);
        }
        else
        {
            break;
        }
    }
}

void AGTTVillageEventDirector::CleanupInvalidCrowd()
{
    PartyCrowd.RemoveAll([](const TObjectPtr<AGTTCitizenPawn>& Citizen)
    {
        return !IsValid(Citizen.Get());
    });
}
