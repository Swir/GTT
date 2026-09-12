#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/GTTCombatTypes.h"
#include "GTTCombatComponent.generated.h"

UCLASS(ClassGroup=(GTT), meta=(BlueprintSpawnableComponent))
class GTT_API UGTTCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTTCombatComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Combat") bool AddWeapon(EGTTWeaponType Type, int32 Ammo = 0, bool bAutoEquip = true);
    UFUNCTION(BlueprintCallable, Category="GTT|Combat") void Attack();
    UFUNCTION(BlueprintCallable, Category="GTT|Combat") void CycleWeapon();
    UFUNCTION(BlueprintCallable, Category="GTT|Combat") void DropCurrentWeapon();
    UFUNCTION(BlueprintPure, Category="GTT|Combat") EGTTWeaponType GetEquippedWeapon() const { return EquippedWeapon; }
    UFUNCTION(BlueprintPure, Category="GTT|Combat") int32 GetShotgunAmmo() const { return ShotgunAmmo; }
    UFUNCTION(BlueprintPure, Category="GTT|Combat") FString GetCombatStatusText() const;
    UFUNCTION(BlueprintPure, Category="GTT|Combat") bool HasWeapon(EGTTWeaponType Type) const { return Inventory.Contains(Type); }

private:
    void PerformMeleeAttack(const FGTTWeaponProfile& Profile);
    void PerformShotgunAttack(const FGTTWeaponProfile& Profile);
    void ApplyHit(AActor* Target, const FVector& HitDirection, const FGTTWeaponProfile& Profile);
    void AddCrimeHeat(float Amount, const FString& Message);

    UPROPERTY() TArray<EGTTWeaponType> Inventory;
    EGTTWeaponType EquippedWeapon = EGTTWeaponType::BareHands;
    int32 ShotgunAmmo = 0;
    float AttackCooldownRemaining = 0.0f;
};
