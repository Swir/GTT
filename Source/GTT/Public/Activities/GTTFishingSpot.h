#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTFishingSpot.generated.h"

class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTFishingSpot : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTFishingSpot();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Fishing")
    TObjectPtr<UStaticMeshComponent> MarkerMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GTT|Fishing")
    bool bRestrictedFishing = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Fishing", meta=(ClampMin="0.5"))
    float CastCooldownSeconds = 3.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Fishing|Crime", meta=(ClampMin="0.0"))
    float RestrictedFishingHeat = 5.0f;

private:
    double NextAllowedCastTime = 0.0;
};
