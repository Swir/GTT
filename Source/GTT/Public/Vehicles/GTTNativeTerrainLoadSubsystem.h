#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeTerrainLoadSubsystem.generated.h"

class AGTTFarmTrailer;
class AGTTFieldmasterNativePawn;

/**
 * Terrain/load controller for the Native Fieldmaster.
 * Combines authored mud, grade, trailer tongue load and four-wheel ground/load
 * evidence into hill-start, rollback and low/medium-speed cross-axle control.
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
    struct FWheelLoadEvidence
    {
        float FrontLeftClearanceCm = 0.0f;
        float FrontRightClearanceCm = 0.0f;
        float RearLeftClearanceCm = 0.0f;
        float RearRightClearanceCm = 0.0f;
        float FrontLeftLoad = 1.0f;
        float FrontRightLoad = 1.0f;
        float RearLeftLoad = 1.0f;
        float RearRightLoad = 1.0f;
        float FrontLoadFactor = 1.0f;
        float RearLoadFactor = 1.0f;
        float FrontCrossAxleImbalance = 0.0f;
        float RearCrossAxleImbalance = 0.0f;
        float SideLoadImbalance = 0.0f;
        int32 GroundedWheelCount = 0;
    };

    struct FTerrainLoadState
    {
        float EvidenceSeconds = 0.0f;
        float RollbackSeconds = 0.0f;
        float CrossAxleRiskSeconds = 0.0f;
        float LastThrottleLimit = 1.0f;
        float LastRearLoadBias = 0.0f;
        float LastLaunchGrip = 1.0f;
        float LastCrossAxleRisk = 0.0f;
        float LastFrontAxleGrip = 1.0f;
        float LastRearAxleGrip = 1.0f;
        bool bLastRollbackControl = false;
        bool bLastCrossAxleControl = false;
    };

    void EvaluateFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime);
    bool SampleWheelLoadEvidence(const AGTTFieldmasterNativePawn* NativePawn, FWheelLoadEvidence& OutEvidence) const;
    AGTTFarmTrailer* FindAttachedTrailer(const AGTTFieldmasterNativePawn* NativePawn) const;

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, FTerrainLoadState> TerrainLoadStates;
};
