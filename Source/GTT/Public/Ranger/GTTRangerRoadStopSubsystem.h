#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTRangerRoadStopSubsystem.generated.h"

class AGTTRangerAIController;
class APawn;

UENUM(BlueprintType)
enum class EGTTRangerRoadStopPhase : uint8
{
    None,
    Comply,
    Search,
    Flee
};

/**
 * Single world authority for an active warden vehicle stop.
 *
 * The subsystem keeps ranger positioning, ambient-traffic yielding and the
 * player-facing enforcement phase on one shared state instead of letting each
 * ranger or traffic car invent its own copy of the incident.
 */
UCLASS()
class GTT_API UGTTRangerRoadStopSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    bool BeginStop(AGTTRangerAIController* Controller, APawn* Target, const FVector& RangerLocation, float GraceSeconds);
    void UpdateStop(AGTTRangerAIController* Controller, APawn* Target, float SecondsRemaining,
        float HoldElapsed, float HoldRequired, float TargetSpeedKmh, bool bSearching);
    void MarkFlee(AGTTRangerAIController* Controller, float DisplaySeconds = 4.5f);
    void EndStop(AGTTRangerAIController* Controller);

    bool IsOwnedBy(const AGTTRangerAIController* Controller) const;
    bool IsStopForTarget(const APawn* Target) const;
    bool HasTrafficControl() const;

    FVector GetRangerStagingPoint(float LateralOffsetCm, float RearOffsetCm) const;
    FVector GetRangerSupportPoint(float LateralOffsetCm, float RearOffsetCm) const;

    bool GetTrafficResponse(const FVector& VehicleLocation, const FVector& VehicleForward,
        float& OutSpeedScale, bool& bOutHold) const;

    EGTTRangerRoadStopPhase GetPhase() const { return Phase; }
    FString GetStatusText() const;

private:
    void RefreshRoadFrame(APawn* Target);
    bool IsFleeDisplayVisible() const;
    void ClearStop();

    TWeakObjectPtr<AGTTRangerAIController> ActiveController;
    TWeakObjectPtr<APawn> ActiveTarget;
    EGTTRangerRoadStopPhase Phase = EGTTRangerRoadStopPhase::None;
    FVector StopLocation = FVector::ZeroVector;
    FVector RoadForward = FVector::ForwardVector;
    float ShoulderSide = 1.0f;
    float RemainingSeconds = 0.0f;
    float SearchHoldElapsed = 0.0f;
    float SearchHoldRequired = 0.0f;
    float ObservedTargetSpeedKmh = 0.0f;
    float FleeDisplayUntilSeconds = 0.0f;

    static constexpr float TrafficSlowRadiusCm = 2200.0f;
    static constexpr float TrafficHoldRadiusCm = 650.0f;
    static constexpr float TrafficSearchHoldRadiusCm = 800.0f;
    static constexpr float TrafficCorridorHalfWidthCm = 1100.0f;
};
