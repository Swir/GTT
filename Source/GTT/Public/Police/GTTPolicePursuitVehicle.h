#pragma once

#include "CoreMinimal.h"
#include "Vehicles/GTTVehicleBase.h"
#include "GTTPolicePursuitVehicle.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class GTT_API AGTTPolicePursuitVehicle : public AGTTVehicleBase
{
    GENERATED_BODY()

public:
    AGTTPolicePursuitVehicle();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="GTT|Police")
    int32 GetResponseTier() const { return ResponseTier; }

    UFUNCTION(BlueprintCallable, Category="GTT|Police")
    void SetResponseTier(int32 InTier);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Police")
    TObjectPtr<UStaticMeshComponent> BeaconLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Police")
    TObjectPtr<UStaticMeshComponent> BeaconRight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Police")
    TObjectPtr<UTextRenderComponent> PoliceLabel;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="100.0"))
    float PursuitAcceleration = 1350.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="1.0"))
    float PursuitSteeringTorque = 105.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="100.0"))
    float BrakeDistance = 500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="50.0"))
    float ArrestRadius = 210.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Police", meta=(ClampMin="0.5"))
    float ArrestCooldown = 4.0f;

private:
    void UpdateBeacon(float DeltaSeconds);
    void DriveTowardPlayer(APawn* PlayerPawn, int32 WantedLevel, float DeltaSeconds);

    int32 ResponseTier = 1;
    float BeaconClock = 0.0f;
    float LastArrestAttemptTime = -1000.0f;
};
