#include "World/GTTArc3WorldSubsystem.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Missions/GTTArc3Director.h"
#include "Missions/GTTArc3Terminal.h"

namespace
{
AStaticMeshActor* SpawnArc3Box(UWorld& World, const FVector& Location, const FVector& Scale)
{
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh) return nullptr;
    AStaticMeshActor* Box = World.SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
    if (!Box) return nullptr;
    Box->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
    Box->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetActorScale3D(Scale);
    return Box;
}
}

void UGTTArc3WorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (!InWorld.IsGameWorld()) return;

    InWorld.SpawnActor<AGTTArc3Director>();

    if (AGTTArc3Terminal* Farm = InWorld.SpawnActor<AGTTArc3Terminal>(FVector(-2860.0f, -450.0f, 55.0f), FRotator::ZeroRotator))
        Farm->SetTerminalType(EGTTArc3TerminalType::FarmOffice);

    if (AGTTArc3Terminal* Barn = InWorld.SpawnActor<AGTTArc3Terminal>(FVector(-5200.0f, 1800.0f, 55.0f), FRotator::ZeroRotator))
        Barn->SetTerminalType(EGTTArc3TerminalType::RedBarn);

    if (AGTTArc3Terminal* County = InWorld.SpawnActor<AGTTArc3Terminal>(FVector(-4700.0f, -2500.0f, 55.0f), FRotator::ZeroRotator))
        County->SetTerminalType(EGTTArc3TerminalType::CountyDrop);

    // West-side expansion: a distinct red-barn compound and county evidence shed.
    SpawnArc3Box(InWorld, FVector(-5200.0f, 1800.0f, 180.0f), FVector(8.5f, 6.0f, 3.6f));
    SpawnArc3Box(InWorld, FVector(-5650.0f, 1500.0f, 80.0f), FVector(2.2f, 8.0f, 1.4f));
    SpawnArc3Box(InWorld, FVector(-4700.0f, -2500.0f, 145.0f), FVector(6.0f, 4.5f, 2.8f));
}
