#include "Police/GTTPolicePursuitVehicle.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGTTPolicePursuitVehicle::AGTTPolicePursuitVehicle()
{
    PrimaryActorTick.bCanEverTick = true;
    VehicleDisplayName = NSLOCTEXT("GTT", "PolicePursuitCar", "County Patrol Interceptor");
    PersistentVehicleId = NAME_None;
    bIllegalToTake = false;
    MaxCondition = 145.0f;
    DriveAcceleration = 1450.0f;
    SteeringAcceleration = 110.0f;
    FuelCapacityLiters = 70.0f;
    StartingFuelLiters = 70.0f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;

    BeaconLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconLeft"));
    BeaconLeft->SetupAttachment(VehicleMesh);
    BeaconRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconRight"));
    BeaconRight->SetupAttachment(VehicleMesh);
    for (UStaticMeshComponent* Beacon : {BeaconLeft.Get(), BeaconRight.Get()})
    {
        Beacon->SetStaticMesh(SphereMesh);
        Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Beacon->SetCastShadow(false);
        Beacon->SetRelativeScale3D(FVector(0.18f));
    }
    BeaconLeft->SetRelativeLocation(FVector(10.0f, -32.0f, 95.0f));
    BeaconRight->SetRelativeLocation(FVector(10.0f, 32.0f, 95.0f));

    PoliceLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PoliceLabel"));
    PoliceLabel->SetupAttachment(VehicleMesh);
    PoliceLabel->SetText(NSLOCTEXT("GTT", "PoliceVehicleLabel", "POLICE"));
    PoliceLabel->SetHorizontalAlignment(EHTA_Center);
    PoliceLabel->SetWorldSize(42.0f);
    PoliceLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 135.0f));
    PoliceLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
}

void AGTTPolicePursuitVehicle::BeginPlay()
{
    Super::BeginPlay();
    MarkOwnedByPlayer();
}

void AGTTPolicePursuitVehicle::SetResponseTier(int32 InTier)
{
    ResponseTier = FMath::Clamp(InTier, 1, 3);
    PursuitAcceleration = 1350.0f + ResponseTier * 180.0f;
    PursuitSteeringTorque = 100.0f + ResponseTier * 12.0f;
}

void AGTTPolicePursuitVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateBeacon(DeltaSeconds);

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    const int32 WantedLevel = UGTTGameplayStatics::GetPlayerWantedLevel(this, 0);
    if (!PlayerPawn || WantedLevel < 3 || !VehicleMesh || !VehicleMesh->IsSimulatingPhysics()) return;

    DriveTowardPlayer(PlayerPawn, WantedLevel, DeltaSeconds);
}

void AGTTPolicePursuitVehicle::DriveTowardPlayer(APawn* PlayerPawn, int32 WantedLevel, float DeltaSeconds)
{
    const FVector ToTarget = PlayerPawn->GetActorLocation() - GetActorLocation();
    const float Distance = ToTarget.Size2D();
    const FVector DesiredDir = ToTarget.GetSafeNormal2D();
    const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
    const FVector Right = GetActorRightVector().GetSafeNormal2D();

    const float Steering = FMath::Clamp(FVector::DotProduct(Right, DesiredDir), -1.0f, 1.0f);
    const float Facing = FVector::DotProduct(Forward, DesiredDir);
    const float SpeedKmh = GetSpeedKmh();
    const float DesiredSpeed = 62.0f + WantedLevel * 11.0f + ResponseTier * 8.0f;

    float Throttle = FMath::Clamp((DesiredSpeed - SpeedKmh) / FMath::Max(DesiredSpeed, 1.0f), -0.35f, 1.0f);
    if (Distance < BrakeDistance) Throttle = FMath::Min(Throttle, 0.25f);
    if (Facing < -0.25f) Throttle = -0.25f;

    VehicleMesh->AddForce(Forward * Throttle * PursuitAcceleration, NAME_None, true);
    VehicleMesh->AddTorqueInRadians(FVector::UpVector * Steering * PursuitSteeringTorque, NAME_None, true);

    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    if (Distance <= ArrestRadius && SpeedKmh < 24.0f && Now - LastArrestAttemptTime >= ArrestCooldown)
    {
        LastArrestAttemptTime = Now;
        if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this)))
        {
            GameMode->TryArrestPlayer(PlayerPawn);
        }
    }
}

void AGTTPolicePursuitVehicle::UpdateBeacon(float DeltaSeconds)
{
    BeaconClock += DeltaSeconds;
    const bool bLeft = FMath::Fmod(BeaconClock * (2.5f + ResponseTier), 1.0f) < 0.5f;
    if (BeaconLeft) BeaconLeft->SetVisibility(bLeft, true);
    if (BeaconRight) BeaconRight->SetVisibility(!bLeft, true);
}
