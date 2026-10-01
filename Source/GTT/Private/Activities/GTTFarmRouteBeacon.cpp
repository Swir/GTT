#include "Activities/GTTFarmRouteBeacon.h"

#include "Activities/GTTFarmJobDirector.h"
#include "Activities/GTTFarmJobTerminal.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void ConfigureBeaconMesh(UStaticMeshComponent* Component, UStaticMesh* Mesh, const FVector& Location, const FVector& Scale)
{
    if (!Component) return;
    Component->SetStaticMesh(Mesh);
    Component->SetRelativeLocation(Location);
    Component->SetRelativeScale3D(Scale);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCastShadow(false);
}
}

AGTTFarmRouteBeacon::AGTTFarmRouteBeacon()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.10f;
    SetCanBeDamaged(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
    UStaticMesh* CylinderMesh = CylinderFinder.Succeeded() ? CylinderFinder.Object : nullptr;
    UStaticMesh* ConeMesh = ConeFinder.Succeeded() ? ConeFinder.Object : nullptr;

    GroundRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundRing"));
    GroundRing->SetupAttachment(SceneRoot);
    ConfigureBeaconMesh(GroundRing, CylinderMesh, FVector(0.0f, 0.0f, 3.0f), FVector(1.15f, 1.15f, 0.025f));

    Pointer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pointer"));
    Pointer->SetupAttachment(SceneRoot);
    ConfigureBeaconMesh(Pointer, ConeMesh, FVector(0.0f, 0.0f, 95.0f), FVector(0.30f, 0.30f, 0.42f));
    Pointer->SetRelativeRotation(FRotator(180.0f, 0.0f, 0.0f));

    RouteLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RouteLabel"));
    RouteLabel->SetupAttachment(SceneRoot);
    RouteLabel->SetHorizontalAlignment(EHTA_Center);
    RouteLabel->SetWorldSize(30.0f);
    RouteLabel->SetTextRenderColor(FColor(98, 229, 255));
    RouteLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 165.0f));

    RouteLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RouteLight"));
    RouteLight->SetupAttachment(SceneRoot);
    RouteLight->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
    RouteLight->SetLightColor(FLinearColor(0.10f, 0.72f, 1.0f));
    RouteLight->SetAttenuationRadius(360.0f);
    RouteLight->SetIntensity(900.0f);
    RouteLight->SetCastShadows(false);
}

void AGTTFarmRouteBeacon::BeginPlay()
{
    Super::BeginPlay();
    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);
    bDeployed = false;
}

void AGTTFarmRouteBeacon::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    AGTTFarmJobDirector* FarmDirector = ResolveDirector();
    if (!FarmDirector || !FarmDirector->IsJobActive())
    {
        CachedStageValue = 255;
        TargetTerminal.Reset();
        SetDeployed(false);
        return;
    }

    const uint8 StageValue = static_cast<uint8>(FarmDirector->GetStage());
    if (StageValue != CachedStageValue || !TargetTerminal.IsValid())
    {
        CachedStageValue = StageValue;
        TargetTerminal = ResolveTargetTerminal(StageValue);
    }

    if (!TargetTerminal.IsValid())
    {
        SetDeployed(false);
        return;
    }

    SetDeployed(true);
    const FVector GroundedLocation = ResolveGroundedLocation(TargetTerminal->GetActorLocation());
    SetActorLocation(GroundedLocation, false, nullptr, ETeleportType::TeleportPhysics);

    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    const float DistanceMeters = PlayerPawn
        ? FVector::Dist2D(PlayerPawn->GetActorLocation(), GroundedLocation) / 100.0f
        : 0.0f;
    UpdatePresentation(DeltaSeconds, StageValue, DistanceMeters);
    FaceLocalPlayer();
}

AGTTFarmJobDirector* AGTTFarmRouteBeacon::ResolveDirector()
{
    if (Director.IsValid()) return Director.Get();
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    for (TActorIterator<AGTTFarmJobDirector> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            Director = *It;
            return Director.Get();
        }
    }
    return nullptr;
}

