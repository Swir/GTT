#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativePhysicsAcceptanceSubsystem.generated.h"

class AGTTFieldmasterNativePawn;

/**
 * Runtime acceptance layer for the Native Chaos Fieldmaster.
 * It validates authored rig geometry/Physics Asset presence and samples four
 * ground probes while takeover is live. Sustained invalid authored physics
 * falls back to the legacy tractor instead of leaving the player in a broken pawn.
 */
UCLASS()
class GTT_API UGTTNativePhysicsAcceptanceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    struct FAcceptanceState
    {
        float EvidenceSeconds = 0.0f;
        float InvalidSeconds = 0.0f;
        int32 LastGroundContacts = 0;
    };

    void EvaluateNativeFieldmaster(AGTTFieldmasterNativePawn* NativePawn, float DeltaTime);
    bool ValidateAuthoredPhysics(AGTTFieldmasterNativePawn* NativePawn, FString& OutReason) const;
    int32 SampleGroundContacts(AGTTFieldmasterNativePawn* NativePawn, TArray<float>& OutClearancesCm) const;

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, FAcceptanceState> AcceptanceStates;
};
