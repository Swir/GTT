#include "Vehicles/GTTFarmVanPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    void ConfigureVanPart(UStaticMeshComponent* Part, UStaticMesh* Mesh)
    {
        if (!Part)
        {
            return;
        }
        Part->SetStaticMesh(Mesh);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
    }
}

AGTTFarmVanPawn::AGTTFarmVanPawn()
{
    VehicleDisplayName = NSLOCTEXT("GTT", "FarmVanName", "Mulebox 1200");
    PersistentVehicleId = TEXT("Mulebox1200");
    MaxCondition = 125.0f;
    Condition = MaxCondition;
    DriveAcceleration = 1750.0f;
    SteeringAcceleration = 118.0f;
    FuelCapacityLiters = 62.0f;
    StartingFuelLiters = 24.0f;
    IdleFuelBurnPerSecond = 0.04f;
    FullThrottleFuelBurnPerSecond = 0.22f;
    TheftHeat = 31.0f;
    MinDamagingImpulse = 145000.0f;
    ImpulsePerDamagePoint = 52000.0f;
    ExitOffset = FVector(0.0f, 205.0f, 82.0f);
    LowConditionFaultChancePerSecond = 0.12f;
    CriticalEngineTemperatureC = 124.0f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
    UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;

    if (VehicleMesh && CubeMesh)
    {
        VehicleMesh->SetStaticMesh(CubeMesh);
        VehicleMesh->SetRelativeScale3D(FVector(2.45f, 1.08f, 0.52f));
        VehicleMesh->SetMassOverrideInKg(NAME_None, 1680.0f, true);
        VehicleMesh->SetCenterOfMass(FVector(-10.0f, 0.0f, -48.0f));
    }

    CabinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinMesh"));
    CabinMesh->SetupAttachment(VehicleMesh);
    ConfigureVanPart(CabinMesh, CubeMesh);
    CabinMesh->SetRelativeLocation(FVector(70.0f, 0.0f, 105.0f));
    CabinMesh->SetRelativeScale3D(FVector(0.72f, 0.96f, 0.9f));

    CargoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoMesh"));
    CargoMesh->SetupAttachment(VehicleMesh);
    ConfigureVanPart(CargoMesh, CubeMesh);
    CargoMesh->SetRelativeLocation(FVector(-65.0f, 0.0f, 118.0f));
    CargoMesh->SetRelativeScale3D(FVector(1.05f, 1.0f, 1.05f));

    UStaticMeshComponent* SlidingDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlidingDoorMesh"));
    SlidingDoorMesh->SetupAttachment(VehicleMesh);
    ConfigureVanPart(SlidingDoorMesh, CubeMesh);
    SlidingDoorMesh->SetRelativeLocation(FVector(-35.0f, 104.0f, 100.0f));
    SlidingDoorMesh->SetRelativeScale3D(FVector(0.78f, 0.08f, 0.72f));

    UStaticMeshComponent* RearDoorLeftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearDoorLeftMesh"));
    RearDoorLeftMesh->SetupAttachment(VehicleMesh);
    ConfigureVanPart(RearDoorLeftMesh, CubeMesh);
    RearDoorLeftMesh->SetRelativeLocation(FVector(-157.0f, -48.0f, 105.0f));
    RearDoorLeftMesh->SetRelativeScale3D(FVector(0.08f, 0.45f, 0.78f));

    UStaticMeshComponent* RearDoorRightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearDoorRightMesh"));
    RearDoorRightMesh->SetupAttachment(VehicleMesh);
    ConfigureVanPart(RearDoorRightMesh, CubeMesh);
    RearDoorRightMesh->SetRelativeLocation(FVector(-157.0f, 48.0f, 105.0f));
    RearDoorRightMesh->SetRelativeScale3D(FVector(0.08f, 0.45f, 0.78f));

    UStaticMeshComponent* FrontBumperMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VanFrontBumperMesh"));
    FrontBumperMesh->SetupAttachment(VehicleMesh);
    ConfigureVanPart(FrontBumperMesh, CubeMesh);
    FrontBumperMesh->SetRelativeLocation(FVector(162.0f, 0.0f, 0.0f));
    FrontBumperMesh->SetRelativeScale3D(FVector(0.12f, 1.08f, 0.14f));

    LeftFrontWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFrontWheel"));
    LeftFrontWheel->SetupAttachment(VehicleMesh);
    ConfigureVanPart(LeftFrontWheel, CylinderMesh);
    LeftFrontWheel->SetRelativeLocation(FVector(88.0f, -88.0f, -20.0f));
    LeftFrontWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    LeftFrontWheel->SetRelativeScale3D(FVector(0.39f, 0.39f, 0.25f));

    RightFrontWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFrontWheel"));
    RightFrontWheel->SetupAttachment(VehicleMesh);
    ConfigureVanPart(RightFrontWheel, CylinderMesh);
    RightFrontWheel->SetRelativeLocation(FVector(88.0f, 88.0f, -20.0f));
    RightFrontWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    RightFrontWheel->SetRelativeScale3D(FVector(0.39f, 0.39f, 0.25f));

    LeftRearWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRearWheel"));
    LeftRearWheel->SetupAttachment(VehicleMesh);
    ConfigureVanPart(LeftRearWheel, CylinderMesh);
    LeftRearWheel->SetRelativeLocation(FVector(-92.0f, -88.0f, -20.0f));
    LeftRearWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    LeftRearWheel->SetRelativeScale3D(FVector(0.39f, 0.39f, 0.25f));

    RightRearWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRearWheel"));
    RightRearWheel->SetupAttachment(VehicleMesh);
    ConfigureVanPart(RightRearWheel, CylinderMesh);
    RightRearWheel->SetRelativeLocation(FVector(-92.0f, 88.0f, -20.0f));
    RightRearWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    RightRearWheel->SetRelativeScale3D(FVector(0.39f, 0.39f, 0.25f));

    RegisterBreakablePart(FrontBumperMesh, 0.70f, TEXT("front bumper"));
    RegisterBreakablePart(SlidingDoorMesh, 0.52f, TEXT("sliding door"));
    RegisterBreakablePart(RearDoorLeftMesh, 0.34f, TEXT("left rear door"));
    RegisterBreakablePart(RearDoorRightMesh, 0.24f, TEXT("right rear door"));
    RegisterBreakablePart(LeftRearWheel, 0.12f, TEXT("left rear wheel"));
}
