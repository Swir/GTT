#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GTTFieldmasterChaosMovementComponent.generated.h"

UENUM(BlueprintType)
enum class EGTTTrailerBrakeThermalState : uint8
{
    Normal UMETA(DisplayName="Normal"),
    Hot UMETA(DisplayName="Hot"),
    Fading UMETA(DisplayName="Fading"),
    Critical UMETA(DisplayName="Critical")
};

/**
 * Dedicated native Chaos movement authority for the Rusty Fieldmaster 60.
 *
 * This component owns the tractor's canonical Chaos wheel/powertrain setup and
 * translates shared sandbox state (fuel, condition, tire integrity, terrain
 * grip, live trailer load, hill grade and trailer-brake thermal state) into
 * the inputs consumed by Chaos Vehicles. The legacy vehicle remains the
 * persistence mirror/fallback until packaged runtime acceptance is proven.
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

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Terrain")
    float GetTerrainThrottleAuthority() const { return TerrainThrottleAuthority; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Terrain")
    float GetTravelGradeDegrees() const { return TravelGradeDegrees; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Terrain")
    float GetHillHaulBrake() const { return HillHaulBrake; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Terrain")
    bool IsHillHoldActive() const { return bHillHoldActive; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Terrain")
    bool IsDownhillTowBrakeActive() const { return bDownhillTowBrakeActive; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Trailer")
    float GetTrailerBrakeHeat01() const { return TrailerBrakeHeat01; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Trailer")
    float GetTrailerBrakeAuthority() const { return TrailerBrakeAuthority; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Trailer")
    bool IsTrailerBrakeFadeActive() const { return bTrailerBrakeFadeActive; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Trailer")
    bool IsTrailerBrakeCoolingActive() const { return bTrailerBrakeCoolingActive; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Trailer")
    EGTTTrailerBrakeThermalState GetTrailerBrakeThermalState() const { return TrailerBrakeThermalState; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Trailer")
    bool IsTrailerRunawayMitigationActive() const { return bTrailerRunawayMitigationActive; }

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Fieldmaster|Trailer")
    float GetTrailerRunawaySafetyBrake() const { return TrailerRunawaySafetyBrake; }

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
    void UpdateTrailerBrakeThermalState();

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

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Terrain")
    float TerrainThrottleAuthority = 1.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Terrain")
    float TravelGradeDegrees = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Terrain")
    float HillHaulBrake = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Terrain")
    bool bHillHoldActive = false;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Terrain")
    bool bDownhillTowBrakeActive = false;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    float TrailerBrakeHeat01 = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    float TrailerBrakeAuthority = 1.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    bool bTrailerBrakeFadeActive = false;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    bool bTrailerBrakeCoolingActive = false;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    EGTTTrailerBrakeThermalState TrailerBrakeThermalState = EGTTTrailerBrakeThermalState::Normal;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    bool bTrailerRunawayMitigationActive = false;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    float TrailerRunawaySafetyBrake = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    float TowLoadFactor = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    float TowThrottleAuthority = 1.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Chaos|Fieldmaster|Trailer")
    float TowSteeringAuthority = 1.0f;
};
