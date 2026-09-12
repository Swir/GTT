#include "World/GTTArc4WorldSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Missions/GTTArc4Director.h"
#include "Missions/GTTArc4Terminal.h"

namespace
{
AStaticMeshActor* SpawnBox(UWorld& World, const FVector& Location, const FVector& Scale)
{
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cube) return nullptr;
    AStaticMeshActor* Box = World.SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
    if (!Box) return nullptr;
    Box->GetStaticMeshComponent()->SetStaticMesh(Cube);
    Box->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetActorScale3D(Scale);
    return Box;
}
}

void UGTTArc4WorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!InWorld.IsGameWorld()) return;

    InWorld.SpawnActor<AGTTArc4Director>();
    if (AGTTArc4Terminal* Farm = InWorld.SpawnActor<AGTTArc4Terminal>(FVector(-2780,-520,55), FRotator::ZeroRotator))
        Farm->SetTerminalType(EGTTArc4TerminalType::FarmOffice);
    if (AGTTArc4Terminal* Pass = InWorld.SpawnActor<AGTTArc4Terminal>(FVector(10150,2550,70), FRotator::ZeroRotator))
        Pass->SetTerminalType(EGTTArc4TerminalType::NorthPass);
    if (AGTTArc4Terminal* Ridge = InWorld.SpawnActor<AGTTArc4Terminal>(FVector(11200,800,70), FRotator::ZeroRotator))
        Ridge->SetTerminalType(EGTTArc4TerminalType::RidgeExchange);

    // Distinct landmarks for the expanded north-east countryside.
    SpawnBox(InWorld, FVector(10150,2550,180), FVector(9.0f,1.2f,2.4f));
    SpawnBox(InWorld, FVector(11200,800,170), FVector(7.5f,5.0f,3.2f));
    SpawnBox(InWorld, FVector(9600,1200,100), FVector(2.5f,11.0f,0.35f));
    SpawnBox(InWorld, FVector(8850,800,120), FVector(4.0f,4.0f,1.8f));
}
