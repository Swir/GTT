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
    UPROPERTY(BlueprintReadOnly) bool bInPullOverZone = false;
    UPROPERTY(BlueprintReadOnly) float PullOverDistanceMeters = 0.0f;
    UPROPERTY(BlueprintReadOnly) FVector PullOverWorldLocation = FVector::ZeroVector;
};

/**
 * Single world authority for an active warden vehicle stop.
 *
 * 0.1.25 gives each incident a stable roadside frame: a lane anchor used by
 * ambient traffic, a physical shoulder target the player must actually reach,
 * ranger staging positions and a patrol-unit parking transform. 0.1.26 exposes
 * that exact same target as a world-space presentation transform, keeping the
 * marker visual-only rather than creating a second source of compliance truth.
 */
UCLASS()
class GTT_API UGTTRangerRoadStopSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    bool BeginStop(AGTTRangerAIController* Controller, APawn* Target, const FVector& RangerLocation, float GraceSeconds);
    void UpdateStop(AGTTRangerAIController* Controller, APawn* Target, float SecondsRemaining,
        float HoldElapsed, float HoldRequired, float TargetSpeedKmh, bool bSearching,
        float InComplianceSpeedLimitKmh = 2.5f, bool bInPullOverZone = false);
    void MarkFlee(AGTTRangerAIController* Controller, float DisplaySeconds = 4.5f);
    void EndStop(AGTTRangerAIController* Controller);

    bool IsOwnedBy(const AGTTRangerAIController* Controller) const;
    bool IsStopForTarget(const APawn* Target) const;
    bool HasTrafficControl() const;
    bool HasPatrolScene() const;

    FVector GetRangerStagingPoint(float LateralOffsetCm, float RearOffsetCm) const;
    FVector GetRangerSupportPoint(float LateralOffsetCm, float RearOffsetCm) const;
    FVector GetPullOverTargetLocation() const { return PullOverTargetLocation; }
    float GetPullOverDistanceCm(const APawn* Target) const;
    bool IsTargetInPullOverZone(const APawn* Target, float AcceptanceRadiusCm = 275.0f) const;
    bool GetPullOverMarkerTransform(FTransform& OutTransform) const
    {
        if (!HasTrafficControl())
        {
            return false;
        }
        OutTransform = FTransform(RoadForward.Rotation(), PullOverTargetLocation, FVector::OneVector);
        return true;
    }
    bool GetPatrolVehicleTransform(FTransform& OutTransform) const;

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
    FVector PullOverTargetLocation = FVector::ZeroVector;
    FVector RoadForward = FVector::ForwardVector;
    float ShoulderSide = 1.0f;
    float RemainingSeconds = 0.0f;
    float InitialGraceSeconds = 0.0f;
    float SearchHoldElapsed = 0.0f;
    float SearchHoldRequired = 0.0f;
    float ObservedTargetSpeedKmh = 0.0f;
    float ComplianceSpeedLimitKmh = 2.5f;
    float FleeDisplayUntilSeconds = 0.0f;
    bool bTargetInPullOverZone = false;

    static constexpr float PullOverAheadCm = 700.0f;
    static constexpr float PullOverLateralCm = 420.0f;
    static constexpr float PullOverAcceptanceRadiusCm = 275.0f;
    static constexpr float PatrolVehicleRearOffsetCm = 520.0f;

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