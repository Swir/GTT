#include "World/GTTPrototypeWorld.h"
#include "Activities/GTTFarmJobTerminal.h"
#include "Activities/GTTFishingSpot.h"
#include "Activities/GTTForestPoachingSpot.h"
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
#include "Vehicles/GTTFarmVanPawn.h"
#include "Vehicles/GTTOldCarPawn.h"
#include "Vehicles/GTTTractorPawn.h"
#include "World/GTTGarageTerminal.h"
#include "World/GTTMissionSafeZone.h"
#include "World/GTTServiceTerminal.h"
#include "World/GTTTuningTerminal.h"

AGTTPrototypeWorld::AGTTPrototypeWorld(){ PrimaryActorTick.bCanEverTick=false; }
void AGTTPrototypeWorld::BeginPlay(){ Super::BeginPlay(); BuildWorld(); }

void AGTTPrototypeWorld::BuildWorld()
{
    if (bWorldBuilt || !GetWorld()) return;
    bWorldBuilt=true;

    ADirectionalLight* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector,FRotator(-48.0f,-35.0f,0.0f));
    if(Sun&&Sun->GetLightComponent()) Sun->GetLightComponent()->SetIntensity(7.5f);
    ASkyLight* Sky=GetWorld()->SpawnActor<ASkyLight>();
    if(Sky&&Sky->GetLightComponent()){ Sky->GetLightComponent()->SetIntensity(1.25f); Sky->GetLightComponent()->SetRealTimeCapture(true); }

    SpawnBox(FVector(0,0,-100),FVector(200,200,1));
    SpawnBox(FVector(0,-1800,-42),FVector(70,6,.08f),FRotator::ZeroRotator,false);
    SpawnBox(FVector(0,1800,-42),FVector(70,6,.08f),FRotator::ZeroRotator,false);
    SpawnBox(FVector(-3300,0,-42),FVector(36,6,.08f),FRotator(0,90,0),false);
    SpawnBox(FVector(3300,0,-42),FVector(36,6,.08f),FRotator(0,90,0),false);
    SpawnLabel(TEXT("LIVE VILLAGE TRAFFIC LOOP"),FVector(0,-1800,170),FRotator(0,180,0),38.0f);

    SpawnBox(FVector(-3000,-900,125),FVector(7,6,3.5f));
    SpawnLabel(TEXT("PLAYER FARM / 4-SLOT GARAGE"),FVector(-3000,-900,520));
    GetWorld()->SpawnActor<AGTTGarageTerminal>(FVector(-2350,-650,55),FRotator::ZeroRotator);
    SpawnLabel(TEXT("GARAGE: REGISTER / RECALL NEXT - E"),FVector(-2350,-650,200),FRotator(0,180,0),42.0f);
    for(int32 Slot=0; Slot<4; ++Slot)
    {
        SpawnBox(FVector(-3650.0f + Slot*420.0f,-1500.0f,-35.0f),FVector(3.6f,1.8f,.05f),FRotator::ZeroRotator,false);
        SpawnLabel(FString::Printf(TEXT("GARAGE %d"),Slot+1),FVector(-3650.0f + Slot*420.0f,-1500.0f,65.0f),FRotator(0,180,0),30.0f);
    }

    if(AGTTFarmJobTerminal* JobStart=GetWorld()->SpawnActor<AGTTFarmJobTerminal>(FVector(-2450,-1150,55),FRotator::ZeroRotator)) JobStart->SetTerminalType(EGTTFarmJobTerminalType::Start);
    SpawnLabel(TEXT("LEGAL FARM JOB START - E"),FVector(-2450,-1150,200),FRotator(0,180,0),46.0f);

    SpawnBox(FVector(-3600,900,180),FVector(8,7,4.5f));
    SpawnLabel(TEXT("BARN - MISSION GOAL"),FVector(-3500,300,420));
    GetWorld()->SpawnActor<AGTTMissionSafeZone>(FVector(-3500,250,140),FRotator::ZeroRotator);

    SpawnBox(FVector(2850,700,125),FVector(7,6,3.5f));
    SpawnBox(FVector(3650,1050,110),FVector(5,8,3));
    SpawnLabel(TEXT("NEIGHBOUR FARM"),FVector(3050,700,520));
    GetWorld()->SpawnActor<AGTTTractorPawn>(FVector(2500,250,160),FRotator(0,180,0));
    SpawnLabel(TEXT("RUSTY FIELDMASTER 60"),FVector(2500,250,440),FRotator(0,180,0),65.0f);

    GetWorld()->SpawnActor<AGTTOldCarPawn>(FVector(1450,-2450,90),FRotator(0,90,0));
    SpawnLabel(TEXT("RATTLEBACK 82 - OLD CAR"),FVector(1450,-2450,300),FRotator(0,180,0),48.0f);

    SpawnBox(FVector(900,-2700,150),FVector(6,5,4));
    SpawnLabel(TEXT("VILLAGE SHOP / FISH BUYER"),FVector(900,-2700,560));
    if(AGTTServiceTerminal* Buyer=GetWorld()->SpawnActor<AGTTServiceTerminal>(FVector(900,-2200,55),FRotator::ZeroRotator)) Buyer->SetServiceType(EGTTServiceType::FishBuyer);
    SpawnLabel(TEXT("SELL FISH - E"),FVector(900,-2200,190),FRotator(0,180,0),50.0f);

    SpawnBox(FVector(2700,-2600,165),FVector(7,5,4.3f));
    SpawnLabel(TEXT("POLICE / ARREST RELEASE"),FVector(2700,-2600,590));
    SpawnBox(FVector(2400,2700,155),FVector(8,6,4.1f));
    SpawnLabel(TEXT("COMMUNITY HALL"),FVector(2400,2700,580));

    SpawnBox(FVector(-400,2900,140),FVector(7,5,3.8f));
    SpawnLabel(TEXT("WORKSHOP + TUNING"),FVector(-400,2900,550));
    if(AGTTServiceTerminal* Workshop=GetWorld()->SpawnActor<AGTTServiceTerminal>(FVector(-650,2400,55),FRotator::ZeroRotator)) Workshop->SetServiceType(EGTTServiceType::Workshop);
    SpawnLabel(TEXT("REPAIR + REFUEL $75 - E"),FVector(-650,2400,190),FRotator(0,180,0),42.0f);
    GetWorld()->SpawnActor<AGTTTuningTerminal>(FVector(-150,2400,55),FRotator::ZeroRotator);
    SpawnLabel(TEXT("TUNING / TIRES - E"),FVector(-150,2400,190),FRotator(0,180,0),44.0f);
    GetWorld()->SpawnActor<AGTTFarmVanPawn>(FVector(450,2850,110),FRotator(0,-90,0));
    SpawnLabel(TEXT("MULEBOX 1200 - FARM VAN"),FVector(450,2850,340),FRotator(0,180,0),48.0f);

    SpawnBox(FVector(4700,-500,-35),FVector(20,28,.12f),FRotator::ZeroRotator,false);
    SpawnLabel(TEXT("PRIVATE LAKE - NO FISHING"),FVector(4700,-500,220));
    GetWorld()->SpawnActor<AGTTFishingSpot>(FVector(4050,-500,40),FRotator::ZeroRotator);
    SpawnLabel(TEXT("POACH FISH - E"),FVector(4050,-500,175),FRotator(0,180,0),52.0f);

    SpawnBox(FVector(5050,700,115),FVector(5.5f,4.5f,3.2f));
    SpawnLabel(TEXT("GAME WARDEN OUTPOST"),FVector(5050,700,470),FRotator(0,180,0),46.0f);
    SpawnLabel(TEXT("POACHING TRIGGERS WARDEN ALERT 1-3"),FVector(4600,350,260),FRotator(0,180,0),38.0f);

    // First forest expansion: a rough woodland patch east of the lake with an illegal poaching interaction.
    for(int32 Tree=0; Tree<18; ++Tree)
    {
        const float X = 6100.0f + (Tree%6)*420.0f;
        const float Y = -1750.0f + (Tree/6)*620.0f;
        SpawnBox(FVector(X,Y,120),FVector(.45f,.45f,3.2f));
        SpawnBox(FVector(X,Y,410),FVector(1.4f,1.4f,1.2f),FRotator::ZeroRotator,false);
    }
    SpawnLabel(TEXT("WARDEN FOREST / NO HUNTING"),FVector(7050,-1700,540),FRotator(0,180,0),52.0f);
    GetWorld()->SpawnActor<AGTTForestPoachingSpot>(FVector(7050,-950,55),FRotator::ZeroRotator);
    SpawnLabel(TEXT("ILLEGAL FOREST POACHING - E"),FVector(7050,-950,210),FRotator(0,180,0),44.0f);

    SpawnBox(FVector(5200,2550,20),FVector(20,12,.2f),FRotator::ZeroRotator,false);
    SpawnLabel(TEXT("FIELD DELIVERY / LEGAL JOB"),FVector(5200,2550,260));
    if(AGTTFarmJobTerminal* JobFinish=GetWorld()->SpawnActor<AGTTFarmJobTerminal>(FVector(4850,2550,55),FRotator::ZeroRotator)) JobFinish->SetTerminalType(EGTTFarmJobTerminalType::Finish);
    SpawnLabel(TEXT("FINISH FARM JOB - E"),FVector(4850,2550,200),FRotator(0,180,0),48.0f);

    const TArray<FVector> CitizenSpawns={FVector(2200,450,120),FVector(3150,350,120),FVector(850,-2150,120),FVector(1650,-1650,120),FVector(-250,2150,120),FVector(2100,2200,120),FVector(-1900,-900,120),FVector(3400,-1500,120)};
    for(const FVector& P:CitizenSpawns) GetWorld()->SpawnActor<AGTTCitizenPawn>(P,FRotator::ZeroRotator);
    SpawnLabel(TEXT("VILLAGERS: WORK DAY / SOCIAL EVENING / HOME NIGHT"),FVector(1450,-1350,320),FRotator(0,180,0),44.0f);

    for(int32 I=0;I<9;++I) SpawnBox(FVector(-2200+I*520,-650,35),FVector(4.2f,.18f,.85f));
    for(int32 I=0;I<7;++I) SpawnBox(FVector(1450,-900+I*420,35),FVector(.18f,3.5f,.85f));

    SpawnLabel(TEXT("GTT 0.0.9 | GARAGE RECALL + TUNING + FOREST POACHING"),FVector(-2500,-1250,380),FRotator(0,180,0),46.0f);
    if(APawn* PlayerPawn=UGameplayStatics::GetPlayerPawn(this,0)){ PlayerPawn->SetActorLocation(FVector(-2550,-1250,120)); PlayerPawn->SetActorRotation(FRotator(0,25,0)); }
}

