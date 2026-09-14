#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeDriveDynamicsSubsystem.generated.h"

class APawn;
class AGTTFieldmasterNativePawn;
class AGTTRoadVehicleNativePawn;
class UChaosWheeledVehicleMovementComponent;

/** Final command state after drivetrain, axle, suspension and safety limits are composed. */
struct FGTTNativeDrivetrainAuthorityState
{
    int32 StableDirection = 1;
    bool bInitialized = false;
    bool bDirectionInterlock = false;
    bool bEngineBrakeActive = false;
    bool bAxleTorqueCut = false;
    bool bSuspensionRuntimeReady = false;
    float FinalThrottle = 0.0f;
    float FinalBrake = 0.0f;
    float FinalSteering = 0.0f;
    float EvidenceSeconds = 0.0f;
};

/**
 * Single final Native Chaos command composer for Fieldmaster, Rattleback and Mulebox.
 * Vehicle-specific systems may reduce available performance upstream, but this subsystem
 * composes the strictest drivetrain/axle/suspension safety result once per vehicle.
 */
UCLASS()
class GTT_API UGTTNativeDriveDynamicsSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    void ApplyDriveDynamics(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime);
    void ApplyDrivetrainAuthority(
        APawn* NativePawn,
        UChaosWheeledVehicleMovementComponent* Movement,
        FName VehicleId,
        bool bAuthorityEligible,
        float TireIntegrity,
        int32 TireUpgradeLevel,
        float DeltaTime);
    void RemoveAuthorityState(APawn* NativePawn);

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, float> EvidenceSeconds;
    TMap<TWeakObjectPtr<APawn>, FGTTNativeDrivetrainAuthorityState> AuthorityStates;
};
