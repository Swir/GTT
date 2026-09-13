#pragma once

#include "CoreMinimal.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTTOldCarPawn.generated.h"

class UGTTChaosVehicleBridgeComponent;
class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTOldCarPawn : public AGTTVehicleBase
{
    GENERATED_BODY()

public:
    AGTTOldCarPawn();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
    void CaptureChaosThrottle(float Value);
    void CaptureChaosSteering(float Value);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Chaos")
    TObjectPtr<UGTTChaosVehicleBridgeComponent> ChaosVehicleBridge;

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
