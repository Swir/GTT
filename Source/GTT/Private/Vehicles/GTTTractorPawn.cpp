#include "Vehicles/GTTTractorPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    void ConfigureVisualPart(UStaticMeshComponent* Part, UStaticMesh* Mesh)
    {
        if (!Part)
        {
            return;
        }

        Part->SetStaticMesh(Mesh);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetAbsolute(false, false, true);
    }
}

AGTTTractorPawn::AGTTTractorPawn()
{
    VehicleDisplayName = NSLOCTEXT("GTT", "PrototypeTractorName", "Rusty Fieldmaster 60");
    MaxCondition = 150.0f;
    Condition = MaxCondition;
    DriveAcceleration = 1450.0f;
    SteeringAcceleration = 105.0f;
    TheftHeat = 42.0f;
    MinDamagingImpulse = 160000.0f;
    ImpulsePerDamagePoint = 60000.0f;
    ExitOffset = FVector(0.0f, 220.0f, 90.0f);

    FuelCapacityLiters = 55.0f;
    StartingFuelLiters = 18.0f;
    IdleFuelBurnPerSecond = 0.035f;
    FullThrottleFuelBurnPerSecond = 0.16f;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
    UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;

    if (VehicleMesh && CubeMesh)
    {
        VehicleMesh->SetStaticMesh(CubeMesh);
        VehicleMesh->SetRelativeScale3D(FVector(2.75f, 1.25f, 0.55f));
        VehicleMesh->SetMassOverrideInKg(NAME_None, 1850.0f, true);
        VehicleMesh->SetCenterOfMass(FVector(-20.0f, 0.0f, -55.0f));
    }

    HoodMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HoodMesh"));
    HoodMesh->SetupAttachment(VehicleMesh);
    ConfigureVisualPart(HoodMesh, CubeMesh);
    HoodMesh->SetRelativeLocation(FVector(22.0f, 0.0f, 90.0f));
    HoodMesh->SetRelativeScale3D(FVector(1.45f, 1.05f, 0.65f));

    CabinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinMesh"));
    CabinMesh->SetupAttachment(VehicleMesh);
    ConfigureVisualPart(CabinMesh, CubeMesh);
    CabinMesh->SetRelativeLocation(FVector(-38.0f, 0.0f, 165.0f));
    CabinMesh->SetRelativeScale3D(FVector(0.9f, 1.0f, 1.45f));

    ExhaustMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExhaustMesh"));
    ExhaustMesh->SetupAttachment(VehicleMesh);
    ConfigureVisualPart(ExhaustMesh, CylinderMesh);
    ExhaustMesh->SetRelativeLocation(FVector(32.0f, 44.0f, 165.0f));
    ExhaustMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 1.25f));

    LeftFrontWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFrontWheel"));
    LeftFrontWheel->SetupAttachment(VehicleMesh);
    ConfigureVisualPart(LeftFrontWheel, CylinderMesh);
    LeftFrontWheel->SetRelativeLocation(FVector(54.0f, -82.0f, -5.0f));
    LeftFrontWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    LeftFrontWheel->SetRelativeScale3D(FVector(0.48f, 0.48f, 0.30f));

    RightFrontWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFrontWheel"));
    RightFrontWheel->SetupAttachment(VehicleMesh);
    ConfigureVisualPart(RightFrontWheel, CylinderMesh);
    RightFrontWheel->SetRelativeLocation(FVector(54.0f, 82.0f, -5.0f));
    RightFrontWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    RightFrontWheel->SetRelativeScale3D(FVector(0.48f, 0.48f, 0.30f));

    LeftRearWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRearWheel"));
    LeftRearWheel->SetupAttachment(VehicleMesh);
    ConfigureVisualPart(LeftRearWheel, CylinderMesh);
    LeftRearWheel->SetRelativeLocation(FVector(-57.0f, -86.0f, 4.0f));
    LeftRearWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    LeftRearWheel->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.36f));

    RightRearWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRearWheel"));
    RightRearWheel->SetupAttachment(VehicleMesh);
    ConfigureVisualPart(RightRearWheel, CylinderMesh);
    RightRearWheel->SetRelativeLocation(FVector(-57.0f, 86.0f, 4.0f));
    RightRearWheel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    RightRearWheel->SetRelativeScale3D(FVector(0.72f, 0.72f, 0.36f));
}
