#pragma once

#include "CoreMinimal.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTTFarmVanPawn.generated.h"

class UGTTChaosVehicleBridgeComponent;
class UStaticMeshComponent;

UCLASS()
class GTT_API AGTTFarmVanPawn : public AGTTVehicleBase
{
    GENERATED_BODY()

public:
    AGTTFarmVanPawn();
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
    void CaptureChaosThrottle(float Value);
    void CaptureChaosSteering(float Value);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Chaos")
    TObjectPtr<UGTTChaosVehicleBridgeComponent> ChaosVehicleBridge;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|FarmVan")
    TObjectPtr<UStaticMeshComponent> CabinMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|FarmVan")
    TObjectPtr<UStaticMeshComponent> CargoMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|FarmVan")
    TObjectPtr<UStaticMeshComponent> LeftFrontWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|FarmVan")
    TObjectPtr<UStaticMeshComponent> RightFrontWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|FarmVan")
    TObjectPtr<UStaticMeshComponent> LeftRearWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|FarmVan")
    TObjectPtr<UStaticMeshComponent> RightRearWheel;
};