AStaticMeshActor* AGTTPrototypeWorld::SpawnBox(const FVector& Location,const FVector& Scale,const FRotator& Rotation,bool bCollision)
{
    if(!GetWorld()) return nullptr;
    UStaticMesh* CubeMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    if(!CubeMesh) return nullptr;
    AStaticMeshActor* Prop=GetWorld()->SpawnActor<AStaticMeshActor>(Location,Rotation);
    if(!Prop) return nullptr;
    UStaticMeshComponent* Mesh=Prop->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable); Mesh->SetStaticMesh(CubeMesh); Mesh->SetCollisionEnabled(bCollision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision); Prop->SetActorScale3D(Scale); return Prop;
}

ATextRenderActor* AGTTPrototypeWorld::SpawnLabel(const FString& Text,const FVector& Location,const FRotator& Rotation,float WorldSize)
{
    if(!GetWorld()) return nullptr;
    ATextRenderActor* Label=GetWorld()->SpawnActor<ATextRenderActor>(Location,Rotation);
    if(!Label||!Label->GetTextRender()) return Label;
    UTextRenderComponent* T=Label->GetTextRender(); T->SetText(FText::FromString(Text)); T->SetWorldSize(WorldSize); T->SetHorizontalAlignment(EHTA_Center); T->SetTextRenderColor(FColor(255,220,80)); T->SetCastShadow(true); return Label;
}
