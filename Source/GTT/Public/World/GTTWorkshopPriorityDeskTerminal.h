#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTWorkshopPriorityDeskTerminal.generated.h"

class UStaticMeshComponent;

/**
 * Deliberate quote-changing counter for workshop priority upgrades.
 *
 * The desk never spends cash or repairs vehicles. It requires two exact-ID interactions before
 * delegating a STANDARD -> URGENT promotion to the authoritative workshop queue, where the new
 * locked quote, appointment order and shortened service duration are persisted atomically.
 */
UCLASS()
class GTT_API AGTTWorkshopPriorityDeskTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTWorkshopPriorityDeskTerminal();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Workshop|Priority")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Workshop|Priority", meta=(ClampMin="100.0"))
    float NearbyVehicleRadius = 450.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Workshop|Priority", meta=(ClampMin="2.0", ClampMax="15.0"))
    float ConfirmationSeconds = 6.0f;

private:
    FName FindNearestUpgradeableVehicle() const;

    FName PendingVehicleId = NAME_None;
    float PendingExpiresAt = -1.0f;
};
