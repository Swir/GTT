#include "Ranger/GTTRangerDirector.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Ranger/GTTRangerPatrolVehicle.h"
#include "Ranger/GTTRangerPawn.h"
#include "Ranger/GTTRangerPullOverMarker.h"
#include "TimerManager.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"

AGTTRangerDirector::AGTTRangerDirector()
{
    PrimaryActorTick.bCanEverTick = false;
    RangerClass = AGTTRangerPawn::StaticClass();
    PatrolVehicleClass = AGTTRangerPatrolVehicle::StaticClass();
    PullOverMarkerClass = AGTTRangerPullOverMarker::StaticClass();
}

void AGTTRangerDirector::BeginPlay()
{
    Super::BeginPlay();

    // Keep lightweight presentation actors resident and hidden. The road-stop
    // subsystem is still the only incident authority; these actors merely render
    // its stable scene transform without entering the garage/save/economy stacks.
    if (GetWorld())
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        if (PatrolVehicleClass)
        {
            PatrolVehicle = GetWorld()->SpawnActor<AGTTRangerPatrolVehicle>(
                PatrolVehicleClass, GetActorLocation(), GetActorRotation(), SpawnParams);
        }
        if (PullOverMarkerClass)
        {
            PullOverMarker = GetWorld()->SpawnActor<AGTTRangerPullOverMarker>(
                PullOverMarkerClass, GetActorLocation(), GetActorRotation(), SpawnParams);
        }
    }

    GetWorldTimerManager().SetTimer(ResponseTimer, this, &AGTTRangerDirector::UpdateResponse, ResponseInterval, true, 0.35f);
}

void AGTTRangerDirector::CleanupInvalidRangers()
{
    ActiveRangers.RemoveAll([](const TWeakObjectPtr<AGTTRangerPawn>& Ranger)
    {
        return !Ranger.IsValid();
    });
}

void AGTTRangerDirector::ApplyPoliceHandoff(APawn* PlayerPawn, int32 AlertLevel)
{
    if (!PlayerPawn || bPoliceHandoffIssued || AlertLevel < PoliceHandoffAlertLevel)
    {
        return;
    }

    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(PlayerPawn))
    {
        Wanted->AddHeat(PoliceHandoffHeat);
        bPoliceHandoffIssued = true;

        if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
        {
            Economy->PushMessage(
                FString::Printf(TEXT("WARDEN RADIO HANDOFF: county police notified | wildlife alert %d/3 | wanted %d/5"),
                    AlertLevel, Wanted->GetWantedLevel()),
                5.5f);
        }
    }
}

void AGTTRangerDirector::UpdateResponse()
{
    AGTTGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGTTGameMode>() : nullptr;
    if (!GameMode || !GetWorld())
    {
        return;
    }

    CleanupInvalidRangers();
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    const int32 AlertLevel = GameMode->GetWildlifeAlertLevel();
    const AGTTDayNightCycle* DayNight = GameMode->GetDayNightCycle();
    const bool bNight = DayNight && DayNight->IsNight();

    if (AlertLevel <= 0)
    {
        bPoliceHandoffIssued = false;
        bNightReinforcementActive = false;
        EnforcementTier = 0;
    }
    else
    {
        bNightReinforcementActive = bNight && AlertLevel >= NightReinforcementAlertLevel;
        EnforcementTier = AlertLevel + (bNightReinforcementActive ? 1 : 0) + (bPoliceHandoffIssued ? 1 : 0);
        ApplyPoliceHandoff(PlayerPawn, AlertLevel);
        EnforcementTier = AlertLevel + (bNightReinforcementActive ? 1 : 0) + (bPoliceHandoffIssued ? 1 : 0);
    }

    int32 DesiredRangers = 0;
    if (AlertLevel > 0)
    {
        DesiredRangers = AlertLevel >= 3 ? 2 : 1;
        if (bNightReinforcementActive)
        {
            DesiredRangers = FMath::Max(DesiredRangers, 2);
        }
    }

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
        const int32 Index = FMath::Clamp(ActiveRangers.Num(), 0, 1);
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