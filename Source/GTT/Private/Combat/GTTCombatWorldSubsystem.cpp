#include "Combat/GTTCombatWorldSubsystem.h"

#include "Activities/GTTBrawlDirector.h"
#include "Activities/GTTBrawlTerminal.h"
#include "Combat/GTTCombatTypes.h"
#include "Combat/GTTWeaponPickup.h"
#include "Engine/World.h"

void UGTTCombatWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    SpawnPickup(InWorld, FVector(-3300,-1050,80), (uint8)EGTTWeaponType::Pitchfork);
    SpawnPickup(InWorld, FVector(-3550,650,80), (uint8)EGTTWeaponType::Rake);
    SpawnPickup(InWorld, FVector(-450,2500,80), (uint8)EGTTWeaponType::WorkshopWrench);
    SpawnPickup(InWorld, FVector(7350,-1050,80), (uint8)EGTTWeaponType::Axe);
    SpawnPickup(InWorld, FVector(6500,-1350,80), (uint8)EGTTWeaponType::Branch);
    SpawnPickup(InWorld, FVector(5650,3050,80), (uint8)EGTTWeaponType::Shovel);
    SpawnPickup(InWorld, FVector(2700,850,80), (uint8)EGTTWeaponType::CowChain);
    SpawnPickup(InWorld, FVector(3650,1200,80), (uint8)EGTTWeaponType::FarmShotgun, 6);

    InWorld.SpawnActor<AGTTBrawlDirector>();
    InWorld.SpawnActor<AGTTBrawlTerminal>(FVector(1450,2380,65),FRotator::ZeroRotator);
}

void UGTTCombatWorldSubsystem::SpawnPickup(UWorld& World, const FVector& Location, uint8 WeaponTypeValue, int32 Ammo) const
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if (AGTTWeaponPickup* Pickup = World.SpawnActor<AGTTWeaponPickup>(Location, FRotator::ZeroRotator, Params))
    {
        Pickup->Configure((EGTTWeaponType)WeaponTypeValue, Ammo);
    }
}
