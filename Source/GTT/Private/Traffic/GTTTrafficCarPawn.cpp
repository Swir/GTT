#include "Traffic/GTTTrafficCarPawn.h"

#include "Components/StaticMeshComponent.h"

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
    const float Steering = FMath::Clamp(FVector::DotProduct(Right, DesiredDirection) * 2.1f, -1.0f, 1.0f);
    const float Speed = GetVelocity().Size2D();
    const float Throttle = Speed < TargetCruiseSpeedCm
        ? FMath::Clamp(0.35f + ForwardAlignment * 0.35f, 0.12f, 0.72f)
        : 0.05f;

    if (VehicleMesh->IsSimulatingPhysics())
    {
        VehicleMesh->AddForce(Forward * Throttle * TrafficDriveForce, NAME_None, true);
        VehicleMesh->AddTorqueInRadians(FVector::UpVector * Steering * TrafficSteeringTorque, NAME_None, true);
    }
}

void AGTTTrafficCarPawn::Interact_Implementation(AActor* Interactor)
{
    // Ambient traffic is deliberately not player-stealable yet. Parked world vehicles remain the theft targets.
}

FText AGTTTrafficCarPawn::GetInteractionText_Implementation() const
{
    return NSLOCTEXT("GTT", "TrafficCarBusy", "Traffic vehicle - driver inside");
}
