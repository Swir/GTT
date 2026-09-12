#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTTuningTerminal.generated.h"

class AGTTVehicleBase;
class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTTuningTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTTuningTerminal();
    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Tuning")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Tuning", meta=(ClampMin="100.0"))
    float VehicleSearchRadius = 950.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Tuning", meta=(ClampMin="1"))
    int32 EngineBaseCost = 240;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Tuning", meta=(ClampMin="1"))
    int32 TireBaseCost = 170;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Tuning", meta=(ClampMin="1"))
    int32 TireRepairCost = 65;

private:
    AGTTVehicleBase* FindNearestOwnedVehicle() const;
};
