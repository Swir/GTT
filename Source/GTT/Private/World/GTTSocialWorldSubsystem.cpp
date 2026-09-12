#include "World/GTTSocialWorldSubsystem.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "NPC/GTTSocialNPC.h"
#include "UObject/UObjectGlobals.h"
#include "World/GTTSocialVenueDoor.h"

namespace
{
    AStaticMeshActor* SpawnInteriorBox(UWorld& World, UStaticMesh* CubeMesh, const FVector& Location, const FVector& Scale)
    {
        AStaticMeshActor* Box = World.SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
        if (!Box || !Box->GetStaticMeshComponent()) return Box;
        Box->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
        Box->GetStaticMeshComponent()->SetWorldScale3D(Scale);
        Box->GetStaticMeshComponent()->SetMobility(EComponentMobility::Static);
        return Box;
    }

    void BuildRoom(UWorld& World, UStaticMesh* CubeMesh, const FVector& Center, const FVector& HalfScale)
    {
        SpawnInteriorBox(World, CubeMesh, Center + FVector(0.0f, 0.0f, -70.0f), FVector(HalfScale.X * 2.0f, HalfScale.Y * 2.0f, 0.15f));
        SpawnInteriorBox(World, CubeMesh, Center + FVector(HalfScale.X * 100.0f, 0.0f, 230.0f), FVector(0.15f, HalfScale.Y * 2.0f, 3.0f));
        SpawnInteriorBox(World, CubeMesh, Center + FVector(-HalfScale.X * 100.0f, 0.0f, 230.0f), FVector(0.15f, HalfScale.Y * 2.0f, 3.0f));
        SpawnInteriorBox(World, CubeMesh, Center + FVector(0.0f, HalfScale.Y * 100.0f, 230.0f), FVector(HalfScale.X * 2.0f, 0.15f, 3.0f));
        SpawnInteriorBox(World, CubeMesh, Center + FVector(0.0f, -HalfScale.Y * 100.0f, 230.0f), FVector(HalfScale.X * 2.0f, 0.15f, 3.0f));
    }
}

void UGTTSocialWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (bBuilt || InWorld.GetNetMode() == NM_Client) return;
    bBuilt = true;

    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh) return;

    const FVector TavernInterior(30000.0f, 30000.0f, 0.0f);
    const FVector HallInterior(34000.0f, 30000.0f, 0.0f);
    BuildRoom(InWorld, CubeMesh, TavernInterior, FVector(6.0f, 4.5f, 1.0f));
    BuildRoom(InWorld, CubeMesh, HallInterior, FVector(8.0f, 6.0f, 1.0f));

    SpawnInteriorBox(InWorld, CubeMesh, TavernInterior + FVector(250.0f, 0.0f, 55.0f), FVector(1.8f, 0.7f, 0.7f));
    SpawnInteriorBox(InWorld, CubeMesh, TavernInterior + FVector(-180.0f, 170.0f, 45.0f), FVector(1.0f, 0.7f, 0.45f));
    SpawnInteriorBox(InWorld, CubeMesh, TavernInterior + FVector(-180.0f, -170.0f, 45.0f), FVector(1.0f, 0.7f, 0.45f));

    SpawnInteriorBox(InWorld, CubeMesh, HallInterior + FVector(0.0f, 260.0f, 35.0f), FVector(3.0f, 0.65f, 0.35f));
    SpawnInteriorBox(InWorld, CubeMesh, HallInterior + FVector(0.0f, -180.0f, 20.0f), FVector(2.6f, 2.2f, 0.18f));

    if (AGTTSocialVenueDoor* TavernDoor = InWorld.SpawnActor<AGTTSocialVenueDoor>(FVector(1450.0f, 3210.0f, 80.0f), FRotator(0.0f, 90.0f, 0.0f)))
        TavernDoor->ConfigureDoor(TEXT("Bent Axle"), TavernInterior + FVector(-420.0f, 0.0f, 20.0f), FRotator::ZeroRotator, false);
    if (AGTTSocialVenueDoor* TavernExit = InWorld.SpawnActor<AGTTSocialVenueDoor>(TavernInterior + FVector(-520.0f, 0.0f, 80.0f), FRotator::ZeroRotator))
        TavernExit->ConfigureDoor(TEXT("Bent Axle"), FVector(1450.0f, 3210.0f, 100.0f), FRotator(0.0f, -90.0f, 0.0f), true);

    if (AGTTSocialVenueDoor* HallDoor = InWorld.SpawnActor<AGTTSocialVenueDoor>(FVector(2400.0f, 2050.0f, 80.0f), FRotator(0.0f, -90.0f, 0.0f)))
        HallDoor->ConfigureDoor(TEXT("Community Hall"), HallInterior + FVector(-610.0f, 0.0f, 20.0f), FRotator::ZeroRotator, false);
    if (AGTTSocialVenueDoor* HallExit = InWorld.SpawnActor<AGTTSocialVenueDoor>(HallInterior + FVector(-720.0f, 0.0f, 80.0f), FRotator::ZeroRotator))
        HallExit->ConfigureDoor(TEXT("Community Hall"), FVector(2400.0f, 2050.0f, 100.0f), FRotator(0.0f, 90.0f, 0.0f), true);

    if (AGTTSocialNPC* Bartender = InWorld.SpawnActor<AGTTSocialNPC>(TavernInterior + FVector(250.0f, 0.0f, 20.0f), FRotator(0.0f, 180.0f, 0.0f)))
        Bartender->ConfigureSocialRole(EGTTSocialRole::Bartender, TEXT("Mara - bartender"));
    if (AGTTSocialNPC* Mechanic = InWorld.SpawnActor<AGTTSocialNPC>(TavernInterior + FVector(-120.0f, 160.0f, 20.0f), FRotator::ZeroRotator))
        Mechanic->ConfigureSocialRole(EGTTSocialRole::MechanicLocal, TEXT("Jory - local mechanic"));
    if (AGTTSocialNPC* Organizer = InWorld.SpawnActor<AGTTSocialNPC>(HallInterior + FVector(260.0f, 220.0f, 20.0f), FRotator(0.0f, 180.0f, 0.0f)))
        Organizer->ConfigureSocialRole(EGTTSocialRole::HallOrganizer, TEXT("Nell - hall organizer"));
    if (AGTTSocialNPC* Farmer = InWorld.SpawnActor<AGTTSocialNPC>(HallInterior + FVector(-100.0f, -190.0f, 20.0f), FRotator::ZeroRotator))
        Farmer->ConfigureSocialRole(EGTTSocialRole::FarmerLocal, TEXT("Oren - hill farmer"));
}
