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

USTRUCT(BlueprintType)
struct FGTTRangerRoadStopPresentation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bVisible = false;
    UPROPERTY(BlueprintReadOnly) EGTTRangerRoadStopPhase Phase = EGTTRangerRoadStopPhase::None;
    UPROPERTY(BlueprintReadOnly) FString PhaseLabel;
    UPROPERTY(BlueprintReadOnly) FString Instruction;
    UPROPERTY(BlueprintReadOnly) float Progress01 = 0.0f;
    UPROPERTY(BlueprintReadOnly) float SecondsRemaining = 0.0f;
    UPROPERTY(BlueprintReadOnly) float ObservedSpeedKmh = 0.0f;
};

/**
 * Single world authority for an active warden vehicle stop.
 *
 * The subsystem keeps ranger positioning, ambient-traffic yielding, civilian
 * scene response and the player-facing enforcement phase on one shared state
 * instead of letting each ranger, NPC or traffic car invent its own incident.
 */
UCLASS()
class GTT_API UGTTRangerRoadStopSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    bool BeginStop(AGTTRangerAIController* Controller, APawn* Target, const FVector& RangerLocation, float GraceSeconds);
    void UpdateStop(AGTTRangerAIController* Controller, APawn* Target, float SecondsRemaining,
        float HoldElapsed, float HoldRequired, float TargetSpeedKmh, bool bSearching,
        float ComplianceSpeedLimitKmh = 2.5f);
    void MarkFlee(AGTTRangerAIController* Controller, float DisplaySeconds = 4.5f);
    void EndStop(AGTTRangerAIController* Controller);

    bool IsOwnedBy(const AGTTRangerAIController* Controller) const;
    bool IsStopForTarget(const APawn* Target) const;
    bool HasTrafficControl() const;

    FVector GetRangerStagingPoint(float LateralOffsetCm, float RearOffsetCm) const;
    FVector GetRangerSupportPoint(float LateralOffsetCm, float RearOffsetCm) const;

    bool GetTrafficResponse(const FVector& VehicleLocation, const FVector& VehicleForward,
        float& OutSpeedScale, bool& bOutHold) const;

    /** Returns a temporary safe-side destination/focus point for nearby civilian NPCs. */
    bool GetCivilianResponse(const FVector& CitizenLocation, FVector& OutSafeLocation,
        FVector& OutFocusLocation, bool& bOutNeedsMove) const;

    UFUNCTION(BlueprintPure, Category="GTT|Ranger|Road Stop")
    FGTTRangerRoadStopPresentation GetPresentationSnapshot() const;

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
    float InitialGraceSeconds = 0.0f;
    float SearchHoldElapsed = 0.0f;
    float SearchHoldRequired = 0.0f;
    float ObservedTargetSpeedKmh = 0.0f;
    float ComplianceSpeedLimitKmh = 2.5f;
    float FleeDisplayUntilSeconds = 0.0f;

    static constexpr float TrafficSlowRadiusCm = 2200.0f;
    static constexpr float TrafficHoldRadiusCm = 650.0f;
    static constexpr float TrafficSearchHoldRadiusCm = 800.0f;
    static constexpr float TrafficCorridorHalfWidthCm = 1100.0f;
    static constexpr float TrafficSameDirectionDot = 0.35f;

    static constexpr float CivilianAwarenessRadiusCm = 1800.0f;
    static constexpr float CivilianMoveThresholdCm = 600.0f;
    static constexpr float CivilianSafeLateralCm = 720.0f;
    static constexpr float CivilianLongitudinalWindowCm = 1300.0f;
};
