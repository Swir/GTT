#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTTrailerEvidenceScenarioSubsystem.generated.h"

class AGTTFarmTrailer;
class AGTTFieldmasterNativePawn;
class UChaosWheeledVehicleMovementComponent;
class UGTTTrailerAuthoredRuntimeSubsystem;

UCLASS()
class GTT_API UGTTTrailerEvidenceScenarioSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    enum class ETrailerEvidencePhase : uint8
    {
        Waiting,
        StageAndAttach,
        AwaitAuthoredRuntime,
        LoadedMotion,
        ControlledStop,
        Complete
    };

    AGTTFieldmasterNativePawn* ResolveFieldmaster();
    AGTTFarmTrailer* ResolveTrailer();
    UChaosWheeledVehicleMovementComponent* ResolveMovement(AGTTFieldmasterNativePawn* Pawn) const;
    UGTTTrailerAuthoredRuntimeSubsystem* ResolveRuntime() const;
    bool StageTrailerAtHitch(AGTTFieldmasterNativePawn* Pawn, AGTTFarmTrailer* Trailer);
    void MarkFailure(const TCHAR* Reason);
    void CompleteScenario(AGTTFieldmasterNativePawn* Pawn, AGTTFarmTrailer* Trailer, UChaosWheeledVehicleMovementComponent* Movement, const TCHAR* Reason);

    bool bEnabled = false;
    bool bFinished = false;
    bool bSequenceHealthy = true;
    bool bAttachmentProven = false;
    bool bAuthoredRuntimeProven = false;
    bool bLoadedTowProven = false;
    bool bControlledStopProven = false;
    float Elapsed = 0.0f;
    float PhaseStartedSeconds = 0.0f;
    float SampleAccumulator = 0.0f;
    FVector MotionStartLocation = FVector::ZeroVector;
    float MaxTowSpeedKmh = 0.0f;
    float MaxTowDistanceCm = 0.0f;
    float MaxHitchErrorCm = 0.0f;
    float MaxArticulationDeg = 0.0f;
    float MinCargoIntegrity = 1.0f;
    int32 MovingDualContactSamples = 0;
    int32 SafeMovingSamples = 0;
    ETrailerEvidencePhase Phase = ETrailerEvidencePhase::Waiting;
    TWeakObjectPtr<AGTTFieldmasterNativePawn> Fieldmaster;
    TWeakObjectPtr<AGTTFarmTrailer> Trailer;
};
