#include "World/GTTPrototypeWorld.h"

#include "Activities/GTTFishingSpot.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NPC/GTTCitizenPawn.h"
#include "Vehicles/GTTTractorPawn.h"
#include "World/GTTMissionSafeZone.h"
#include "World/GTTServiceTerminal.h"

AGTTPrototypeWorld::AGTTPrototypeWorld()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGTTPrototypeWorld::BeginPlay()
{
    Super::BeginPlay();
    BuildWorld();
}

void AGTTPrototypeWorld::BuildWorld()
{
    if (bWorldBuilt || !GetWorld())
    {
        return;
    }

    bWorldBuilt = true;

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-48.0f, -35.0f, 0.0f));
    if (Sun && Sun->GetLightComponent())
    {
        Sun->GetLightComponent()->SetIntensity(7.5f);
    }

    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky && Sky->GetLightComponent())
    {
        Sky->GetLightComponent()->SetIntensity(1.25f);
        Sky->GetLightComponent()->SetRealTimeCapture(true);
    }

    // Ground and a simple road loop.
    SpawnBox(FVector(0.0f, 0.0f, -100.0f), FVector(200.0f, 200.0f, 1.0f));
    SpawnBox(FVector(0.0f, -1800.0f, -42.0f), FVector(70.0f, 6.0f, 0.08f), FRotator::ZeroRotator, false);
    SpawnBox(FVector(0.0f, 1800.0f, -42.0f), FVector(70.0f, 6.0f, 0.08f), FRotator::ZeroRotator, false);
    SpawnBox(FVector(-3300.0f, 0.0f, -42.0f), FVector(36.0f, 6.0f, 0.08f), FRotator(0.0f, 90.0f, 0.0f), false);
    SpawnBox(FVector(3300.0f, 0.0f, -42.0f), FVector(36.0f, 6.0f, 0.08f), FRotator(0.0f, 90.0f, 0.0f), false);

    // Player farm / starting area.
    SpawnBox(FVector(-3000.0f, -900.0f, 125.0f), FVector(7.0f, 6.0f, 3.5f));
    SpawnLabel(TEXT("PLAYER FARM"), FVector(-3000.0f, -900.0f, 520.0f));

    // Barn and mission return zone.
    SpawnBox(FVector(-3600.0f, 900.0f, 180.0f), FVector(8.0f, 7.0f, 4.5f));
    SpawnLabel(TEXT("BARN - MISSION GOAL"), FVector(-3500.0f, 300.0f, 420.0f));

    AGTTMissionSafeZone* SafeZone = GetWorld()->SpawnActor<AGTTMissionSafeZone>(
        FVector(-3500.0f, 250.0f, 140.0f),
        FRotator::ZeroRotator);
    if (SafeZone)
    {
        SafeZone->SetActorScale3D(FVector(1.0f));
    }

    // Neighbour farm where the first stolen tractor waits.
    SpawnBox(FVector(2850.0f, 700.0f, 125.0f), FVector(7.0f, 6.0f, 3.5f));
    SpawnBox(FVector(3650.0f, 1050.0f, 110.0f), FVector(5.0f, 8.0f, 3.0f));
    SpawnLabel(TEXT("NEIGHBOUR FARM"), FVector(3050.0f, 700.0f, 520.0f));

    GetWorld()->SpawnActor<AGTTTractorPawn>(
        FVector(2500.0f, 250.0f, 160.0f),
        FRotator(0.0f, 180.0f, 0.0f));
    SpawnLabel(TEXT("RUSTY FIELDMASTER 60"), FVector(2500.0f, 250.0f, 440.0f), FRotator(0.0f, 180.0f, 0.0f), 65.0f);

    // Village shop doubles as the prototype fish buyer.
    SpawnBox(FVector(900.0f, -2700.0f, 150.0f), FVector(6.0f, 5.0f, 4.0f));
    SpawnLabel(TEXT("VILLAGE SHOP / FISH BUYER"), FVector(900.0f, -2700.0f, 560.0f));
    if (AGTTServiceTerminal* FishBuyer = GetWorld()->SpawnActor<AGTTServiceTerminal>(
        FVector(900.0f, -2200.0f, 55.0f), FRotator::ZeroRotator))
    {
        FishBuyer->SetServiceType(EGTTServiceType::FishBuyer);
    }
    SpawnLabel(TEXT("SELL FISH - E"), FVector(900.0f, -2200.0f, 190.0f), FRotator(0.0f, 180.0f, 0.0f), 50.0f);

    SpawnBox(FVector(2700.0f, -2600.0f, 165.0f), FVector(7.0f, 5.0f, 4.3f));
    SpawnLabel(TEXT("POLICE"), FVector(2700.0f, -2600.0f, 590.0f));

    SpawnBox(FVector(2400.0f, 2700.0f, 155.0f), FVector(8.0f, 6.0f, 4.1f));
    SpawnLabel(TEXT("COMMUNITY HALL"), FVector(2400.0f, 2700.0f, 580.0f));

    // Workshop terminal repairs and refuels the nearest parked vehicle.
    SpawnBox(FVector(-400.0f, 2900.0f, 140.0f), FVector(7.0f, 5.0f, 3.8f));
    SpawnLabel(TEXT("WORKSHOP"), FVector(-400.0f, 2900.0f, 550.0f));
    if (AGTTServiceTerminal* Workshop = GetWorld()->SpawnActor<AGTTServiceTerminal>(
        FVector(-400.0f, 2400.0f, 55.0f), FRotator::ZeroRotator))
    {
        Workshop->SetServiceType(EGTTServiceType::Workshop);
    }
    SpawnLabel(TEXT("REPAIR + REFUEL $75 - E"), FVector(-400.0f, 2400.0f, 190.0f), FRotator(0.0f, 180.0f, 0.0f), 48.0f);

    // Lake and illegal fishing interaction.
    SpawnBox(FVector(4700.0f, -500.0f, -35.0f), FVector(20.0f, 28.0f, 0.12f), FRotator::ZeroRotator, false);
    SpawnLabel(TEXT("PRIVATE LAKE - NO FISHING"), FVector(4700.0f, -500.0f, 220.0f));
    GetWorld()->SpawnActor<AGTTFishingSpot>(FVector(4050.0f, -500.0f, 40.0f), FRotator::ZeroRotator);
    SpawnLabel(TEXT("POACH FISH - E"), FVector(4050.0f, -500.0f, 175.0f), FRotator(0.0f, 180.0f, 0.0f), 52.0f);

    // Citizens: simple wandering prototype NPCs. Their location matters because they can witness theft.
    const TArray<FVector> CitizenSpawns = {
        FVector(2200.0f, 450.0f, 120.0f),
        FVector(3150.0f, 350.0f, 120.0f),
        FVector(850.0f, -2150.0f, 120.0f),
        FVector(1650.0f, -1650.0f, 120.0f),
        FVector(-250.0f, 2150.0f, 120.0f),
        FVector(2100.0f, 2200.0f, 120.0f),
        FVector(-1900.0f, -900.0f, 120.0f),
        FVector(3400.0f, -1500.0f, 120.0f)
    };

    for (const FVector& SpawnLocation : CitizenSpawns)
    {
        GetWorld()->SpawnActor<AGTTCitizenPawn>(SpawnLocation, FRotator::ZeroRotator);
    }
    SpawnLabel(TEXT("VILLAGERS CAN WITNESS CRIME"), FVector(1450.0f, -1350.0f, 320.0f), FRotator(0.0f, 180.0f, 0.0f), 48.0f);

    // A few fence / obstacle lines to create shortcuts and crash opportunities.
    for (int32 Index = 0; Index < 9; ++Index)
    {
        const float X = -2200.0f + Index * 520.0f;
        SpawnBox(FVector(X, -650.0f, 35.0f), FVector(4.2f, 0.18f, 0.85f));
    }

    for (int32 Index = 0; Index < 7; ++Index)
    {
        const float Y = -900.0f + Index * 420.0f;
        SpawnBox(FVector(1450.0f, Y, 35.0f), FVector(0.18f, 3.5f, 0.85f));
    }

    SpawnLabel(
        TEXT("GTT 0.0.4  |  STEAL - ESCAPE - FISH - SELL - REPAIR - REPEAT"),
        FVector(-2500.0f, -1250.0f, 380.0f),
        FRotator(0.0f, 180.0f, 0.0f),
        54.0f);

    if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        PlayerPawn->SetActorLocation(FVector(-2550.0f, -1250.0f, 120.0f));
        PlayerPawn->SetActorRotation(FRotator(0.0f, 25.0f, 0.0f));
    }
}

