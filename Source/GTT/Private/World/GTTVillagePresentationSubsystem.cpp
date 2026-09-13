#include "World/GTTVillagePresentationSubsystem.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTPrototypeWorld.h"

bool UGTTVillagePresentationSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGTTVillagePresentationSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    bool bHasPrototypeWorld = false;
    for (TActorIterator<AGTTPrototypeWorld> It(&InWorld); It; ++It) { bHasPrototypeWorld = true; break; }
    if (!bHasPrototypeWorld) return;
    BuildPresentation();
    InWorld.GetTimerManager().SetTimer(RefreshTimer, this, &UGTTVillagePresentationSubsystem::RefreshPresentation, 0.5f, true);
    RefreshPresentation();
}

void UGTTVillagePresentationSubsystem::BuildPresentation()
{
    if (bPresentationBuilt || !GetWorld()) return;
    bPresentationBuilt = true;
    const TArray<FVector> LampPositions = {
        FVector(-2700,-1800,0), FVector(-1350,-1800,0), FVector(0,-1800,0), FVector(1350,-1800,0), FVector(2700,-1800,0),
        FVector(-2700,1800,0), FVector(-1350,1800,0), FVector(0,1800,0), FVector(1350,1800,0), FVector(2700,1800,0),
        FVector(-3300,-900,0), FVector(-3300,900,0), FVector(3300,-900,0), FVector(3300,900,0)
    };
    for (const FVector& Base : LampPositions)
    {
        SpawnProp(Base + FVector(0,0,180), FVector(0.10f,0.10f,3.6f), FRotator::ZeroRotator, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
        SpawnProp(Base + FVector(0,0,540), FVector(0.42f,0.28f,0.18f), FRotator::ZeroRotator, TEXT("/Engine/BasicShapes/Cube.Cube"));
        SpawnStreetLight(Base + FVector(0,0,515));
    }
    for (int32 I=-5; I<=5; ++I)
    {
        const float X=I*560.0f;
        SpawnProp(FVector(X,-1660,45), FVector(0.08f,0.08f,0.9f), FRotator::ZeroRotator, TEXT("/Engine/BasicShapes/Cube.Cube"));
        SpawnProp(FVector(X,1660,45), FVector(0.08f,0.08f,0.9f), FRotator::ZeroRotator, TEXT("/Engine/BasicShapes/Cube.Cube"));
    }
    for (int32 I=-2; I<=2; ++I)
    {
        const float Y=I*560.0f;
        SpawnProp(FVector(-3160,Y,45), FVector(0.08f,0.08f,0.9f), FRotator::ZeroRotator, TEXT("/Engine/BasicShapes/Cube.Cube"));
        SpawnProp(FVector(3160,Y,45), FVector(0.08f,0.08f,0.9f), FRotator::ZeroRotator, TEXT("/Engine/BasicShapes/Cube.Cube"));
    }
    for (int32 Bale=0; Bale<8; ++Bale)
    {
        const float X=5000.0f+(Bale%4)*190.0f, Y=3350.0f+(Bale/4)*220.0f;
        SpawnProp(FVector(X,Y,70), FVector(0.72f,0.72f,0.72f), FRotator(90,0,0), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    }
    for (int32 Log=0; Log<10; ++Log)
    {
        const float X=7550.0f+(Log%5)*180.0f, Y=1120.0f+(Log/5)*150.0f;
        SpawnProp(FVector(X,Y,65), FVector(0.25f,0.25f,1.7f), FRotator(0,90,0), TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    }
}

AStaticMeshActor* UGTTVillagePresentationSubsystem::SpawnProp(const FVector& Location,const FVector& Scale,const FRotator& Rotation,const TCHAR* MeshPath)
{
    if (!GetWorld()) return nullptr;
    UStaticMesh* MeshAsset=LoadObject<UStaticMesh>(nullptr,MeshPath);
    if (!MeshAsset) return nullptr;
    AStaticMeshActor* Prop=GetWorld()->SpawnActor<AStaticMeshActor>(Location,Rotation);
    if (!Prop || !Prop->GetStaticMeshComponent()) return Prop;
    UStaticMeshComponent* Mesh=Prop->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable); Mesh->SetStaticMesh(MeshAsset); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Prop->SetActorScale3D(Scale); PresentationProps.Add(Prop); return Prop;
}

APointLight* UGTTVillagePresentationSubsystem::SpawnStreetLight(const FVector& Location)
{
    if (!GetWorld()) return nullptr;
    APointLight* Lamp=GetWorld()->SpawnActor<APointLight>(Location,FRotator::ZeroRotator);
    if (!Lamp || !Lamp->GetLightComponent()) return Lamp;
    UPointLightComponent* Light=Lamp->GetPointLightComponent();
    Light->SetIntensity(4200.0f); Light->SetAttenuationRadius(850.0f); Light->SetLightColor(FLinearColor(1.0f,0.68f,0.34f)); Light->SetCastShadows(false);
    StreetLights.Add(Lamp); return Lamp;
}

void UGTTVillagePresentationSubsystem::RefreshPresentation()
{
    UWorld* World=GetWorld(); if (!World) return;
    bool bNight=false;
    for (TActorIterator<AGTTDayNightCycle> It(World); It; ++It) { bNight=It->IsNight(); break; }
    for (APointLight* Lamp:StreetLights) if (IsValid(Lamp) && Lamp->GetLightComponent()) Lamp->GetLightComponent()->SetVisibility(bNight);
    APawn* Player=UGameplayStatics::GetPlayerPawn(World,0); if (!Player) return;
    const FVector PlayerLocation=Player->GetActorLocation(); constexpr float LabelVisibleDistance=1900.0f; const float LabelVisibleDistanceSq=FMath::Square(LabelVisibleDistance);
    for (TActorIterator<ATextRenderActor> It(World); It; ++It)
    {
        if (UTextRenderComponent* Text=It->GetTextRender())
        {
            const FString CurrentText=Text->GetText().ToString();
            if (CurrentText.StartsWith(TEXT("GTT 0.0.12 |")) || CurrentText.StartsWith(TEXT("GTT 0.0.37 |")))
                Text->SetText(FText::FromString(TEXT("GTT 0.0.38 | LIVING VILLAGE SANDBOX")));
            const bool bNearby=FVector::DistSquared(PlayerLocation,It->GetActorLocation())<=LabelVisibleDistanceSq;
            Text->SetVisibility(bNearby,true);
        }
    }
}
