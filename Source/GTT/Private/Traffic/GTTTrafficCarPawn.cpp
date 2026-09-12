#include "Traffic/GTTTrafficCarPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"

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

void AGTTTrafficCarPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    HornCooldownRemaining = FMath::Max(0.0f, HornCooldownRemaining - DeltaSeconds);
    HornVisualRemaining = FMath::Max(0.0f, HornVisualRemaining - DeltaSeconds);
    if (HornText)
    {
        HornText->SetVisibility(HornVisualRemaining > 0.0f, true);
    }

    if (!VehicleMesh || RoutePoints.Num() < 2 || IsOccupied())
    {
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

    bool bObstacleAhead = false;
    FHitResult ObstacleHit;
    if (GetWorld())
    {
        const FVector ProbeStart = GetActorLocation() + Forward * 140.0f + FVector(0.0f, 0.0f, 35.0f);
        const FVector ProbeEnd = ProbeStart + Forward * ObstacleProbeDistance;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTTrafficAvoidance), false, this);
        Params.AddIgnoredActor(this);
        bObstacleAhead = GetWorld()->LineTraceSingleByChannel(ObstacleHit, ProbeStart, ProbeEnd, ECC_Visibility, Params);
    }

    float Throttle = Speed < TargetCruiseSpeedCm
        ? FMath::Clamp(0.35f + ForwardAlignment * 0.35f, 0.12f, 0.72f)
        : 0.05f;

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
    return NSLOCTEXT("GTT", "TrafficCarBusy", "Traffic vehicle - driver inside");
}
