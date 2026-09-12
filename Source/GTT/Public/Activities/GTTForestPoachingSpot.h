#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTForestPoachingSpot.generated.h"

class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTForestPoachingSpot : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTForestPoachingSpot();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Poaching")
    TObjectPtr<UStaticMeshComponent> MarkerMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Poaching", meta=(ClampMin="1.0"))
    float AttemptCooldownSeconds = 7.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Poaching", meta=(ClampMin="1.0"))
    float WildlifeHeatPerAttempt = 31.0f;

private:
    double NextAllowedAttemptTime = 0.0;
};
