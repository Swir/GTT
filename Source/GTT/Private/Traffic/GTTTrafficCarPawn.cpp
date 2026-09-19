#include "Traffic/GTTTrafficCarPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Ranger/GTTRangerRoadStopSubsystem.h"
#include "World/GTTWorldPerformanceSubsystem.h"
#include "GTT.h"

AGTTTrafficCarPawn::AGTTTrafficCarPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    VehicleDisplayName = NSLOCTEXT("GTT", "TrafficCarName", "Village traffic car");
    PersistentVehicleId = NAME_None;
    bIllegalToTake = false;
    TheftHeat = 0.0f;
    FuelCapacityLiters = 999.0f;
    StartingFuelLiters = 999.0f;
    IdleFuelBurnPerSecond = 0.0f;
    FullThrottleFuelBurnPerSecond = 0.0f;

    HornText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HornText"));
    HornText->SetupAttachment(VehicleMesh);
    HornText->SetText(NSLOCTEXT("GTT", "TrafficHorn", "BEEP!"));
    HornText->SetWorldSize(48.0f);
    HornText->SetHorizontalAlignment(EHTA_Center);
    HornText->SetTextRenderColor(FColor(255, 220, 40));
    HornText->SetRelativeLocation(FVector(0.0f, 0.0f, 185.0f));
    HornText->SetVisibility(false, true);
}

void AGTTTrafficCarPawn::InitializeRoute(const TArray<FVector>& InRoute, int32 StartIndex)
{
    RoutePoints = InRoute;
    LastObservedConditionPercent = GetConditionPercent();
    if (RoutePoints.Num() > 0)
    {
        CurrentRoutePoint = FMath::Abs(StartIndex) % RoutePoints.Num();
        SetActorLocation(RoutePoints[CurrentRoutePoint] + FVector(0.0f, 0.0f, 90.0f), false, nullptr, ETeleportType::TeleportPhysics);
        CurrentRoutePoint = (CurrentRoutePoint + 1) % RoutePoints.Num();
    }
}

void AGTTTrafficCarPawn::RegisterCollisionIncident(float ImpactSpeedKmh, FVector SourceLocation)
{
    const bool bWasDisabled = bIncidentDisabled;
    const float Severity = FMath::Clamp((ImpactSpeedKmh - 12.0f) / 58.0f, 0.0f, 1.0f);
    LastIncidentSeverity = FMath::Max(LastIncidentSeverity, Severity);
    IncidentStopRemaining = FMath::Max(IncidentStopRemaining, FMath::Lerp(CollisionStopSeconds * 0.65f, CollisionStopSeconds * 1.65f, Severity));
    IncidentLimpRemaining = FMath::Max(IncidentLimpRemaining, FMath::Lerp(IncidentLimpSeconds * 0.55f, IncidentLimpSeconds * 1.25f, Severity));

    const FVector Away = (GetActorLocation() - SourceLocation).GetSafeNormal2D();
    IncidentSteerBias = FMath::Clamp(FVector::DotProduct(GetActorRightVector().GetSafeNormal2D(), Away), -1.0f, 1.0f);
    bIncidentDisabled = GetConditionPercent() <= DisableConditionThreshold;
    if (bIncidentDisabled && !bWasDisabled)
    {
        bRoadsideAssistanceCompletedForIncident = false;
        RoadsideAssistanceRemaining = 0.0f;
        RoadsideHelper.Reset();
        bRoadsideAssistanceActive = false;
        bRoadsideResponderSceneAuthority = false;
    }

    if (HornText)
    {
        HornText->SetText(bIncidentDisabled
            ? NSLOCTEXT("GTT", "TrafficDisabled", "HAZARD")
            : NSLOCTEXT("GTT", "TrafficCrashHazard", "CAUTION"));
        HornVisualRemaining = FMath::Max(HornVisualRemaining, IncidentStopRemaining);
    }

    if (GetWorld() && Severity >= 0.15f)
    {
        const float RadiusSq = FMath::Square(IncidentAwarenessRadius);
        for (TActorIterator<AGTTTrafficCarPawn> It(GetWorld()); It; ++It)
        {
            AGTTTrafficCarPawn* OtherTraffic = *It;
            if (!OtherTraffic || OtherTraffic == this) continue;
            if (FVector::DistSquared2D(OtherTraffic->GetActorLocation(), GetActorLocation()) <= RadiusSq)
            {
                OtherTraffic->ReactToNearbyIncident(GetActorLocation(), Severity);
            }
        }
    }

    UE_LOG(LogGTT, Warning,
        TEXT("TRAFFIC_CRASH_RESPONSE car=%s impact_speed_kmh=%.1f severity=%.2f stop_s=%.1f limp_s=%.1f disabled=%s condition=%.2f"),
        *GetName(), ImpactSpeedKmh, Severity, IncidentStopRemaining, IncidentLimpRemaining,
        bIncidentDisabled ? TEXT("YES") : TEXT("NO"), GetConditionPercent());
}

