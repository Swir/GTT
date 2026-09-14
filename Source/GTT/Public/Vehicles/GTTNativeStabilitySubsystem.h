#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeStabilitySubsystem.generated.h"

class AGTTFarmTrailer;
class AGTTFieldmasterNativePawn;

/**
 * Contact-aware stability controller for the Native Chaos Fieldmaster.
 * It combines authored wheel-ground evidence with heavy-haul load/sway state,
 * axle load-transfer and terrain grade so cargo mass, hitch stress, trailer
 * damage and uneven ground alter driving intervention without duplicating state.
 */
UCLASS()
class GTT_API UGTTNativeStabilitySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    struct FStabilityState
    {
        float EvidenceSeconds = 0.0f;
        float LowContactSeconds = 0.0f;
        float TrailerSwaySeconds = 0.0f;
        float LoadTransferSeconds = 0.0f;
        int32 LastContacts = 4;
        float LastRisk = 0.0f;
        float LastTowLoad = 0.0f;
        float LastSwayRisk = 0.0f;
        float LastLoadTransferRisk = 0.0f;
        float LastFrontRearBias = 0.0f;
        float LastSideBias = 0.0f;
    };

    void EvaluateFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime);
    int32 SampleWheelContacts(AGTTFieldmasterNativePawn* NativePawn, TArray<float>& OutClearancesCm) const;
    AGTTFarmTrailer* FindAttachedTrailer(const AGTTFieldmasterNativePawn* NativePawn) const;
    float ComputeTrailerSwayRisk(const AGTTFieldmasterNativePawn* NativePawn, const AGTTFarmTrailer* Trailer, float TowLoadFactor) const;
    float ComputeLoadTransferRisk(const AGTTFieldmasterNativePawn* NativePawn, const TArray<float>& ClearancesCm, float TowLoadFactor, float& OutFrontRearBias, float& OutSideBias) const;
    float ComputeStabilityRisk(const AGTTFieldmasterNativePawn* NativePawn, int32 Contacts, float SpeedKmh, float TowLoadFactor, float SwayRisk, float LoadTransferRisk) const;

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, FStabilityState> StabilityStates;
};