#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GTTFieldmasterChaosMovementComponent.generated.h"

/**
 * Dedicated native Chaos movement authority for the Rusty Fieldmaster 60.
 *
 * This component owns the tractor's canonical Chaos wheel/powertrain setup and
 * translates shared sandbox state (fuel, condition, tire integrity, terrain
 * grip and live trailer load) into the inputs consumed by Chaos Vehicles. The
 * legacy vehicle remains the persistence mirror/fallback until packaged runtime
 * acceptance is proven.
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

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster")
    float GetEffectiveThrottle() const { return EffectiveThrottle; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster")
    float GetEffectiveSteering() const { return EffectiveSteering; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster")
    float GetDriveHealthFactor() const { return DriveHealthFactor; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster")
    float GetSteeringGripFactor() const { return SteeringGripFactor; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Trailer")
    float GetTowLoadFactor() const { return TowLoadFactor; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Trailer")
    float GetTowThrottleAuthority() const { return TowThrottleAuthority; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Trailer")
    float GetTowSteeringAuthority() const { return TowSteeringAuthority; }

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

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    float TowLoadFactor = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    float TowThrottleAuthority = 1.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    float TowSteeringAuthority = 1.0f;
};
