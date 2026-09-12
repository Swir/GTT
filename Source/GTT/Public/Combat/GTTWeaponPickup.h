#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "Combat/GTTCombatTypes.h"
#include "GTTWeaponPickup.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class GTT_API AGTTWeaponPickup : public AActor, public IGTTInteractable
{
    GENERATED_BODY()
public:
    AGTTWeaponPickup();
    void Configure(EGTTWeaponType InType, int32 InAmmo = 0);
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
private:
    void RefreshVisuals();
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Label;
    EGTTWeaponType WeaponType = EGTTWeaponType::Branch;
    int32 Ammo = 0;
};