void AGTTTrafficCarPawn::ReactToNearbyIncident(FVector SourceLocation, float Severity)
{
    if (bIncidentDisabled)
    {
        return;
    }

    const float ClampedSeverity = FMath::Clamp(Severity, 0.0f, 1.0f);
    IncidentStopRemaining = FMath::Max(IncidentStopRemaining, NearbyIncidentStopSeconds * FMath::Lerp(0.55f, 1.35f, ClampedSeverity));
    const FVector Away = (GetActorLocation() - SourceLocation).GetSafeNormal2D();
    IncidentSteerBias = FMath::Clamp(FVector::DotProduct(GetActorRightVector().GetSafeNormal2D(), Away), -1.0f, 1.0f);
    HornCooldownRemaining = FMath::Max(HornCooldownRemaining, 0.75f);

    if (HornText)
    {
        HornText->SetText(NSLOCTEXT("GTT", "TrafficIncidentWarning", "SLOW"));
        HornVisualRemaining = FMath::Max(HornVisualRemaining, 1.1f);
    }
}

bool AGTTTrafficCarPawn::BeginRoadsideAssistance(AActor* Helper)
{
    if (!bIncidentDisabled || bRoadsideAssistanceCompletedForIncident || bRoadsideAssistanceActive || !IsValid(Helper))
    {
        return false;
    }

    UGTTPlayerEconomyComponent* Economy = Helper->FindComponentByClass<UGTTPlayerEconomyComponent>();
    if (!Economy)
    {
        return false;
    }

    if (bRoadsideResponderSceneAuthority)
    {
        Economy->PushMessage(TEXT("County road service has scene authority. This incident no longer offers a player roadside payout."), 3.5f);
        return false;
    }

    if (FVector::DistSquared2D(Helper->GetActorLocation(), GetActorLocation()) > FMath::Square(RoadsideAssistanceMaxDistance))
    {
        Economy->PushMessage(TEXT("Move closer to the disabled traffic vehicle."), 3.0f);
        return false;
    }

    RoadsideHelper = Helper;
    RoadsideAssistanceRemaining = RoadsideAssistanceDurationSeconds;
    bRoadsideAssistanceActive = true;
    Economy->PushMessage(TEXT("Roadside assist started - stay beside the vehicle."), 3.5f);
    if (HornText)
    {
        HornText->SetText(NSLOCTEXT("GTT", "TrafficAssist", "ASSIST"));
        HornVisualRemaining = FMath::Max(HornVisualRemaining, RoadsideAssistanceDurationSeconds);
    }
    UE_LOG(LogGTT, Log, TEXT("TRAFFIC_ROADSIDE_ASSIST_START car=%s helper=%s severity=%.2f"), *GetName(), *Helper->GetName(), LastIncidentSeverity);
    return true;
}

void AGTTTrafficCarPawn::CancelRoadsideAssistance(const TCHAR* Reason)
{
    if (!bRoadsideAssistanceActive) return;
    if (AActor* Helper = RoadsideHelper.Get())
    {
        if (UGTTPlayerEconomyComponent* Economy = Helper->FindComponentByClass<UGTTPlayerEconomyComponent>())
        {
            Economy->PushMessage(TEXT("Roadside assist paused - return to the vehicle to restart."), 3.0f);
        }
    }
    UE_LOG(LogGTT, Log, TEXT("TRAFFIC_ROADSIDE_ASSIST_CANCEL car=%s reason=%s"), *GetName(), Reason);
    bRoadsideAssistanceActive = false;
    RoadsideAssistanceRemaining = 0.0f;
    RoadsideHelper.Reset();
    if (HornText) HornText->SetText(NSLOCTEXT("GTT", "TrafficDisabled", "HAZARD"));
}

