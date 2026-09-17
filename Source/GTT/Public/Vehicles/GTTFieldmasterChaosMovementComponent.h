#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GTTFieldmasterChaosMovementComponent.generated.h"

/** One wheel sample captured directly from Chaos Vehicles at runtime. */
struct FGTTFieldmasterWheelRuntimeTelemetry
{
    int32 WheelIndex = INDEX_NONE;
    bool bValid = false;
    bool bInContact = false;
    bool bSlipping = false;
    bool bSkidding = false;
    float NormalizedSuspensionLength = 0.0f;
    float SpringForce = 0.0f;
    float SlipAngle = 0.0f;
    float SlipMagnitude = 0.0f;
    float SkidMagnitude = 0.0f;
    float DriveTorque = 0.0f;
    float BrakeTorque = 0.0f;
};

/** Compact authoritative snapshot used by packaged-runtime acceptance. */
struct FGTTFieldmasterRuntimeTelemetry
{
    bool bConfigurationValid = false;
    bool bMovementActive = false;
    int32 CurrentGear = 0;
    int32 TargetGear = 0;
    float EngineRpm = 0.0f;
    float EngineMaxRpm = 0.0f;
    float ForwardSpeedKmh = 0.0f;
    float RequestedSignedThrottle = 0.0f;
    float EffectiveThrottle = 0.0f;
    float EffectiveSteering = 0.0f;
    float DriveHealthFactor = 1.0f;
    float SteeringGripFactor = 1.0f;
    float TerrainGripFactor = 1.0f;
    int32 ValidWheelCount = 0;
    int32 ContactCount = 0;
    int32 SuspensionSampleCount = 0;
    int32 SlippingWheelCount = 0;
    int32 SkiddingWheelCount = 0;
    float SuspensionMin = 0.0f;
    float SuspensionMax = 0.0f;
    float TotalSpringForce = 0.0f;
    float MaxSlipMagnitude = 0.0f;
    float TotalDriveTorque = 0.0f;
    float TotalBrakeTorque = 0.0f;
    TArray<FGTTFieldmasterWheelRuntimeTelemetry> Wheels;
};

/**
 * Dedicated native Chaos movement authority for the Rusty Fieldmaster 60.
 *
 * This component owns the tractor's canonical Chaos wheel/powertrain setup and
 * translates shared sandbox state (fuel, condition, tire integrity and terrain
 * grip) into the inputs consumed by Chaos Vehicles. The legacy vehicle remains
 * the persistence mirror/fallback until packaged runtime acceptance is proven.
 */
UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class GTT_API UGTTFieldmasterChaosMovementComponent : public UChaosWheeledVehicleMovementComponent
{
    GENERATED_BODY()

public:
    UGTTFieldmasterChaosMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Fieldmaster")
    bool ConfigureAndValidateFieldmaster(FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Fieldmaster")
    void ApplyFieldmasterDriveCommand(
        float SignedThrottle,
        float Steering,
        bool bHasFuel,
        float ConditionPercent,
        float TireIntegrity,
        float TerrainGripFactor);

    UFUNCTION(BlueprintCallable, Category="GTT|Chaos|Fieldmaster")
    void HoldFieldmasterStopped();

    /** Capture live engine/gear/wheel/suspension state from Chaos Vehicles. */
    FGTTFieldmasterRuntimeTelemetry CaptureRuntimeTelemetry();

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster")
    float GetEffectiveThrottle() const { return EffectiveThrottle; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster")
    float GetEffectiveSteering() const { return EffectiveSteering; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster")
    float GetDriveHealthFactor() const { return DriveHealthFactor; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster")
    float GetSteeringGripFactor() const { return SteeringGripFactor; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster")
    bool IsFieldmasterConfigurationValid() const { return bFieldmasterConfigurationValid; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster")
    FString GetFieldmasterConfigurationSummary() const { return FieldmasterConfigurationSummary; }

private:
    static float NormalizeCondition(float ConditionPercent);

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster")
    bool bFieldmasterConfigurationValid = false;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster")
    FString FieldmasterConfigurationSummary = TEXT("Not configured");

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster")
    float EffectiveThrottle = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster")
    float EffectiveSteering = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster")
    float DriveHealthFactor = 1.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster")
    float SteeringGripFactor = 1.0f;

    float LastRequestedSignedThrottle = 0.0f;
    float LastTerrainGripFactor = 1.0f;
};
