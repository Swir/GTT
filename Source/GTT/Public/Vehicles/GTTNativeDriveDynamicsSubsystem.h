#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeDriveDynamicsSubsystem.generated.h"

class AGTTFieldmasterNativePawn;

/**
 * Gameplay-facing drivability layer for the Native Chaos Fieldmaster.
 * It translates persistent condition/tire/tuning state into live speed,
 * drag and breakdown consequences without creating a second vehicle state.
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

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, float> EvidenceSeconds;
};
