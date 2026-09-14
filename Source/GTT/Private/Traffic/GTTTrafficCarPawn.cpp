#include "Traffic/GTTTrafficCarPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
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
    if (RoutePoints.Num() > 0)
    {
        CurrentRoutePoint = FMath::Abs(StartIndex) % RoutePoints.Num();
        SetActorLocation(RoutePoints[CurrentRoutePoint] + FVector(0.0f, 0.0f, 90.0f), false, nullptr, ETeleportType::TeleportPhysics);
        CurrentRoutePoint = (CurrentRoutePoint + 1) % RoutePoints.Num();
    }
}

void AGTTTrafficCarPawn::RegisterCollisionIncident(float ImpactSpeedKmh, FVector SourceLocation)
{
    const float Severity = FMath::Clamp((ImpactSpeedKmh - 12.0f) / 58.0f, 0.0f, 1.0f);
    IncidentStopRemaining = FMath::Max(IncidentStopRemaining, FMath::Lerp(CollisionStopSeconds * 0.65f, CollisionStopSeconds * 1.65f, Severity));
    IncidentLimpRemaining = FMath::Max(IncidentLimpRemaining, FMath::Lerp(IncidentLimpSeconds * 0.55f, IncidentLimpSeconds * 1.25f, Severity));

    const FVector Away = (GetActorLocation() - SourceLocation).GetSafeNormal2D();
    IncidentSteerBias = FMath::Clamp(FVector::DotProduct(GetActorRightVector().GetSafeNormal2D(), Away), -1.0f, 1.0f);
    bIncidentDisabled = GetConditionPercent() <= DisableConditionThreshold;

    if (HornText)
    {
        HornText->SetText(bIncidentDisabled
            ? NSLOCTEXT("GTT", "TrafficDisabled", "HAZARD")
            : NSLOCTEXT("GTT", "TrafficCrashHazard", "CAUTION"));
        HornVisualRemaining = FMath::Max(HornVisualRemaining, IncidentStopRemaining);
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

void AGTTTrafficCarPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    bool bAllowExpensiveQueries = true;
    if (GetWorld())
    {
        if (UGTTWorldPerformanceSubsystem* Performance = GetWorld()->GetSubsystem<UGTTWorldPerformanceSubsystem>())
        {
            const bool bForceCritical = IsOccupied() || IncidentStopRemaining > 0.0f || bIncidentDisabled;
            const float BudgetInterval = Performance->GetRecommendedTickInterval(this, bForceCritical);
            const float TrafficInterval = bForceCritical ? 0.0f : FMath::Min(BudgetInterval, 0.35f);
            if (!FMath::IsNearlyEqual(GetActorTickInterval(), TrafficInterval, 0.01f))
            {
                SetActorTickInterval(TrafficInterval);
            }
            bAllowExpensiveQueries = Performance->AllowsExpensiveQueries(this, bForceCritical);
        }
    }

    HornCooldownRemaining = FMath::Max(0.0f, HornCooldownRemaining - DeltaSeconds);
    HornVisualRemaining = FMath::Max(0.0f, HornVisualRemaining - DeltaSeconds);
    IncidentStopRemaining = FMath::Max(0.0f, IncidentStopRemaining - DeltaSeconds);
    IncidentLimpRemaining = FMath::Max(0.0f, IncidentLimpRemaining - DeltaSeconds);

    if (HornText)
    {
        HornText->SetVisibility(HornVisualRemaining > 0.0f || bIncidentDisabled, true);
        if (HornVisualRemaining <= 0.0f && !bIncidentDisabled)
        {
            HornText->SetText(NSLOCTEXT("GTT", "TrafficHorn", "BEEP!"));
        }
    }

    if (!VehicleMesh || RoutePoints.Num() < 2 || IsOccupied())
    {
        return;
    }

    if (bIncidentDisabled || IncidentStopRemaining > 0.0f)
    {
        if (VehicleMesh->IsSimulatingPhysics())
        {
            FVector FlatVelocity = VehicleMesh->GetPhysicsLinearVelocity();
            FlatVelocity.Z = 0.0f;
            if (!FlatVelocity.IsNearlyZero())
            {
                VehicleMesh->AddForce(-FlatVelocity.GetSafeNormal() * TrafficDriveForce * (bIncidentDisabled ? 1.8f : 1.25f), NAME_None, true);
            }
            if (!bIncidentDisabled && FMath::Abs(IncidentSteerBias) > 0.08f)
            {
                VehicleMesh->AddTorqueInRadians(FVector::UpVector * IncidentSteerBias * TrafficSteeringTorque * 0.45f, NAME_None, true);
            }
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

    if (ToTarget.IsNearlyZero())
    {
        return;
    }

    const FVector DesiredDirection = ToTarget.GetSafeNormal2D();
    const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
    const FVector Right = GetActorRightVector().GetSafeNormal2D();
    const float ForwardAlignment = FVector::DotProduct(Forward, DesiredDirection);
    float Steering = FMath::Clamp(FVector::DotProduct(Right, DesiredDirection) * 2.1f, -1.0f, 1.0f);
    const float Speed = GetVelocity().Size2D();
    const float EffectiveCruiseSpeedCm = IncidentLimpRemaining > 0.0f ? TargetCruiseSpeedCm * 0.52f : TargetCruiseSpeedCm;

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

    float Throttle = Speed < EffectiveCruiseSpeedCm
        ? FMath::Clamp(0.35f + ForwardAlignment * 0.35f, 0.12f, IncidentLimpRemaining > 0.0f ? 0.42f : 0.72f)
        : 0.05f;

    if (IncidentLimpRemaining > 0.0f)
    {
        Steering *= 0.72f;
    }

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

    if (Speed < 55.0f && !bObstacleAhead)
    {
        StuckTime += DeltaSeconds;
        if (StuckTime >= StuckRecoverySeconds)
        {
            CurrentRoutePoint = (CurrentRoutePoint + 1) % RoutePoints.Num();
            VehicleMesh->AddImpulse((Forward * 185.0f) + FVector::UpVector * 85.0f, NAME_None, true);
            StuckTime = 0.0f;
        }
    }
    else
    {
        StuckTime = 0.0f;
    }

    if (VehicleMesh->IsSimulatingPhysics())
    {
        VehicleMesh->AddForce(Forward * Throttle * TrafficDriveForce, NAME_None, true);
        VehicleMesh->AddTorqueInRadians(FVector::UpVector * Steering * TrafficSteeringTorque, NAME_None, true);
    }
}

void AGTTTrafficCarPawn::Interact_Implementation(AActor* Interactor)
{
    // Ambient traffic keeps its NPC driver for now. Parked world vehicles remain the theft targets.
}

FText AGTTTrafficCarPawn::GetInteractionText_Implementation() const
{
    if (bIncidentDisabled)
    {
        return NSLOCTEXT("GTT", "TrafficCarDisabled", "Traffic vehicle - disabled after collision");
    }
    if (IncidentStopRemaining > 0.0f)
    {
        return NSLOCTEXT("GTT", "TrafficCarIncident", "Traffic vehicle - crash response");
    }
    return NSLOCTEXT("GTT", "TrafficCarBusy", "Traffic vehicle - driver inside");
}
