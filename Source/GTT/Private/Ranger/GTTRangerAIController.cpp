#include "Ranger/GTTRangerAIController.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Vehicles/GTTFieldmasterNativePawn.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTRuralEconomySubsystem.h"

AGTTRangerAIController::AGTTRangerAIController()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGTTRangerAIController::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimer(PursuitTimer, this, &AGTTRangerAIController::UpdatePursuit, RepathInterval, true, 0.2f);
}

void AGTTRangerAIController::ResetRoadStopState()
{
    bRoadStopActive = false;
    bRoadStopEvasionEscalated = false;
    bSearchHoldMessageShown = false;
    RoadStopTimeRemaining = 0.0f;
    ComplianceHoldElapsed = 0.0f;
}

bool AGTTRangerAIController::IsVehicleTarget(APawn* Target) const
{
    return Target &&
        (Cast<AGTTVehicleBase>(Target) || Cast<AGTTFieldmasterNativePawn>(Target) || Cast<AGTTRoadVehicleNativePawn>(Target));
}

float AGTTRangerAIController::GetTargetSpeedKmh(APawn* Target) const
{
    if (!Target)
    {
        return 0.0f;
    }

    if (const AGTTVehicleBase* LegacyVehicle = Cast<AGTTVehicleBase>(Target))
    {
        return LegacyVehicle->GetSpeedKmh();
    }

    return Target->GetVelocity().Size2D() * 0.036f;
}

void AGTTRangerAIController::PushRangerMessage(APawn* Target, const FString& Message, float Duration) const
{
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Target))
    {
        Economy->PushMessage(Message, Duration);
    }
}

bool AGTTRangerAIController::TryResolveRoadsideSearch(APawn* Target)
{
    AGTTGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGTTGameMode>() : nullptr;
    if (!GameMode || !Target || !GameMode->TryRangerCitation(Target))
    {
        return false;
    }

    int32 ConfiscatedValue = 0;
    int32 ConfiscatedUnits = 0;
    if (UGTTRuralEconomySubsystem* RuralEconomy = GetWorld()->GetSubsystem<UGTTRuralEconomySubsystem>())
    {
        ConfiscatedUnits = RuralEconomy->ConfiscateContraband(ConfiscatedValue);
    }

    if (ConfiscatedUnits > 0)
    {
        PushRangerMessage(
            Target,
            FString::Printf(TEXT("WARDEN SEIZURE: roadside search found %d contraband unit%s | fence value $%d lost."),
                ConfiscatedUnits, ConfiscatedUnits == 1 ? TEXT("") : TEXT("s"), ConfiscatedValue),
            5.5f);
    }
    else
    {
        PushRangerMessage(Target, TEXT("WARDEN SEARCH COMPLETE: no rural contraband found. Citation resolved."), 4.5f);
    }

    bRoadStopActive = false;
    ComplianceHoldElapsed = 0.0f;
    bSearchHoldMessageShown = false;
    RoadStopTimeRemaining = 0.0f;
    return true;
}

void AGTTRangerAIController::EscalateRoadStopEvasion(APawn* Target)
{
    if (!Target || bRoadStopEvasionEscalated)
    {
        return;
    }

    if (UGTTWantedComponent* Wanted = UGTTGameplayStatics::FindWantedComponentForPawn(Target))
    {
        Wanted->AddHeat(RoadStopEvasionWantedHeat);
        PushRangerMessage(
            Target,
            FString::Printf(TEXT("FLED WARDEN STOP: +%.0f police heat | wanted %d/5. Rangers remain active."),
                RoadStopEvasionWantedHeat, Wanted->GetWantedLevel()),
            5.5f);
    }
    else
    {
        PushRangerMessage(Target, TEXT("FLED WARDEN STOP: county police escalation requested."), 5.0f);
    }

    bRoadStopEvasionEscalated = true;
    bRoadStopActive = false;
    bSearchHoldMessageShown = false;
    RoadStopTimeRemaining = 0.0f;
    ComplianceHoldElapsed = 0.0f;
}

void AGTTRangerAIController::UpdatePursuit()
{
    AGTTGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AGTTGameMode>() : nullptr;
    APawn* Target = UGameplayStatics::GetPlayerPawn(this, 0);
    APawn* RangerPawn = GetPawn();
    const int32 AlertLevel = GameMode ? GameMode->GetWildlifeAlertLevel() : 0;
    if (!GameMode || !Target || !RangerPawn || AlertLevel <= 0)
    {
        ResetRoadStopState();
        StopMovement();
        return;
    }

    if (ACharacter* RangerCharacter = Cast<ACharacter>(RangerPawn))
    {
        RangerCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseChaseSpeed + AlertLevel * SpeedPerAlertLevel;
    }

    MoveToActor(Target, AcceptanceRadius, true, true, true, nullptr, true);

    const float DistanceSquared = FVector::DistSquared2D(RangerPawn->GetActorLocation(), Target->GetActorLocation());
    const bool bVehicleTarget = IsVehicleTarget(Target);

    if (bVehicleTarget && AlertLevel >= RoadStopAlertLevel && !bRoadStopEvasionEscalated)
    {
        if (!bRoadStopActive && DistanceSquared <= FMath::Square(RoadStopOrderRadius))
        {
            bRoadStopActive = true;
            RoadStopTimeRemaining = RoadStopGraceSeconds;
            ComplianceHoldElapsed = 0.0f;
            bSearchHoldMessageShown = false;
            PushRangerMessage(
                Target,
                FString::Printf(TEXT("WARDEN ROAD STOP: pull over below %.1f km/h and hold still. %.0fs compliance window."),
                    RoadStopComplianceSpeedKmh, RoadStopGraceSeconds),
                5.0f);
        }

        if (bRoadStopActive)
        {
            const float SpeedKmh = GetTargetSpeedKmh(Target);
            const bool bInsideSearchRadius = DistanceSquared <= FMath::Square(RoadStopSearchRadius);
            const bool bCompliantSpeed = SpeedKmh <= RoadStopComplianceSpeedKmh;

            RoadStopTimeRemaining = FMath::Max(0.0f, RoadStopTimeRemaining - RepathInterval);

            if (bCompliantSpeed && bInsideSearchRadius)
            {
                ComplianceHoldElapsed += RepathInterval;
                if (!bSearchHoldMessageShown)
                {
                    bSearchHoldMessageShown = true;
                    PushRangerMessage(
                        Target,
                        FString::Printf(TEXT("WARDEN SEARCH: hold position for %.1fs. Fish and rural contraband are subject to seizure."),
                            RoadStopComplianceHoldSeconds),
                        4.0f);
                }

                if (ComplianceHoldElapsed >= RoadStopComplianceHoldSeconds && TryResolveRoadsideSearch(Target))
                {
                    StopMovement();
                    return;
                }
            }
            else
            {
                ComplianceHoldElapsed = 0.0f;
                bSearchHoldMessageShown = false;
            }

            if (RoadStopTimeRemaining <= 0.0f && SpeedKmh >= RoadStopFleeSpeedKmh)
            {
                EscalateRoadStopEvasion(Target);
            }

            // While a vehicle stop is active, do not allow the legacy proximity citation
            // to resolve the incident through a moving vehicle. The player must stop, exit,
            // or explicitly flee and accept police escalation.
            return;
        }
    }

    if (DistanceSquared <= FMath::Square(CitationRadius))
    {
        if (TryResolveRoadsideSearch(Target))
        {
            StopMovement();
        }
    }
}