void AGTTTrafficCarPawn::CompleteRoadsideAssistance()
{
    AActor* Helper = RoadsideHelper.Get();
    UGTTPlayerEconomyComponent* Economy = Helper ? Helper->FindComponentByClass<UGTTPlayerEconomyComponent>() : nullptr;
    if (!Helper || !Economy || !bIncidentDisabled)
    {
        CancelRoadsideAssistance(TEXT("invalid-helper-or-incident"));
        return;
    }

    RepairVehicle(MaxCondition * RoadsideRepairFraction);
    bIncidentDisabled = GetConditionPercent() <= DisableConditionThreshold;
    if (bIncidentDisabled)
    {
        CancelRoadsideAssistance(TEXT("repair-insufficient"));
        return;
    }

    const int32 Payout = RoadsideBasePayout + FMath::RoundToInt(static_cast<float>(RoadsideSeverityBonus) * FMath::Clamp(LastIncidentSeverity, 0.0f, 1.0f));
    Economy->AddCash(Payout, TEXT("Roadside traffic assistance"));
    Economy->PushMessage(FString::Printf(TEXT("Roadside assist complete: +$%d"), Payout), 4.0f);

    IncidentStopRemaining = 0.0f;
    IncidentLimpRemaining = FMath::Max(IncidentLimpRemaining, RoadsidePostAssistLimpSeconds);
    LastObservedConditionPercent = GetConditionPercent();
    bRoadsideAssistanceActive = false;
    bRoadsideAssistanceCompletedForIncident = true;
    RoadsideAssistanceRemaining = 0.0f;
    RoadsideHelper.Reset();

    if (HornText)
    {
        HornText->SetText(NSLOCTEXT("GTT", "TrafficAssistThanks", "THANKS"));
        HornVisualRemaining = 2.5f;
    }
    UE_LOG(LogGTT, Log, TEXT("TRAFFIC_ROADSIDE_ASSIST_COMPLETE car=%s payout=%d condition=%.2f limp_s=%.1f"),
        *GetName(), Payout, GetConditionPercent(), IncidentLimpRemaining);
}

void AGTTTrafficCarPawn::SetRoadsideResponderSceneAuthority(bool bActive)
{
    bRoadsideResponderSceneAuthority = bActive && bIncidentDisabled;
    if (bRoadsideResponderSceneAuthority && bRoadsideAssistanceActive)
    {
        CancelRoadsideAssistance(TEXT("county-road-service-scene-handoff"));
    }

    if (HornText && bRoadsideResponderSceneAuthority)
    {
        HornText->SetText(NSLOCTEXT("GTT", "TrafficRoadService", "SERVICE"));
        HornVisualRemaining = FMath::Max(HornVisualRemaining, 0.5f);
    }
}

bool AGTTTrafficCarPawn::CompleteRoadsideResponderRecovery()
{
    if (!bIncidentDisabled || !bRoadsideResponderSceneAuthority || bRoadsideAssistanceActive)
    {
        return false;
    }

    RepairVehicle(MaxCondition * ResponderRecoveryFraction);
    bIncidentDisabled = GetConditionPercent() <= DisableConditionThreshold;
    if (bIncidentDisabled)
    {
        UE_LOG(LogGTT, Warning, TEXT("TRAFFIC_RESPONDER_RECOVERY_RETRY car=%s condition=%.2f"), *GetName(), GetConditionPercent());
        return false;
    }

    IncidentStopRemaining = 0.0f;
    IncidentLimpRemaining = FMath::Max(IncidentLimpRemaining, ResponderPostRecoveryLimpSeconds);
    LastObservedConditionPercent = GetConditionPercent();
    bRoadsideResponderSceneAuthority = false;
    RoadsideAssistanceRemaining = 0.0f;
    RoadsideHelper.Reset();
    bRoadsideAssistanceActive = false;

    if (HornText)
    {
        HornText->SetText(NSLOCTEXT("GTT", "TrafficRoadServiceComplete", "RECOVERED"));
        HornVisualRemaining = 2.5f;
    }

    UE_LOG(LogGTT, Log,
        TEXT("TRAFFIC_RESPONDER_RECOVERY_COMPLETE car=%s condition=%.2f limp_s=%.1f payout=NONE"),
        *GetName(), GetConditionPercent(), IncidentLimpRemaining);
    return true;
}

void AGTTTrafficCarPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const float CurrentConditionPercent = GetConditionPercent();
    const float ConditionDrop = LastObservedConditionPercent - CurrentConditionPercent;
    if (ConditionDrop >= 0.04f)
    {
        const float EstimatedImpactSpeedKmh = FMath::Clamp(12.0f + ConditionDrop * 145.0f, 12.0f, 80.0f);
        RegisterCollisionIncident(EstimatedImpactSpeedKmh, GetActorLocation() - GetVelocity().GetSafeNormal2D() * 120.0f);
    }
    LastObservedConditionPercent = CurrentConditionPercent;

    if (bRoadsideAssistanceActive)
    {
        AActor* Helper = RoadsideHelper.Get();
        if (!bIncidentDisabled || !Helper || FVector::DistSquared2D(Helper->GetActorLocation(), GetActorLocation()) > FMath::Square(RoadsideAssistanceMaxDistance))
        {
            CancelRoadsideAssistance(TEXT("helper-left-scene"));
        }
        else
        {
            RoadsideAssistanceRemaining = FMath::Max(0.0f, RoadsideAssistanceRemaining - DeltaSeconds);
            if (HornText)
            {
                const int32 Percent = FMath::Clamp(FMath::RoundToInt((1.0f - RoadsideAssistanceRemaining / FMath::Max(0.01f, RoadsideAssistanceDurationSeconds)) * 100.0f), 0, 100);
                HornText->SetText(FText::FromString(FString::Printf(TEXT("ASSIST %d%%"), Percent)));
                HornVisualRemaining = FMath::Max(HornVisualRemaining, 0.25f);
            }
            if (RoadsideAssistanceRemaining <= 0.0f)
            {
                CompleteRoadsideAssistance();
            }
        }
    }

    float RangerStopSpeedScale = 1.0f;
    bool bRangerStopHold = false;
    bYieldingForRangerStop = false;
    bHoldingForRangerStop = false;
    if (GetWorld())
    {
        if (const UGTTRangerRoadStopSubsystem* RoadStop = GetWorld()->GetSubsystem<UGTTRangerRoadStopSubsystem>())
        {
            bYieldingForRangerStop = RoadStop->GetTrafficResponse(
                GetActorLocation(), GetActorForwardVector(), RangerStopSpeedScale, bRangerStopHold);
            bHoldingForRangerStop = bYieldingForRangerStop && bRangerStopHold;
        }
    }

    bool bAllowExpensiveQueries = true;
    if (GetWorld())
    {
        if (UGTTWorldPerformanceSubsystem* Performance = GetWorld()->GetSubsystem<UGTTWorldPerformanceSubsystem>())
        {
            const bool bForceCritical = IsOccupied() || IncidentStopRemaining > 0.0f || bIncidentDisabled || bRoadsideAssistanceActive || bRoadsideResponderSceneAuthority || bYieldingForRangerStop;
            const float BudgetInterval = Performance->GetRecommendedTickInterval(this, bForceCritical);
            const float TrafficInterval = bForceCritical ? 0.0f : FMath::Min(BudgetInterval, 0.35f);
            if (!FMath::IsNearlyEqual(GetActorTickInterval(), TrafficInterval, 0.01f)) SetActorTickInterval(TrafficInterval);
            bAllowExpensiveQueries = Performance->AllowsExpensiveQueries(this, bForceCritical);
        }
    }

    HornCooldownRemaining = FMath::Max(0.0f, HornCooldownRemaining - DeltaSeconds);
    HornVisualRemaining = FMath::Max(0.0f, HornVisualRemaining - DeltaSeconds);
    IncidentStopRemaining = FMath::Max(0.0f, IncidentStopRemaining - DeltaSeconds);
    IncidentLimpRemaining = FMath::Max(0.0f, IncidentLimpRemaining - DeltaSeconds);

    if (HornText && bYieldingForRangerStop && !bIncidentDisabled)
    {
        HornText->SetText(bHoldingForRangerStop ? NSLOCTEXT("GTT", "TrafficRangerStop", "STOP") : NSLOCTEXT("GTT", "TrafficRangerSlow", "SLOW"));
        HornVisualRemaining = FMath::Max(HornVisualRemaining, 0.30f);
    }

    if (HornText && bRoadsideResponderSceneAuthority && bIncidentDisabled)
    {
        HornText->SetText(NSLOCTEXT("GTT", "TrafficRoadService", "SERVICE"));
        HornVisualRemaining = FMath::Max(HornVisualRemaining, 0.30f);
    }

    if (HornText)
    {
        HornText->SetVisibility(HornVisualRemaining > 0.0f || bIncidentDisabled || bRoadsideAssistanceActive || bRoadsideResponderSceneAuthority, true);
        if (HornVisualRemaining <= 0.0f && !bIncidentDisabled && !bRoadsideAssistanceActive && !bRoadsideResponderSceneAuthority) HornText->SetText(NSLOCTEXT("GTT", "TrafficHorn", "BEEP!"));
    }

    if (!VehicleMesh || RoutePoints.Num() < 2 || IsOccupied()) return;

    if (bIncidentDisabled || IncidentStopRemaining > 0.0f || bRoadsideAssistanceActive)
    {
        if (VehicleMesh->IsSimulatingPhysics())
        {
            FVector FlatVelocity = VehicleMesh->GetPhysicsLinearVelocity();
            FlatVelocity.Z = 0.0f;
            if (!FlatVelocity.IsNearlyZero()) VehicleMesh->AddForce(-FlatVelocity.GetSafeNormal() * TrafficDriveForce * (bIncidentDisabled ? 1.8f : 1.25f), NAME_None, true);
            if (!bIncidentDisabled && FMath::Abs(IncidentSteerBias) > 0.08f) VehicleMesh->AddTorqueInRadians(FVector::UpVector * IncidentSteerBias * TrafficSteeringTorque * 0.45f, NAME_None, true);
        }
        StuckTime = 0.0f;
        return;
    }

    if (bHoldingForRangerStop)
    {
        if (VehicleMesh->IsSimulatingPhysics())
        {
            FVector FlatVelocity = VehicleMesh->GetPhysicsLinearVelocity();
            FlatVelocity.Z = 0.0f;
            if (!FlatVelocity.IsNearlyZero()) VehicleMesh->AddForce(-FlatVelocity.GetSafeNormal() * TrafficDriveForce * 1.45f, NAME_None, true);
        }
        StuckTime = 0.0f;
        return;
    }

    FVector ToTarget = RoutePoints[CurrentRoutePoint] - GetActorLocation();
    ToTarget.Z = 0.0f;
    if (ToTarget.SizeSquared2D() <= FMath::Square(RoutePointRadius))
    {
        CurrentRoutePoint = (CurrentRoutePoint + 1) % RoutePoints.Num();
        ToTarget = RoutePoints[CurrentRoutePoint] - GetActorLocation();
        ToTarget.Z = 0.0f;
    }
    if (ToTarget.IsNearlyZero()) return;

    const FVector DesiredDirection = ToTarget.GetSafeNormal2D();
    const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
    const FVector Right = GetActorRightVector().GetSafeNormal2D();
    const float ForwardAlignment = FVector::DotProduct(Forward, DesiredDirection);
    float Steering = FMath::Clamp(FVector::DotProduct(Right, DesiredDirection) * 2.1f, -1.0f, 1.0f);
    const float Speed = GetVelocity().Size2D();
    const float LimpScale = IncidentLimpRemaining > 0.0f ? 0.52f : 1.0f;
    const float EffectiveCruiseSpeedCm = TargetCruiseSpeedCm * LimpScale * RangerStopSpeedScale;

    bool bObstacleAhead = false;
    FHitResult ObstacleHit;
    if (bAllowExpensiveQueries && GetWorld())
    {
        const FVector ProbeStart = GetActorLocation() + Forward * 140.0f + FVector(0.0f, 0.0f, 35.0f);
        const FVector ProbeEnd = ProbeStart + Forward * ObstacleProbeDistance;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTTrafficAvoidance), false, this);
        Params.AddIgnoredActor(this);
        bObstacleAhead = GetWorld()->LineTraceSingleByChannel(ObstacleHit, ProbeStart, ProbeEnd, ECC_Visibility, Params);
    }

    float Throttle = Speed < EffectiveCruiseSpeedCm ? FMath::Clamp(0.35f + ForwardAlignment * 0.35f, 0.12f, IncidentLimpRemaining > 0.0f ? 0.42f : 0.72f) : 0.05f;
    if (bYieldingForRangerStop && Speed > EffectiveCruiseSpeedCm * 1.05f) Throttle = -0.10f;
    if (IncidentLimpRemaining > 0.0f) Steering *= 0.72f;

    if (bObstacleAhead)
    {
        Throttle = -0.08f;
        const float Side = FVector::DotProduct(Right, ObstacleHit.ImpactNormal);
        Steering += Side >= 0.0f ? -0.55f : 0.55f;
        Steering = FMath::Clamp(Steering, -1.0f, 1.0f);
        if (HornCooldownRemaining <= 0.0f)
        {
            HornCooldownRemaining = HornCooldownSeconds;
            HornVisualRemaining = 0.48f;
        }
    }

    if (Speed < 55.0f && !bObstacleAhead && !bYieldingForRangerStop)
    {
        StuckTime += DeltaSeconds;
        if (StuckTime >= StuckRecoverySeconds)
        {
            CurrentRoutePoint = (CurrentRoutePoint + 1) % RoutePoints.Num();
            VehicleMesh->AddImpulse((Forward * 185.0f) + FVector::UpVector * 85.0f, NAME_None, true);
            StuckTime = 0.0f;
        }
    }
    else StuckTime = 0.0f;

    if (VehicleMesh->IsSimulatingPhysics())
    {
        VehicleMesh->AddForce(Forward * Throttle * TrafficDriveForce, NAME_None, true);
        VehicleMesh->AddTorqueInRadians(FVector::UpVector * Steering * TrafficSteeringTorque, NAME_None, true);
    }
}

