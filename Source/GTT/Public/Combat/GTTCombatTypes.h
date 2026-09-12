#pragma once

#include "CoreMinimal.h"
#include "GTTCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EGTTWeaponType : uint8
{
    BareHands,
    Pitchfork,
    Axe,
    Branch,
    Rake,
    CowChain,
    Shovel,
    WorkshopWrench,
    FarmShotgun
};

USTRUCT(BlueprintType)
struct FGTTWeaponProfile
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) EGTTWeaponType Type = EGTTWeaponType::BareHands;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;
    UPROPERTY(BlueprintReadOnly) float Damage = 10.0f;
    UPROPERTY(BlueprintReadOnly) float Range = 150.0f;
    UPROPERTY(BlueprintReadOnly) float Cooldown = 0.55f;
    UPROPERTY(BlueprintReadOnly) float Knockback = 180.0f;
    UPROPERTY(BlueprintReadOnly) float PoliceHeat = 5.0f;
    UPROPERTY(BlueprintReadOnly) bool bRanged = false;

    static FGTTWeaponProfile Make(EGTTWeaponType InType);
};