AStaticMeshActor* AGTTPrototypeWorld::SpawnBox(
    const FVector& Location,
    const FVector& Scale,
    const FRotator& Rotation,
    bool bCollision)
{
    if (!GetWorld())
    {
        return nullptr;
    }

    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh)
    {
        return nullptr;
    }

    AStaticMeshActor* Prop = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (!Prop)
    {
        return nullptr;
    }

    UStaticMeshComponent* MeshComponent = Prop->GetStaticMeshComponent();
    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetStaticMesh(CubeMesh);
    MeshComponent->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Prop->SetActorScale3D(Scale);
    return Prop;
}

ATextRenderActor* AGTTPrototypeWorld::SpawnLabel(
    const FString& Text,
    const FVector& Location,
    const FRotator& Rotation,
    float WorldSize)
{
    if (!GetWorld())
    {
        return nullptr;
    }

    ATextRenderActor* Label = GetWorld()->SpawnActor<ATextRenderActor>(Location, Rotation);
    if (!Label || !Label->GetTextRender())
    {
        return Label;
    }

    UTextRenderComponent* TextComponent = Label->GetTextRender();
    TextComponent->SetText(FText::FromString(Text));
    TextComponent->SetWorldSize(WorldSize);
    TextComponent->SetHorizontalAlignment(EHTA_Center);
    TextComponent->SetTextRenderColor(FColor(255, 220, 80));
    TextComponent->SetCastShadow(true);
    return Label;
}
