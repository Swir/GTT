#include "Ranger/GTTRangerPatrolVehicle.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "Ranger/GTTRangerRoadStopSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void ConfigureVisualPart(UStaticMeshComponent* Component, UStaticMesh* Mesh, const FVector& Location,
    const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator)
{
    if (!Component)
    {
        return;
    }
    Component->SetStaticMesh(Mesh);
    Component->SetRelativeLocation(Location);
    Component->SetRelativeScale3D(Scale);
    Component->SetRelativeRotation(Rotation);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
}

AGTTRangerPatrolVehicle::AGTTRangerPatrolVehicle()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.08f;
    SetCanBeDamaged(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMesh* CubeMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
    UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
    UStaticMesh* SphereMesh = SphereFinder.Succeeded() ? SphereFinder.Object : nullptr;

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(SceneRoot);
    ConfigureVisualPart(Body, CubeMesh, FVector(0.0f, 0.0f, 54.0f), FVector(1.65f, 0.78f, 0.30f));
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

    Cabin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabin"));
    Cabin->SetupAttachment(SceneRoot);
    ConfigureVisualPart(Cabin, CubeMesh, FVector(-18.0f, 0.0f, 103.0f), FVector(0.76f, 0.68f, 0.40f));

    FrontBumper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontBumper"));
    FrontBumper->SetupAttachment(SceneRoot);
    ConfigureVisualPart(FrontBumper, CubeMesh, FVector(170.0f, 0.0f, 43.0f), FVector(0.12f, 0.82f, 0.09f));

    const FRotator WheelRotation(90.0f, 0.0f, 0.0f);
    WheelFrontLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelFrontLeft"));
    WheelFrontLeft->SetupAttachment(SceneRoot);
    ConfigureVisualPart(WheelFrontLeft, CylinderMesh, FVector(105.0f, -78.0f, 31.0f), FVector(0.38f, 0.38f, 0.20f), WheelRotation);
    WheelFrontRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelFrontRight"));
    WheelFrontRight->SetupAttachment(SceneRoot);
    ConfigureVisualPart(WheelFrontRight, CylinderMesh, FVector(105.0f, 78.0f, 31.0f), FVector(0.38f, 0.38f, 0.20f), WheelRotation);
    WheelRearLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelRearLeft"));
    WheelRearLeft->SetupAttachment(SceneRoot);
    ConfigureVisualPart(WheelRearLeft, CylinderMesh, FVector(-105.0f, -78.0f, 31.0f), FVector(0.42f, 0.42f, 0.22f), WheelRotation);
    WheelRearRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WheelRearRight"));
    WheelRearRight->SetupAttachment(SceneRoot);
    ConfigureVisualPart(WheelRearRight, CylinderMesh, FVector(-105.0f, 78.0f, 31.0f), FVector(0.42f, 0.42f, 0.22f), WheelRotation);

    BeaconLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconLeft"));
    BeaconLeft->SetupAttachment(SceneRoot);
    ConfigureVisualPart(BeaconLeft, SphereMesh, FVector(-20.0f, -34.0f, 151.0f), FVector(0.16f));
    BeaconRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconRight"));
    BeaconRight->SetupAttachment(SceneRoot);
    ConfigureVisualPart(BeaconRight, SphereMesh, FVector(-20.0f, 34.0f, 151.0f), FVector(0.16f));

    BeaconLightLeft = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLightLeft"));
    BeaconLightLeft->SetupAttachment(BeaconLeft);
    BeaconLightLeft->SetLightColor(FLinearColor(1.0f, 0.50f, 0.04f));
    BeaconLightLeft->SetIntensity(2800.0f);
    BeaconLightLeft->SetAttenuationRadius(650.0f);
    BeaconLightLeft->SetCastShadows(false);

    BeaconLightRight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLightRight"));
    BeaconLightRight->SetupAttachment(BeaconRight);
    BeaconLightRight->SetLightColor(FLinearColor(1.0f, 0.50f, 0.04f));
    BeaconLightRight->SetIntensity(2800.0f);
    BeaconLightRight->SetAttenuationRadius(650.0f);
    BeaconLightRight->SetCastShadows(false);

    SearchLamp = CreateDefaultSubobject<USpotLightComponent>(TEXT("SearchLamp"));
    SearchLamp->SetupAttachment(SceneRoot);
    SearchLamp->SetRelativeLocation(FVector(105.0f, 0.0f, 118.0f));
    SearchLamp->SetRelativeRotation(FRotator(-24.0f, 0.0f, 0.0f));
    SearchLamp->SetLightColor(FLinearColor(0.76f, 0.88f, 1.0f));
    SearchLamp->SetIntensity(3400.0f);
    SearchLamp->SetAttenuationRadius(1150.0f);
    SearchLamp->SetInnerConeAngle(18.0f);
    SearchLamp->SetOuterConeAngle(34.0f);
    SearchLamp->SetCastShadows(false);

    WardenLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("WardenLabel"));
    WardenLabel->SetupAttachment(SceneRoot);
    WardenLabel->SetText(NSLOCTEXT("GTT", "RangerPatrolVehicleLabel", "WARDEN"));
    WardenLabel->SetHorizontalAlignment(EHTA_Center);
    WardenLabel->SetWorldSize(34.0f);
    WardenLabel->SetRelativeLocation(FVector(-5.0f, 0.0f, 176.0f));
    WardenLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
}

