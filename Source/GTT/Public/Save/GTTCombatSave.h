#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTCombatSave.generated.h"

UCLASS()
class GTT_API UGTTCombatSave : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category="GTT|Combat|Save")
    int32 CombatSaveVersion = 1;

    UPROPERTY(VisibleAnywhere, Category="GTT|Combat|Save")
    TArray<uint8> WeaponTypes;

    UPROPERTY(VisibleAnywhere, Category="GTT|Combat|Save")
    uint8 EquippedWeaponType = 0;

    UPROPERTY(VisibleAnywhere, Category="GTT|Combat|Save")
    int32 ShotgunAmmo = 0;
};
