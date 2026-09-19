#include "Traffic/GTTRoadsideResponderVehicle.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "UObject/ConstructorHelpers.h"

AGTTRoadsideResponderVehicle::AGTTRoadsideResponderVehicle()
{
    PrimaryActorTick.bCanEverTick = true;
    VehicleDisplayName = NSLOCTEXT("GTT", "RoadServiceVehicle", "County Road Service");
    PersistentVehicleId = NAME_None;
    bIllegalToTake = true;
    TheftHeat = 1.0f;
    MaxCondition = 160.0f;
    FuelCapacityLiters = 80.0f;
    StartingFuelLiters = 80.0f;
    IdleFuelBurnPerSecond = 0.0f;
    FullThrottleFuelBurnPerSecond = 0.0f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));

    if (CubeFinder.Succeeded())
    {
        VehicleMesh->SetStaticMesh(CubeFinder.Object);
        VehicleMesh->SetRelativeScale3D(FVector(2.35f, 1.02f, 0.68f));
    }

    UStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;
    BeaconLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResponderBeaconLeft"));
    BeaconLeft->SetupAttachment(VehicleMesh);
    BeaconRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResponderBeaconRight"));
    BeaconRight->SetupAttachment(VehicleMesh);

    for (UStaticMeshComponent* Beacon : {BeaconLeft.Get(), BeaconRight.Get()})
    {
        Beacon->SetStaticMesh(SphereMesh);
        Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Beacon->SetGenerateOverlapEvents(false);
        Beacon->SetCastShadow(false);
        Beacon->SetRelativeScale3D(FVector(0.16f));
    }
    BeaconLeft->SetRelativeLocation(FVector(8.0f, -34.0f, 96.0f));
    BeaconRight->SetRelativeLocation(FVector(8.0f, 34.0f, 96.0f));

    ServiceLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RoadServiceLabel"));
    ServiceLabel->SetupAttachment(VehicleMesh);
    ServiceLabel->SetText(NSLOCTEXT("GTT", "RoadServiceLabel", "ROAD SERVICE"));
    ServiceLabel->SetHorizontalAlignment(EHTA_Center);
    ServiceLabel->SetWorldSize(38.0f);
    ServiceLabel->SetTextRenderColor(FColor(70, 215, 255));
    ServiceLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));
    ServiceLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

    UStaticMesh* ConeMesh = ConeFinder.Succeeded() ? ConeFinder.Object : nullptr;
    SafetyConeFrontLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResponderSafetyConeFrontLeft"));
    SafetyConeFrontRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResponderSafetyConeFrontRight"));
    SafetyConeRearLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResponderSafetyConeRearLeft"));
    SafetyConeRearRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ResponderSafetyConeRearRight"));

    for (UStaticMeshComponent* Cone : {SafetyConeFrontLeft.Get(), SafetyConeFrontRight.Get(), SafetyConeRearLeft.Get(), SafetyConeRearRight.Get()})
    {
        Cone->SetupAttachment(VehicleMesh);
        Cone->SetStaticMesh(ConeMesh);
        Cone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Cone->SetGenerateOverlapEvents(false);
        Cone->SetCastShadow(false);
        Cone->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.32f));
        Cone->SetVisibility(false, true);
    }
    SafetyConeFrontLeft->SetRelativeLocation(FVector(220.0f, -135.0f, -78.0f));
    SafetyConeFrontRight->SetRelativeLocation(FVector(220.0f, 135.0f, -78.0f));
    SafetyConeRearLeft->SetRelativeLocation(FVector(-220.0f, -135.0f, -78.0f));
    SafetyConeRearRight->SetRelativeLocation(FVector(-220.0f, 135.0f, -78.0f));
}

void AGTTRoadsideResponderVehicle::InitializeIncidentResponse(FName InIncidentId, const FVector& InSceneLocation, bool bStartAtScene)
{
    AssignedIncidentId = InIncidentId;
    SceneLocation = InSceneLocation;
    bParkedAtScene = bStartAtScene;
    bSceneClearing = false;
    SetSafetyCorridorDeployed(bStartAtScene);

    if (bStartAtScene)
    {
        const FVector StopLocation = SceneLocation + FVector(-360.0f, 240.0f, 90.0f);
        SetActorLocation(StopLocation, false, nullptr, ETeleportType::TeleportPhysics);
        if (VehicleMesh && VehicleMesh->IsSimulatingPhysics())
        {
            VehicleMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
            VehicleMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        }
    }
}

void AGTTRoadsideResponderVehicle::BeginSceneClearance()
{
    bParkedAtScene = true;
    bSceneClearing = true;
    SetSafetyCorridorDeployed(true);

    if (VehicleMesh && VehicleMesh->IsSimulatingPhysics())
    {
        VehicleMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
        VehicleMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
}

void AGTTRoadsideResponderVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateBeacon(DeltaSeconds);
    if (!bParkedAtScene)
    {
        DriveTowardScene(DeltaSeconds);
    }
}

