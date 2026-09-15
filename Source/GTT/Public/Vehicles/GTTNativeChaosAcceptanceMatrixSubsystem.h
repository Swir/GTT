#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeChaosAcceptanceMatrixSubsystem.generated.h"

class APawn;
class UChaosWheeledVehicleMovementComponent;
class USkeletalMeshComponent;

USTRUCT()
struct FGTTNativeChaosAcceptanceState
{
    GENERATED_BODY()

    float ConsecutiveAcceptedSeconds = 0.0f;
    float EvidenceSeconds = 0.0f;
    float MinObservedSuspension = 1.0f;
    float MaxObservedSuspension = 0.0f;
    int32 PeakGroundContacts = 0;
    bool bSmokeReadyReported = false;
};

/**
 * Fleet-wide Native Chaos acceptance matrix.
 * Combines authored wheel/powertrain configuration with live Physics Asset,
 * wheel contact and suspension evidence. It produces runtime smoke evidence,
 * but deliberately does not claim packaged Win64 verification.
 */
UCLASS()
class GTT_API UGTTNativeChaosAcceptanceMatrixSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    void EvaluatePawn(APawn* Pawn, FName VehicleId, bool bTakeoverActive, bool bPawnReady, float DeltaTime);
    bool EvaluateLiveMatrix(
        APawn* Pawn,
        FName VehicleId,
        bool bPawnReady,
        UChaosWheeledVehicleMovementComponent* Movement,
        USkeletalMeshComponent* Mesh,
        int32& OutValidWheels,
        int32& OutGroundContacts,
        int32& OutSuspensionSamples,
        float& OutMinSuspension,
        float& OutMaxSuspension,
        bool& bOutWheelConfig,
        bool& bOutPowertrainConfig,
        FString& OutWheelSummary,
        FString& OutPowertrainSummary) const;

    TMap<TWeakObjectPtr<APawn>, FGTTNativeChaosAcceptanceState> States;
};
