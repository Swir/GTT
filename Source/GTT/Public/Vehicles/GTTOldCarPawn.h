#pragma once

#include "CoreMinimal.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTTOldCarPawn.generated.h"

class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTOldCarPawn : public AGTTVehicleBase
{
    GENERATED_BODY()

public:
    AGTTOldCarPawn();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|OldCar")
    TObjectPtr<UStaticMeshComponent> CabinMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|OldCar")
    TObjectPtr<UStaticMeshComponent> TrunkMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|OldCar")
    TObjectPtr<UStaticMeshComponent> LeftFrontWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|OldCar")
    TObjectPtr<UStaticMeshComponent> RightFrontWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|OldCar")
    TObjectPtr<UStaticMeshComponent> LeftRearWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|OldCar")
    TObjectPtr<UStaticMeshComponent> RightRearWheel;
};