void AGTTRoadsideResponderVehicle::DriveTowardScene(float DeltaSeconds)
{
    if (!VehicleMesh || !VehicleMesh->IsSimulatingPhysics() || AssignedIncidentId.IsNone())
    {
        return;
    }

    FVector ToScene = SceneLocation - GetActorLocation();
    ToScene.Z = 0.0f;
    const float Distance = ToScene.Size2D();
    if (Distance <= ArrivalRadiusCm)
    {
        bParkedAtScene = true;
        FVector FlatVelocity = VehicleMesh->GetPhysicsLinearVelocity();
        FlatVelocity.Z = 0.0f;
        if (!FlatVelocity.IsNearlyZero())
        {
            VehicleMesh->AddForce(-FlatVelocity.GetSafeNormal() * ResponseDriveForce * 1.9f, NAME_None, true);
        }
        SetSafetyCorridorDeployed(true);
        return;
    }

    const FVector DesiredDirection = ToScene.GetSafeNormal2D();
    const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
    const FVector Right = GetActorRightVector().GetSafeNormal2D();
    const float Alignment = FVector::DotProduct(Forward, DesiredDirection);
    const float Steering = FMath::Clamp(FVector::DotProduct(Right, DesiredDirection) * 2.0f, -1.0f, 1.0f);
    const float Speed = GetVelocity().Size2D();

    float Throttle = Speed < TargetCruiseSpeedCm ? FMath::Clamp(0.30f + Alignment * 0.45f, -0.18f, 0.85f) : 0.04f;
    if (Distance < 850.0f)
    {
        Throttle = FMath::Min(Throttle, 0.22f);
    }

    VehicleMesh->AddForce(Forward * Throttle * ResponseDriveForce, NAME_None, true);
    VehicleMesh->AddTorqueInRadians(FVector::UpVector * Steering * ResponseSteeringTorque, NAME_None, true);
}

void AGTTRoadsideResponderVehicle::SetSafetyCorridorDeployed(bool bDeployed)
{
    bSafetyCorridorDeployed = bDeployed;

    if (SafetyConeFrontLeft) SafetyConeFrontLeft->SetVisibility(bSafetyCorridorDeployed && !bSceneClearing, true);
    if (SafetyConeFrontRight) SafetyConeFrontRight->SetVisibility(bSafetyCorridorDeployed && !bSceneClearing, true);
    if (SafetyConeRearLeft) SafetyConeRearLeft->SetVisibility(bSafetyCorridorDeployed, true);
    if (SafetyConeRearRight) SafetyConeRearRight->SetVisibility(bSafetyCorridorDeployed, true);

    if (ServiceLabel)
    {
        ServiceLabel->SetText(
            bSceneClearing
                ? NSLOCTEXT("GTT", "RoadServiceLaneReopening", "ROAD SERVICE - LANE REOPENING")
                : bSafetyCorridorDeployed
                    ? NSLOCTEXT("GTT", "RoadServiceSafeCorridor", "ROAD SERVICE - SAFE CORRIDOR")
                    : NSLOCTEXT("GTT", "RoadServiceLabel", "ROAD SERVICE"));
    }
}

void AGTTRoadsideResponderVehicle::UpdateBeacon(float DeltaSeconds)
{
    BeaconClock += FMath::Max(0.0f, DeltaSeconds);
    const bool bLeftVisible = FMath::Fmod(BeaconClock * 3.6f, 1.0f) < 0.5f;
    if (BeaconLeft) BeaconLeft->SetVisibility(bLeftVisible, true);
    if (BeaconRight) BeaconRight->SetVisibility(!bLeftVisible, true);
}

void AGTTRoadsideResponderVehicle::Interact_Implementation(AActor* Interactor)
{
    if (Interactor)
    {
        if (UGTTPlayerEconomyComponent* Economy = Interactor->FindComponentByClass<UGTTPlayerEconomyComponent>())
        {
            Economy->PushMessage(
                bSceneClearing
                    ? TEXT("County road service is reopening the lane after the civilian recovery.")
                    : bSafetyCorridorDeployed
                        ? TEXT("County road service has a safety corridor around the active civilian incident.")
                        : TEXT("County road service is handling an active civilian incident."),
                3.0f);
        }
    }
}

FText AGTTRoadsideResponderVehicle::GetInteractionText_Implementation() const
{
    return bSceneClearing
        ? NSLOCTEXT("GTT", "RoadServiceLaneReopeningInteraction", "County road service - lane reopening")
        : bSafetyCorridorDeployed
            ? NSLOCTEXT("GTT", "RoadServiceSafetyCorridor", "County road service - safety corridor active")
            : NSLOCTEXT("GTT", "RoadServiceEnRoute", "County road service - responding");
}
