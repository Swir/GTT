#pragma once

#include "CoreMinimal.h"
#include "Vehicles/GTTFarmVanPawn.h"
#include "GTTRecoveryTargetVehicle.generated.h"

UCLASS()
class GTT_API AGTTRecoveryTargetVehicle : public AGTTFarmVanPawn
{
    GENERATED_BODY()

public:
    AGTTRecoveryTargetVehicle();

    virtual void Interact_Implementation(AActor* Interactor) override;
    virtual FText GetInteractionText_Implementation() const override;
};
