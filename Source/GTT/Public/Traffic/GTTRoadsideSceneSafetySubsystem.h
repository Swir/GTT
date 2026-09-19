#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTRoadsideSceneSafetySubsystem.generated.h"

class AGTTRoadsideResponderVehicle;
class AGTTTrafficCarPawn;

/**
 * Scene-safety layer for an on-scene county road-service responder.
 *
 * It does not own incidents, vehicle repair, economy, Wanted or ranger state.
 * It only creates bounded, cooldown-limited traffic yielding around a deployed
 * responder and tapers that corridor during the post-recovery lane-reopening
 * phase.
 */
UCLASS()
class GTT_API UGTTRoadsideSceneSafetySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Responder|Safety")
    bool HasActiveSafetyCorridor() const { return bSafetyCorridorActive; }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Responder|Safety")
    int32 GetLastYieldCount() const { return LastYieldCount; }

private:
    AGTTRoadsideResponderVehicle* FindOnSceneResponder() const;
    void ApplySafetyCorridor(AGTTRoadsideResponderVehicle* Responder);
    void PruneYieldHistory();

    TWeakObjectPtr<AGTTRoadsideResponderVehicle> TrackedResponder;
    TMap<TWeakObjectPtr<AGTTTrafficCarPawn>, float> LastYieldTimeByCar;
    float ScanAccumulator = 0.0f;
    int32 LastYieldCount = 0;
    bool bSafetyCorridorActive = false;

    static constexpr float ScanIntervalSeconds = 0.75f;
    static constexpr float SafetyRadiusCm = 1800.0f;
    static constexpr float ReopeningRadiusCm = 980.0f;
    static constexpr float InnerPassRadiusCm = 320.0f;
    static constexpr float ReYieldCooldownSeconds = 4.5f;
    static constexpr float YieldSeverity = 0.38f;
    static constexpr float ReopeningYieldSeverity = 0.20f;
};
