#include "Ranger/GTTRangerAIController.h"

#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Ranger/GTTRangerRoadStopSubsystem.h"
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

void AGTTRangerAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld())
    {
        if (UGTTRangerRoadStopSubsystem* RoadStop = GetWorld()->GetSubsystem<UGTTRangerRoadStopSubsystem>())
        {
            RoadStop->EndStop(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AGTTRangerAIController::ResetRoadStopState()
{
    if (GetWorld())
    {
        if (UGTTRangerRoadStopSubsystem* RoadStop = GetWorld()->GetSubsystem<UGTTRangerRoadStopSubsystem>())
        {
            RoadStop->EndStop(this);
        }
    }

    bRoadStopActive = false;
    bRoadStopEvasionEscalated = false;
    bSearchHoldMessageShown = false;
    bComplianceReminderShown = false;
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

    if (UGTTRangerRoadStopSubsystem* RoadStop = GetWorld()->GetSubsystem<UGTTRangerRoadStopSubsystem>())
    {
        RoadStop->EndStop(this);
    }
    bRoadStopActive = false;
    ComplianceHoldElapsed = 0.0f;
    bSearchHoldMessageShown = false;
    bComplianceReminderShown = false;
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

    if (UGTTRangerRoadStopSubsystem* RoadStop = GetWorld()->GetSubsystem<UGTTRangerRoadStopSubsystem>())
    {
        RoadStop->MarkFlee(this, 4.5f);
    }
    bRoadStopEvasionEscalated = true;
    bRoadStopActive = false;
    bSearchHoldMessageShown = false;
    bComplianceReminderShown = false;
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

    UGTTRangerRoadStopSubsystem* RoadStopSubsystem = GetWorld()->GetSubsystem<UGTTRangerRoadStopSubsystem>();
    const float DistanceSquared = FVector::DistSquared2D(RangerPawn->GetActorLocation(), Target->GetActorLocation());
    const bool bVehicleTarget = IsVehicleTarget(Target);

    if (bRoadStopActive && (!bVehicleTarget || AlertLevel < RoadStopAlertLevel))
    {
        ResetRoadStopState();
    }

    // A driver who has already fled this incident remains a pursuit target; do not let
    // the legacy proximity citation silently erase the police escalation on the next tick.
    if (bRoadStopEvasionEscalated)
    {
        MoveToActor(Target, AcceptanceRadius, true, true, true, nullptr, true);
        return;
    }

    MoveToActor(Target, AcceptanceRadius, true, true, true, nullptr, true);

    if (bVehicleTarget && AlertLevel >= RoadStopAlertLevel && !bRoadStopEvasionEscalated)
    {
        if (!bRoadStopActive && DistanceSquared <= FMath::Square(RoadStopOrderRadius))
        {
            const bool bAcquiredStop = !RoadStopSubsystem || RoadStopSubsystem->BeginStop(
                this, Target, RangerPawn->GetActorLocation(), RoadStopGraceSeconds);
            if (bAcquiredStop)
            {
                bRoadStopActive = true;
                RoadStopTimeRemaining = RoadStopGraceSeconds;
                ComplianceHoldElapsed = 0.0f;
                bSearchHoldMessageShown = false;
                bComplianceReminderShown = false;
                PushRangerMessage(
                    Target,
                    FString::Printf(TEXT("WARDEN ROAD STOP: pull onto the shoulder below %.1f km/h and hold still. %.0fs compliance window."),
                        RoadStopComplianceSpeedKmh, RoadStopGraceSeconds),
                    5.0f);
            }
        }

        // Only one ranger owns the roadside contact. Reinforcement stages behind the stop
        // instead of standing in the traffic lane or issuing a duplicate citation.
        if (!bRoadStopActive && RoadStopSubsystem &&
            RoadStopSubsystem->IsStopForTarget(Target) && !RoadStopSubsystem->IsOwnedBy(this))
        {
            MoveToLocation(
                RoadStopSubsystem->GetRangerSupportPoint(RoadStopShoulderOffset + 70.0f, RoadStopRearOffset),
                RoadStopStagingAcceptanceRadius + 45.0f,
                true, true, true, false, nullptr, true);
            return;
        }

        if (bRoadStopActive)
        {
            if (RoadStopSubsystem)
            {
                MoveToLocation(
                    RoadStopSubsystem->GetRangerStagingPoint(RoadStopShoulderOffset, RoadStopRearOffset),
                    RoadStopStagingAcceptanceRadius,
                    true, true, true, false, nullptr, true);
            }

            const float SpeedKmh = GetTargetSpeedKmh(Target);
            const bool bInsideSearchRadius = DistanceSquared <= FMath::Square(RoadStopSearchRadius);
            const bool bCompliantSpeed = SpeedKmh <= RoadStopComplianceSpeedKmh;

            RoadStopTimeRemaining = FMath::Max(0.0f, RoadStopTimeRemaining - RepathInterval);

            if (!bComplianceReminderShown && RoadStopTimeRemaining <= 2.25f && !bCompliantSpeed)
            {
                bComplianceReminderShown = true;
                PushRangerMessage(Target, TEXT("WARDEN STOP: COMPLY NOW - slow down and hold on the shoulder, or the stop becomes an evasion."), 3.0f);
            }

            const bool bSearching = bCompliantSpeed && bInsideSearchRadius;
            if (bSearching)
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
            }
            else
            {
                ComplianceHoldElapsed = 0.0f;
                bSearchHoldMessageShown = false;
            }

            if (RoadStopSubsystem)
            {
                RoadStopSubsystem->UpdateStop(
                    this, Target, RoadStopTimeRemaining, ComplianceHoldElapsed,
                    RoadStopComplianceHoldSeconds, SpeedKmh, bSearching);
            }

            if (bSearching && ComplianceHoldElapsed >= RoadStopComplianceHoldSeconds && TryResolveRoadsideSearch(Target))
            {
                StopMovement();
                return;
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
