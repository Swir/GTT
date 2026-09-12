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
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Combat") bool AddWeapon(EGTTWeaponType Type, int32 Ammo = 0, bool bAutoEquip = true);
    UFUNCTION(BlueprintCallable, Category="GTT|Combat") void Attack();
    UFUNCTION(BlueprintCallable, Category="GTT|Combat") void CycleWeapon();
    UFUNCTION(BlueprintCallable, Category="GTT|Combat") void DropCurrentWeapon();
    UFUNCTION(BlueprintCallable, Category="GTT|Combat") void ApplyIncomingDamage(float Damage, const FString& SourceLabel);
    UFUNCTION(BlueprintPure, Category="GTT|Combat") EGTTWeaponType GetEquippedWeapon() const { return EquippedWeapon; }
    UFUNCTION(BlueprintPure, Category="GTT|Combat") int32 GetShotgunAmmo() const { return ShotgunAmmo; }
    UFUNCTION(BlueprintPure, Category="GTT|Combat") float GetHealthPercent() const { return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f; }
    UFUNCTION(BlueprintPure, Category="GTT|Combat") FString GetCombatStatusText() const;
    UFUNCTION(BlueprintPure, Category="GTT|Combat") bool HasWeapon(EGTTWeaponType Type) const { return Inventory.Contains(Type); }
    UFUNCTION(BlueprintPure, Category="GTT|Combat|Save") int32 GetStoredWeaponCount() const { return FMath::Max(0, Inventory.Num() - 1); }

    UFUNCTION(BlueprintCallable, Category="GTT|Combat|Save") void SavePersistentLoadout();
    UFUNCTION(BlueprintCallable, Category="GTT|Combat|Save") void LoadPersistentLoadout();

private:
    void PerformMeleeAttack(const FGTTWeaponProfile& Profile);
    void PerformShotgunAttack(const FGTTWeaponProfile& Profile);
    void ApplyHit(AActor* Target, const FVector& HitDirection, const FGTTWeaponProfile& Profile);
    void AddCrimeHeat(float Amount, const FString& Message);
    void HandleDefeat();

    UPROPERTY() TArray<EGTTWeaponType> Inventory;
    EGTTWeaponType EquippedWeapon = EGTTWeaponType::BareHands;
    int32 ShotgunAmmo = 0;
    float AttackCooldownRemaining = 0.0f;
    float MaxHealth = 100.0f;
    float Health = 100.0f;
    FString CombatSaveSlotName = TEXT("GTT_Combat_01");
};
