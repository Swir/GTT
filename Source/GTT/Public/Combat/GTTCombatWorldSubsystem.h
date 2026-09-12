#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTCombatWorldSubsystem.generated.h"

UCLASS()
class GTT_API UGTTCombatWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
private:
    void SpawnPickup(UWorld& World, const FVector& Location, uint8 WeaponTypeValue, int32 Ammo = 0) const;
};
