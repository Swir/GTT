#include "Vehicles/GTTOldCarPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    void ConfigureCarPart(UStaticMeshComponent* Part, UStaticMesh* Mesh)
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
}