void AGTTTrafficCarPawn::Interact_Implementation(AActor* Interactor)
{
    if (bIncidentDisabled)
    {
        if (bRoadsideResponderSceneAuthority)
        {
            if (Interactor)
            {
                if (UGTTPlayerEconomyComponent* Economy = Interactor->FindComponentByClass<UGTTPlayerEconomyComponent>())
                {
                    Economy->PushMessage(TEXT("County road service has this scene. Wait for recovery to finish."), 2.8f);
                }
            }
            return;
        }
        if (!bRoadsideAssistanceCompletedForIncident && !bRoadsideAssistanceActive) BeginRoadsideAssistance(Interactor);
        else if (bRoadsideAssistanceActive && Interactor)
        {
            if (UGTTPlayerEconomyComponent* Economy = Interactor->FindComponentByClass<UGTTPlayerEconomyComponent>()) Economy->PushMessage(TEXT("Roadside assist already in progress."), 2.5f);
        }
        return;
    }
    // Ambient traffic keeps its NPC driver. Parked world vehicles remain the theft targets.
}

FText AGTTTrafficCarPawn::GetInteractionText_Implementation() const
{
    if (bRoadsideResponderSceneAuthority && bIncidentDisabled)
    {
        return NSLOCTEXT("GTT", "TrafficCarResponderScene", "County road service - recovery in progress");
    }
    if (bRoadsideAssistanceActive)
    {
        return FText::FromString(FString::Printf(TEXT("Roadside assist - %.1fs remaining"), RoadsideAssistanceRemaining));
    }
    if (bIncidentDisabled && !bRoadsideAssistanceCompletedForIncident)
    {
        return NSLOCTEXT("GTT", "TrafficCarDisabledAssist", "Help disabled driver (roadside assist)");
    }
    if (bIncidentDisabled) return NSLOCTEXT("GTT", "TrafficCarDisabled", "Traffic vehicle - disabled after collision");
    if (IncidentStopRemaining > 0.0f) return NSLOCTEXT("GTT", "TrafficCarIncident", "Traffic vehicle - crash response");
    if (bHoldingForRangerStop) return NSLOCTEXT("GTT", "TrafficCarRangerHold", "Traffic vehicle - yielding at warden stop");
    if (bYieldingForRangerStop) return NSLOCTEXT("GTT", "TrafficCarRangerSlow", "Traffic vehicle - slowing for warden stop");
    return NSLOCTEXT("GTT", "TrafficCarBusy", "Traffic vehicle - driver inside");
}
