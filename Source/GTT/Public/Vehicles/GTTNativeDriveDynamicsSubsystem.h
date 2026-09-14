#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeDriveDynamicsSubsystem.generated.h"

class APawn;
class AGTTFieldmasterNativePawn;
class AGTTRoadVehicleNativePawn;
class UChaosWheeledVehicleMovementComponent;

/**
 * Final safety authority layered on top of each Native Chaos pawn's vehicle-specific
 * tuning/traction logic. It prevents unsafe instant direction swaps, provides
 * neutral engine-braking / low-speed hold, and keeps one shared runtime evidence
 * path for the Fieldmaster, Rattleback and Mulebox drivetrains.
 */
struct FGTTNativeDrivetrainAuthorityState
{
    int32 StableDirection = 1;
    bool bInitialized = false;
    bool bDirectionInterlock = false;
    bool bEngineBrakeActive = false;
    float EvidenceSeconds = 0.0f;
};

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
        float DeltaTime);
    void RemoveAuthorityState(APawn* NativePawn);

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, float> EvidenceSeconds;
    TMap<TWeakObjectPtr<APawn>, FGTTNativeDrivetrainAuthorityState> AuthorityStates;
};
