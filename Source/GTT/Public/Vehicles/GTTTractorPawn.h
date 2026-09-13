#pragma once

#include "CoreMinimal.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTTTractorPawn.generated.h"

class UInputComponent;
class UStaticMeshComponent;
class UGTTChaosVehicleBridgeComponent;

UCLASS(Blueprintable)
class GTT_API AGTTTractorPawn : public AGTTVehicleBase
{
    GENERATED_BODY()

public:
    AGTTTractorPawn();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Tractor|Chaos")
    TObjectPtr<UGTTChaosVehicleBridgeComponent> ChaosVehicleBridge;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Tractor")
    TObjectPtr<UStaticMeshComponent> HoodMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Tractor")
    TObjectPtr<UStaticMeshComponent> CabinMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Tractor")
    TObjectPtr<UStaticMeshComponent> ExhaustMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Tractor")
    TObjectPtr<UStaticMeshComponent> LeftFrontWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Tractor")
    TObjectPtr<UStaticMeshComponent> RightFrontWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Tractor")
    TObjectPtr<UStaticMeshComponent> LeftRearWheel;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Tractor")
    TObjectPtr<UStaticMeshComponent> RightRearWheel;

private:
    void CaptureChaosThrottle(float Value);
    void CaptureChaosSteering(float Value);
};
