#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeTerrainLoadSubsystem.generated.h"

class AGTTFarmTrailer;
class AGTTFieldmasterNativePawn;

/**
 * Low-speed terrain/load controller for the Native Fieldmaster.
 * Complements the higher-speed stability controller by combining authored mud,
 * grade, axle clearance/load bias and trailer tongue load during hill starts.
 */
UCLASS()
class GTT_API UGTTNativeTerrainLoadSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    struct FTerrainLoadState
    {
        float EvidenceSeconds = 0.0f;
        float RollbackSeconds = 0.0f;
        float LastThrottleLimit = 1.0f;
        float LastRearLoadBias = 0.0f;
        float LastLaunchGrip = 1.0f;
        bool bLastRollbackControl = false;
    };

    void EvaluateFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime);
    bool SampleAxleClearance(const AGTTFieldmasterNativePawn* NativePawn, float& OutFrontCm, float& OutRearCm) const;
    AGTTFarmTrailer* FindAttachedTrailer(const AGTTFieldmasterNativePawn* NativePawn) const;

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, FTerrainLoadState> TerrainLoadStates;
};
