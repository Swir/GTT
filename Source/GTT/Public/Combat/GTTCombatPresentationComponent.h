#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/GTTCombatTypes.h"
#include "GTTCombatPresentationComponent.generated.h"

class UStaticMeshComponent;

UCLASS(ClassGroup=(GTT), meta=(BlueprintSpawnableComponent))
class GTT_API UGTTCombatPresentationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTTCombatPresentationComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Combat|Presentation") void PlayAttack(EGTTWeaponType WeaponType);
    UFUNCTION(BlueprintCallable, Category="GTT|Combat|Presentation") void RefreshEquippedWeapon();
    UFUNCTION(BlueprintPure, Category="GTT|Combat|Presentation") EGTTWeaponType GetVisualWeapon() const { return VisualWeapon; }

private:
    void EnsureVisualParts();
    void ConfigureWeapon(EGTTWeaponType WeaponType);
    void ResetPose();

    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> PrimaryPart;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> SecondaryPart;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> DetailPart;

    EGTTWeaponType VisualWeapon = EGTTWeaponType::BareHands;
    float AttackTimeRemaining = 0.0f;
    float AttackDuration = 0.28f;
    FVector PrimaryBaseLocation = FVector(38.0f, 34.0f, 34.0f);
    FRotator PrimaryBaseRotation = FRotator(0.0f, 0.0f, -12.0f);
};
