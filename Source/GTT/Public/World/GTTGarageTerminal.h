#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/GTTInteractable.h"
#include "GTTGarageTerminal.generated.h"

class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTGarageTerminal : public AActor, public IGTTInteractable
{
    GENERATED_BODY()

public:
    AGTTGarageTerminal();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Garage")
    TObjectPtr<UStaticMeshComponent> Mesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Garage", meta=(ClampMin="0"))
    int32 RegistrationCost = 250;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Garage", meta=(ClampMin="100.0"))
    float VehicleSearchRadius = 1000.0f;
};
