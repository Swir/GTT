#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTDrivetrainEvidenceScenarioSubsystem.generated.h"

class AGTTFieldmasterNativePawn;
class UChaosWheeledVehicleMovementComponent;

/**
 * Late-running packaged evidence scenario for the Native Fieldmaster drivetrain.
 *
 * The regular demo smoke scenario owns the first ~75 seconds of the deterministic run.
 * This subsystem deliberately starts afterwards so it can exercise automatic forward
 * shifting, a controlled forward->reverse transition, real reverse motion and the
 * reverse->forward return without racing the core smoke controls.
 */
UCLASS()
class GTT_API UGTTDrivetrainEvidenceScenarioSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    enum class EDrivetrainEvidencePhase : uint8
    {
        Waiting,
        ForwardAcceleration,
        BrakeForReverse,
        ReverseAcceleration,
        BrakeForForward,
        ForwardReturn,
        Complete
    };

    AGTTFieldmasterNativePawn* ResolveFieldmaster();
    UChaosWheeledVehicleMovementComponent* ResolveMovement(AGTTFieldmasterNativePawn* Pawn) const;
    float GetSignedSpeedKmh(const AGTTFieldmasterNativePawn* Pawn) const;
    void BeginForwardAcceleration(AGTTFieldmasterNativePawn* Pawn, UChaosWheeledVehicleMovementComponent* Movement);
    void MarkFailure(const TCHAR* Reason);
    void CompleteScenario(AGTTFieldmasterNativePawn* Pawn, UChaosWheeledVehicleMovementComponent* Movement, const TCHAR* Reason);

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bAutomaticUpshiftObserved = false;
    bool bReverseInterlockObserved = false;
    bool bSafeReverseCommitObserved = false;
    bool bReverseMotionObserved = false;
    bool bSafeForwardCommitObserved = false;
    bool bForwardReturnObserved = false;
    float Elapsed = 0.0f;
    float PhaseStartedSeconds = 0.0f;
    float ReverseInterlockStartSpeedKmh = 0.0f;
    float ReverseCommitSpeedKmh = 0.0f;
    float ForwardCommitSpeedKmh = 0.0f;
    int32 MaxForwardGearObserved = 0;
    EDrivetrainEvidencePhase Phase = EDrivetrainEvidencePhase::Waiting;
    TWeakObjectPtr<AGTTFieldmasterNativePawn> Fieldmaster;
};