AGTTFarmJobTerminal* AGTTFarmRouteBeacon::ResolveTargetTerminal(uint8 StageValue)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    EGTTFarmJobTerminalType DesiredType = EGTTFarmJobTerminalType::Start;
    if (StageValue == static_cast<uint8>(EGTTFarmJobStage::ReachPickup)) DesiredType = EGTTFarmJobTerminalType::Pickup;
    else if (StageValue == static_cast<uint8>(EGTTFarmJobStage::DeliverCargo)) DesiredType = EGTTFarmJobTerminalType::Finish;
    else if (StageValue == static_cast<uint8>(EGTTFarmJobStage::DeliverFinalStop)) DesiredType = EGTTFarmJobTerminalType::FinalFinish;
    else return nullptr;

    for (TActorIterator<AGTTFarmJobTerminal> It(World); It; ++It)
    {
        AGTTFarmJobTerminal* Terminal = *It;
        if (IsValid(Terminal) && Terminal->GetTerminalType() == DesiredType) return Terminal;
    }
    return nullptr;
}

FVector AGTTFarmRouteBeacon::ResolveGroundedLocation(const FVector& DesiredLocation) const
{
    UWorld* World = GetWorld();
    if (!World) return DesiredLocation;

    FHitResult Hit;
    const FVector TraceStart = DesiredLocation + FVector(0.0f, 0.0f, 320.0f);
    const FVector TraceEnd = DesiredLocation - FVector(0.0f, 0.0f, 700.0f);
    FCollisionObjectQueryParams ObjectQuery;
    ObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GTTFarmRouteBeaconGround), false, this);
    if (World->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, ObjectQuery, QueryParams))
    {
        FVector Grounded = DesiredLocation;
        Grounded.Z = Hit.ImpactPoint.Z + 4.0f;
        return Grounded;
    }
    return DesiredLocation;
}

void AGTTFarmRouteBeacon::SetDeployed(bool bShouldDeploy)
{
    if (bDeployed == bShouldDeploy) return;
    bDeployed = bShouldDeploy;
    SetActorHiddenInGame(!bDeployed);
    SetActorEnableCollision(false);
    RouteLight->SetVisibility(bDeployed, true);
}

void AGTTFarmRouteBeacon::UpdatePresentation(float DeltaSeconds, uint8 StageValue, float DistanceMeters)
{
    PulseClock += DeltaSeconds;
    const float Pulse = 0.5f + 0.5f * FMath::Sin(PulseClock * 3.8f);
    GroundRing->SetRelativeScale3D(FVector(FMath::Lerp(1.05f, 1.22f, Pulse), FMath::Lerp(1.05f, 1.22f, Pulse), 0.025f));
    Pointer->SetRelativeLocation(FVector(0.0f, 0.0f, FMath::Lerp(82.0f, 112.0f, Pulse)));
    RouteLight->SetIntensity(FMath::Lerp(520.0f, 1450.0f, Pulse));

    FString Label;
    if (StageValue == static_cast<uint8>(EGTTFarmJobStage::ReachPickup))
    {
        RouteLabel->SetTextRenderColor(FColor(98, 229, 255));
        RouteLight->SetLightColor(FLinearColor(0.10f, 0.72f, 1.0f));
        Label = FString::Printf(TEXT("FEED DEPOT | LOAD CARGO\n%.0f m"), DistanceMeters);
    }
    else if (StageValue == static_cast<uint8>(EGTTFarmJobStage::DeliverCargo))
    {
        RouteLabel->SetTextRenderColor(FColor(126, 235, 150));
        RouteLight->SetLightColor(FLinearColor(0.18f, 0.88f, 0.36f));
        Label = FString::Printf(TEXT("HILL FARM | HANDOFF\n%.0f m | %.0fs | cargo %.0f%%"),
            DistanceMeters, Director->GetTimeRemaining(), Director->GetCargoIntegrity() * 100.0f);
    }
    else
    {
        RouteLabel->SetTextRenderColor(FColor(255, 205, 72));
        RouteLight->SetLightColor(FLinearColor(1.0f, 0.55f, 0.08f));
        Label = FString::Printf(TEXT("NORTH WOOD YARD | FINAL\n%.0f m | %.0fs | cargo %.0f%%"),
            DistanceMeters, Director->GetTimeRemaining(), Director->GetCargoIntegrity() * 100.0f);
    }
    RouteLabel->SetText(FText::FromString(Label));
}

void AGTTFarmRouteBeacon::FaceLocalPlayer()
{
    APawn* Viewer = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Viewer || !RouteLabel) return;
    const FVector ToViewer = Viewer->GetActorLocation() - RouteLabel->GetComponentLocation();
    if (!ToViewer.IsNearlyZero()) RouteLabel->SetWorldRotation(ToViewer.Rotation() + FRotator(0.0f, 180.0f, 0.0f));
}
