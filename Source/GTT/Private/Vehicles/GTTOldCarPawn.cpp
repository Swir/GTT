#include "Vehicles/GTTOldCarPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    void ConfigureCarPart(UStaticMeshComponent* Part, UStaticMesh* Mesh)
    {
        if (!Part) return;
        Part->SetStaticMesh(Mesh);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
    }
}

AGTTOldCarPawn::AGTTOldCarPawn()
{
    VehicleDisplayName = NSLOCTEXT("GTT", "OldCarName", "Rattleback 82");
    PersistentVehicleId = TEXT("Rattleback82");
    MaxCondition = 85.0f;
    Condition = MaxCondition;
    DriveAcceleration = 2350.0f;
    SteeringAcceleration = 175.0f;
    FuelCapacityLiters = 42.0f;
    StartingFuelLiters = 11.0f;
    IdleFuelBurnPerSecond = 0.028f;
    FullThrottleFuelBurnPerSecond = 0.19f;
    TheftHeat = 34.0f;
    MinDamagingImpulse = 105000.0f;
    ImpulsePerDamagePoint = 36000.0f;
    ExitOffset = FVector(0.0f, 165.0f, 65.0f);
    LowConditionFaultChancePerSecond = 0.23f;
    CriticalEngineTemperatureC = 116.0f;

    FGTTVehicleDynamicsProfile Dynamics;
    Dynamics.WheelBaseCm = 260.0f;
    Dynamics.TrackWidthCm = 164.0f;
    Dynamics.SuspensionRestLengthCm = 42.0f;
    Dynamics.WheelRadiusCm = 34.0f;
    Dynamics.SpringStrength = 28.0f;
    Dynamics.DamperStrength = 5.2f;
    Dynamics.LateralGrip = 9.8f;
    Dynamics.RollingResistance = 0.42f;
    Dynamics.MaxDriveForce = 2550.0f;
    Dynamics.MaxSteerTorque = 188.0f;
    Dynamics.MaxSpeedKmh = 128.0f;
    Dynamics.BrakeStrength = 5.5f;
    Dynamics.ForwardGearTopSpeedsKmh = {28.0f, 52.0f, 82.0f, 108.0f, 128.0f};
    ConfigureDynamics(Dynamics);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
    UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;

    if (VehicleMesh && CubeMesh)
    {
        VehicleMesh->SetStaticMesh(CubeMesh);
        VehicleMesh->SetRelativeScale3D(FVector(2.2f, 1.0f, 0.38f));
        VehicleMesh->SetMassOverrideInKg(NAME_None, 980.0f, true);
        VehicleMesh->SetCenterOfMass(FVector(0.0f, 0.0f, -35.0f));
    }

    CabinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinMesh"));
    CabinMesh->SetupAttachment(VehicleMesh);
    ConfigureCarPart(CabinMesh, CubeMesh);
    CabinMesh->SetRelativeLocation(FVector(-10.0f, 0.0f, 95.0f));
    CabinMesh->SetRelativeScale3D(FVector(0.95f, 0.88f, 0.62f));

    TrunkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrunkMesh"));
    TrunkMesh->SetupAttachment(VehicleMesh);
    ConfigureCarPart(TrunkMesh, CubeMesh);
    TrunkMesh->SetRelativeLocation(FVector(-110.0f, 0.0f, 45.0f));
    TrunkMesh->SetRelativeScale3D(FVector(0.5f, 0.92f, 0.28f));

    UStaticMeshComponent* LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoorMesh"));
    LeftDoorMesh->SetupAttachment(VehicleMesh);
    ConfigureCarPart(LeftDoorMesh, CubeMesh);
    LeftDoorMesh->SetRelativeLocation(FVector(-5.0f, -94.0f, 72.0f));
    LeftDoorMesh->SetRelativeScale3D(FVector(0.78f, 0.08f, 0.34f));

    UStaticMeshComponent* RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoorMesh"));
    RightDoorMesh->SetupAttachment(VehicleMesh);
    ConfigureCarPart(RightDoorMesh, CubeMesh);
    RightDoorMesh->SetRelativeLocation(FVector(-5.0f, 94.0f, 72.0f));
    RightDoorMesh->SetRelativeScale3D(FVector(0.78f, 0.08f, 0.34f));

    UStaticMeshComponent* FrontBumperMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontBumperMesh"));
    FrontBumperMesh->SetupAttachment(VehicleMesh);
    ConfigureCarPart(FrontBumperMesh, CubeMesh);
    FrontBumperMesh->SetRelativeLocation(FVector(142.0f, 0.0f, -2.0f));
    FrontBumperMesh->SetRelativeScale3D(FVector(0.12f, 1.02f, 0.12f));

    UStaticMeshComponent* RearBumperMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearBumperMesh"));
    RearBumperMesh->SetupAttachment(VehicleMesh);
    ConfigureCarPart(RearBumperMesh, CubeMesh);
    RearBumperMesh->SetRelativeLocation(FVector(-142.0f, 0.0f, -2.0f));
    RearBumperMesh->SetRelativeScale3D(FVector(0.12f, 1.02f, 0.12f));

    LeftFrontWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFrontWheel"));
    LeftFrontWheel->SetupAttachment(VehicleMesh);
    ConfigureCarPart(LeftFrontWheel, CylinderMesh);
    LeftFrontWheel->SetRelativeLocation(FVector(78.0f, -82.0f, -28.0f));
    LeftFrontWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    LeftFrontWheel->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.22f));

    RightFrontWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFrontWheel"));
    RightFrontWheel->SetupAttachment(VehicleMesh);
    ConfigureCarPart(RightFrontWheel, CylinderMesh);
    RightFrontWheel->SetRelativeLocation(FVector(78.0f, 82.0f, -28.0f));
    RightFrontWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    RightFrontWheel->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.22f));

    LeftRearWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRearWheel"));
    LeftRearWheel->SetupAttachment(VehicleMesh);
    ConfigureCarPart(LeftRearWheel, CylinderMesh);
    LeftRearWheel->SetRelativeLocation(FVector(-82.0f, -82.0f, -28.0f));
    LeftRearWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    LeftRearWheel->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.22f));

    RightRearWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRearWheel"));
    RightRearWheel->SetupAttachment(VehicleMesh);
    ConfigureCarPart(RightRearWheel, CylinderMesh);
    RightRearWheel->SetRelativeLocation(FVector(-82.0f, 82.0f, -28.0f));
    RightRearWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    RightRearWheel->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.22f));

    RegisterBreakablePart(FrontBumperMesh, 0.78f, TEXT("front bumper"));
    RegisterBreakablePart(LeftDoorMesh, 0.62f, TEXT("left door"));
    RegisterBreakablePart(RightDoorMesh, 0.48f, TEXT("right door"));
    RegisterBreakablePart(TrunkMesh, 0.35f, TEXT("trunk lid"));
    RegisterBreakablePart(RearBumperMesh, 0.26f, TEXT("rear bumper"));
    RegisterBreakablePart(RightRearWheel, 0.14f, TEXT("right rear wheel"));
}