void AGTTRangerPatrolVehicle::BeginPlay()
{
    Super::BeginPlay();
    bRoadsideDeployed = false;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
    BeaconLeft->SetVisibility(false, true);
    BeaconRight->SetVisibility(false, true);
    BeaconLightLeft->SetVisibility(false, true);
    BeaconLightRight->SetVisibility(false, true);
    SearchLamp->SetVisibility(false, true);
}

void AGTTRangerPatrolVehicle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UGTTRangerRoadStopSubsystem* RoadStop = GetWorld()
        ? GetWorld()->GetSubsystem<UGTTRangerRoadStopSubsystem>()
        : nullptr;
    FTransform SceneTransform;
    const bool bShouldDeploy = RoadStop && RoadStop->GetPatrolVehicleTransform(SceneTransform);
    SetRoadsideDeployed(bShouldDeploy);
    if (!bShouldDeploy)
    {
        return;
    }

    SetActorLocationAndRotation(
        ResolveGroundedLocation(SceneTransform.GetLocation()),
        SceneTransform.GetRotation(),
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
    UpdateBeacons(DeltaSeconds, RoadStop->GetPhase() == EGTTRangerRoadStopPhase::Search);
}

void AGTTRangerPatrolVehicle::SetRoadsideDeployed(bool bDeployed)
{
    if (bRoadsideDeployed == bDeployed)
    {
        return;
    }

    bRoadsideDeployed = bDeployed;
    SetActorHiddenInGame(!bRoadsideDeployed);
    SetActorEnableCollision(bRoadsideDeployed);
    if (!bRoadsideDeployed)
    {
        BeaconLeft->SetVisibility(false, true);
        BeaconRight->SetVisibility(false, true);
        BeaconLightLeft->SetVisibility(false, true);
        BeaconLightRight->SetVisibility(false, true);
        SearchLamp->SetVisibility(false, true);
    }
}

FVector AGTTRangerPatrolVehicle::ResolveGroundedLocation(const FVector& DesiredLocation) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return DesiredLocation;
    }

    FHitResult Hit;
    const FVector TraceStart = DesiredLocation + FVector(0.0f, 0.0f, 450.0f);
    const FVector TraceEnd = DesiredLocation - FVector(0.0f, 0.0f, 1000.0f);
    FCollisionObjectQueryParams ObjectQuery;
    ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GTTRangerPatrolGround), false, this);
    if (World->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, ObjectQuery, QueryParams))
    {
        FVector Grounded = DesiredLocation;
        Grounded.Z = Hit.ImpactPoint.Z + GroundClearanceCm;
        return Grounded;
    }
    return DesiredLocation;
}

void AGTTRangerPatrolVehicle::UpdateBeacons(float DeltaSeconds, bool bSearchPhase)
{
    BeaconClock += DeltaSeconds;
    const bool bLeft = FMath::Fmod(BeaconClock * 3.4f, 1.0f) < 0.5f;
    BeaconLeft->SetVisibility(bLeft, true);
    BeaconRight->SetVisibility(!bLeft, true);
    BeaconLightLeft->SetVisibility(bLeft, true);
    BeaconLightRight->SetVisibility(!bLeft, true);
    SearchLamp->SetVisibility(bSearchPhase, true);
}
