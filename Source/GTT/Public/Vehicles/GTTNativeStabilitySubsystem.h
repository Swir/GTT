#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeStabilitySubsystem.generated.h"

class AGTTFarmTrailer;
class AGTTFieldmasterNativePawn;

/**
 * Contact-aware stability and traction controller for the Native Chaos Fieldmaster.
 * It combines authored wheel-ground evidence with heavy-haul load/sway state,
 * axle load-transfer, terrain grade, tire health and chassis slip so unsafe
 * traction loss changes the live Chaos inputs without duplicating vehicle state.
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
        float TractionLossSeconds = 0.0f;
        int32 LastContacts = 4;
        float LastRisk = 0.0f;
        float LastTowLoad = 0.0f;
        float LastSwayRisk = 0.0f;
        float LastLoadTransferRisk = 0.0f;
        float LastTractionRisk = 0.0f;
        float LastSlipAngleDegrees = 0.0f;
        float LastFrontTraction = 1.0f;
        float LastRearTraction = 1.0f;
        float LastFrontRearBias = 0.0f;
        float LastSideBias = 0.0f;
    };

    void EvaluateFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime);
    int32 SampleWheelContacts(AGTTFieldmasterNativePawn* NativePawn, TArray<float>& OutClearancesCm) const;
    AGTTFarmTrailer* FindAttachedTrailer(const AGTTFieldmasterNativePawn* NativePawn) const;
    float ComputeTrailerSwayRisk(const AGTTFieldmasterNativePawn* NativePawn, const AGTTFarmTrailer* Trailer, float TowLoadFactor) const;
    float ComputeLoadTransferRisk(const AGTTFieldmasterNativePawn* NativePawn, const TArray<float>& ClearancesCm, float TowLoadFactor, float& OutFrontRearBias, float& OutSideBias) const;
    float ComputeTractionRisk(const AGTTFieldmasterNativePawn* NativePawn, int32 Contacts, float TowLoadFactor, float FrontRearBias, float SideBias, float& OutSlipAngleDegrees, float& OutFrontTraction, float& OutRearTraction) const;
    float ComputeStabilityRisk(const AGTTFieldmasterNativePawn* NativePawn, int32 Contacts, float SpeedKmh, float TowLoadFactor, float SwayRisk, float LoadTransferRisk, float TractionRisk) const;

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, FStabilityState> StabilityStates;
};
