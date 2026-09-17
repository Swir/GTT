#include "Ranger/GTTRangerPullOverMarker.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Ranger/GTTRangerRoadStopSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void ConfigureMarkerMesh(UStaticMeshComponent* Component, UStaticMesh* Mesh,
    const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator)
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
    Component->SetCastShadow(false);
}
}

AGTTRangerPullOverMarker::AGTTRangerPullOverMarker()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.05f;
    bCanBeDamaged = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
    UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
    UStaticMesh* ConeMesh = ConeFinder.Succeeded() ? ConeFinder.Object : nullptr;

    GroundDisc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundDisc"));
    GroundDisc->SetupAttachment(SceneRoot);
    ConfigureMarkerMesh(GroundDisc, CylinderMesh, FVector(0.0f, 0.0f, 3.0f), FVector(1.35f, 1.35f, 0.025f));

    DirectionChevron = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DirectionChevron"));
    DirectionChevron->SetupAttachment(SceneRoot);
    ConfigureMarkerMesh(DirectionChevron, ConeMesh, FVector(0.0f, 0.0f, 24.0f), FVector(0.34f, 0.34f, 0.24f));

    MarkerLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MarkerLabel"));
    MarkerLabel->SetupAttachment(SceneRoot);
    MarkerLabel->SetText(NSLOCTEXT("GTT", "RangerPullOverMarkerLabel", "PULL OVER"));
    MarkerLabel->SetHorizontalAlignment(EHTA_Center);
    MarkerLabel->SetWorldSize(34.0f);
    MarkerLabel->SetTextRenderColor(FColor(255, 205, 72));
    MarkerLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));
    MarkerLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

    MarkerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MarkerLight"));
    MarkerLight->SetupAttachment(SceneRoot);
    MarkerLight->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
    MarkerLight->SetLightColor(FLinearColor(1.0f, 0.55f, 0.08f));
    MarkerLight->SetAttenuationRadius(430.0f);
    MarkerLight->SetIntensity(1700.0f);
    MarkerLight->SetCastShadows(false);
}

void AGTTRangerPullOverMarker::BeginPlay()
{
    Super::BeginPlay();
    bMarkerDeployed = false;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

void AGTTRangerPullOverMarker::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UGTTRangerRoadStopSubsystem* RoadStop = GetWorld()
        ? GetWorld()->GetSubsystem<UGTTRangerRoadStopSubsystem>()
        : nullptr;
    FTransform MarkerTransform;
    const bool bShouldDeploy = RoadStop && RoadStop->GetPullOverMarkerTransform(MarkerTransform);
    SetMarkerDeployed(bShouldDeploy);
    if (!bShouldDeploy)
    {
        return;
    }

    const FVector GroundedLocation = ResolveGroundedLocation(MarkerTransform.GetLocation());
    SetActorLocationAndRotation(
        GroundedLocation,
        MarkerTransform.GetRotation(),
        false,
        nullptr,
        ETeleportType::TeleportPhysics);

    const bool bSearchPhase = RoadStop->GetPhase() == EGTTRangerRoadStopPhase::Search;
    UpdatePresentation(DeltaSeconds, bSearchPhase);
}

void AGTTRangerPullOverMarker::SetMarkerDeployed(bool bDeployed)
{
    if (bMarkerDeployed == bDeployed)
    {
        return;
    }

    bMarkerDeployed = bDeployed;
    SetActorHiddenInGame(!bMarkerDeployed);
    SetActorEnableCollision(false);
    if (!bMarkerDeployed)
    {
        MarkerLight->SetVisibility(false, true);
    }
}

FVector AGTTRangerPullOverMarker::ResolveGroundedLocation(const FVector& DesiredLocation) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return DesiredLocation;
    }

    FHitResult Hit;
    const FVector TraceStart = DesiredLocation + FVector(0.0f, 0.0f, 350.0f);
    const FVector TraceEnd = DesiredLocation - FVector(0.0f, 0.0f, 900.0f);
    FCollisionObjectQueryParams ObjectQuery;
    ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GTTRangerPullOverMarkerGround), false, this);
    if (World->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, ObjectQuery, QueryParams))
    {
        FVector Grounded = DesiredLocation;
        Grounded.Z = Hit.ImpactPoint.Z + 4.0f;
        return Grounded;
    }
    return DesiredLocation;
}

void AGTTRangerPullOverMarker::UpdatePresentation(float DeltaSeconds, bool bSearchPhase)
{
    PulseClock += DeltaSeconds;
    const float Pulse = 0.5f + 0.5f * FMath::Sin(PulseClock * (bSearchPhase ? 2.4f : 5.2f));

    // Once the player is stopped, retain only a subdued ground reference. The
    // floating instruction disappears so SEARCH stays readable instead of becoming
    // another wall of text in front of the vehicle.
    MarkerLabel->SetVisibility(!bSearchPhase, true);
    DirectionChevron->SetVisibility(!bSearchPhase, true);
    MarkerLight->SetVisibility(true, true);
    MarkerLight->SetIntensity(bSearchPhase
        ? FMath::Lerp(220.0f, 420.0f, Pulse)
        : FMath::Lerp(900.0f, 2100.0f, Pulse));
}