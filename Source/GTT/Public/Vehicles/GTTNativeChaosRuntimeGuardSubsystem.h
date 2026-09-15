#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeChaosRuntimeGuardSubsystem.generated.h"

class APawn;
class AGTTFieldmasterNativePawn;
class AGTTRoadVehicleNativePawn;
class UChaosWheeledVehicleMovementComponent;
class USkeletalMeshComponent;

/**
 * Runtime safety/evidence layer for every active Native Chaos takeover.
 * It validates the live movement/physics/wheel/suspension contract and forces
 * a safe legacy fallback after a grace period when the runtime contract fails.
 * This is runtime evidence only; it does not claim packaged Win64 acceptance.
 */
UCLASS()
class GTT_API UGTTNativeChaosRuntimeGuardSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    void EvaluateFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime);
    void EvaluateRoadVehicle(AGTTRoadVehicleNativePawn* NativePawn, float DeltaTime);
    bool EvaluateLiveChaosContract(
        APawn* NativePawn,
        UChaosWheeledVehicleMovementComponent* Movement,
        USkeletalMeshComponent* Mesh,
        int32& OutValidWheels,
        int32& OutGroundContacts,
        int32& OutSuspensionSamples,
        float& OutMinSuspension,
        float& OutMaxSuspension) const;
    void ClearRuntimeState(APawn* NativePawn);

    TMap<TWeakObjectPtr<APawn>, float> InvalidRuntimeSeconds;
    TMap<TWeakObjectPtr<APawn>, float> EvidenceLogSeconds;
};
