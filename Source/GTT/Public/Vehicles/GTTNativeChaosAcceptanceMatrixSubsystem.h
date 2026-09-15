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

    FName VehicleId = NAME_None;
    float ConsecutiveAcceptedSeconds = 0.0f;
    float EvidenceSeconds = 0.0f;
    float MinObservedSuspension = 1.0f;
    float MaxObservedSuspension = 0.0f;
    int32 PeakGroundContacts = 0;
    bool bSmokeReadyReported = false;
};

/** Fleet-wide Native Chaos acceptance matrix backed by live wheel/contact/suspension evidence. */
UCLASS()
class GTT_API UGTTNativeChaosAcceptanceMatrixSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    /** True only after sustained accepted runtime, ground contact and measurable suspension travel. */
    UFUNCTION(BlueprintPure, Category="GTT|Vehicles|Native Chaos")
    bool IsVehicleRuntimeSmokeReady(FName VehicleId) const;

private:
    void EvaluatePawn(APawn* Pawn, FName VehicleId, bool bTakeoverActive, bool bPawnReady, float DeltaTime);
    bool EvaluateLiveMatrix(APawn* Pawn, FName VehicleId, bool bPawnReady,
        UChaosWheeledVehicleMovementComponent* Movement, USkeletalMeshComponent* Mesh,
        int32& OutValidWheels, int32& OutGroundContacts, int32& OutSuspensionSamples,
        float& OutMinSuspension, float& OutMaxSuspension, bool& bOutWheelConfig,
        bool& bOutPowertrainConfig, FString& OutWheelSummary, FString& OutPowertrainSummary) const;

    TMap<TWeakObjectPtr<APawn>, FGTTNativeChaosAcceptanceState> States;
};
