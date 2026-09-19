#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTWorkshopJobBoardTerminal.generated.h"

class UStaticMeshComponent;

/**
 * Player-facing workshop appointment board.
 *
 * This terminal deliberately owns presentation and safe appointment cancellation only. It never
 * charges cash and never mutates vehicle repair state; the authoritative workshop queue remains
 * responsible for check-in, timed service, payment, repair/refuel and persistence.
 */
UCLASS()
class GTT_API AGTTWorkshopJobBoardTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTWorkshopJobBoardTerminal();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Workshop|JobBoard")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Workshop|JobBoard", meta=(ClampMin="100.0"))
    float NearbyVehicleRadius = 450.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Workshop|JobBoard", meta=(ClampMin="2.0", ClampMax="15.0"))
    float CancelConfirmationSeconds = 6.0f;

private:
    FString BuildBoardSummary() const;
    FName FindNearestQueuedVehicle() const;

    FName PendingCancelVehicleId = NAME_None;
    float PendingCancelExpiresAt = -1.0f;
};
