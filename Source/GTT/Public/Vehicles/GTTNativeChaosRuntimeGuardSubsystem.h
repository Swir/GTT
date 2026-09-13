#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeChaosRuntimeGuardSubsystem.generated.h"

class AGTTFieldmasterNativePawn;

/**
 * Runtime safety/evidence layer for the Native Chaos Fieldmaster takeover.
 * It does not claim visual or packaged-build acceptance. It prevents a live
 * native takeover from remaining active when the Chaos movement component
 * becomes inactive and emits periodic evidence suitable for UE runtime logs.
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

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, float> InvalidRuntimeSeconds;
    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, float> EvidenceLogSeconds;
};
