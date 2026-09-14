#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeStabilitySubsystem.generated.h"

class AGTTFieldmasterNativePawn;

/**
 * Contact-aware stability controller for the Native Chaos Fieldmaster.
 * It uses the authored wheel-bone contract and real world traces to detect
 * partial wheel contact, dangerous body attitude and rollover risk, then
 * applies conservative throttle/brake intervention without duplicating
 * vehicle condition, tire or tuning state.
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
        int32 LastContacts = 4;
        float LastRisk = 0.0f;
    };

    void EvaluateFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime);
    int32 SampleWheelContacts(AGTTFieldmasterNativePawn* NativePawn, TArray<float>& OutClearancesCm) const;
    float ComputeStabilityRisk(const AGTTFieldmasterNativePawn* NativePawn, int32 Contacts, float SpeedKmh) const;

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, FStabilityState> StabilityStates;
};
