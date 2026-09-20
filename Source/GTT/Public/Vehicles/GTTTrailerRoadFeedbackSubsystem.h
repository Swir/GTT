#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTTrailerRoadFeedbackSubsystem.generated.h"

class AGTTFarmTrailer;
class UPointLightComponent;

struct FGTTTrailerRoadFeedbackRuntime
{
    TWeakObjectPtr<UPointLightComponent> TailLeft;
    TWeakObjectPtr<UPointLightComponent> TailRight;
    TWeakObjectPtr<UPointLightComponent> ReverseLeft;
    TWeakObjectPtr<UPointLightComponent> ReverseRight;
    TWeakObjectPtr<UPointLightComponent> HazardLeft;
    TWeakObjectPtr<UPointLightComponent> HazardRight;
    float LastLongitudinalSpeedKmh = 0.0f;
    bool bHasVelocitySample = false;
};

/**
 * Runtime road-feedback layer for the physical farm trailer.
 *
 * Keeps visible lighting and a deliberately bounded loaded-trailer anti-sway
 * assist tied to authoritative trailer state. It does not replace authored
 * trailer assets or the final skeletal wheel/hitch implementation.
 */
UCLASS()
class GTT_API UGTTTrailerRoadFeedbackSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return true; }

private:
    void EnsureRoadLights(AGTTFarmTrailer* Trailer, FGTTTrailerRoadFeedbackRuntime& Runtime);
    void UpdateRoadLights(AGTTFarmTrailer* Trailer, FGTTTrailerRoadFeedbackRuntime& Runtime, float LongitudinalSpeedKmh, float DecelerationKmhPerSecond) const;
    void ApplyLoadedTrailerStability(AGTTFarmTrailer* Trailer, float DeltaTime) const;

    TMap<TWeakObjectPtr<AGTTFarmTrailer>, FGTTTrailerRoadFeedbackRuntime> RuntimeByTrailer;

    static constexpr float StabilityStartSpeedKmh = 25.0f;
    static constexpr float StabilityFullSpeedKmh = 70.0f;
    static constexpr float MaximumStabilityAuthority = 0.45f;
    static constexpr float MaximumLateralStabilityForce = 650000.0f;
    static constexpr float MaximumYawStabilityTorque = 5000000.0f;
    static constexpr float BrakeDecelerationThresholdKmhPerSecond = 6.0f;
    static constexpr float ReverseLightThresholdKmh = -2.0f;
    static constexpr float CriticalIntegrityThreshold = 0.45f;
};
