#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTWorkshopCapacityLifecycleBridgeSubsystem.generated.h"

/**
 * Evidence-only time bridge for the historical 0.1.48 packaged workshop-capacity route.
 *
 * 0.1.49 made queued workshop service consume real world time. The existing 0.1.48 capacity
 * scenario intentionally remains part of the demo technical gate, so when that exact scenario is
 * running this subsystem waits until both evidence vehicles have entered the production IN_SERVICE
 * state, then advances only the evidence world clock to the later persisted service-completion
 * timestamp. Normal gameplay and every other smoke scenario are untouched.
 */
UCLASS()
class GTT_API UGTTWorkshopCapacityLifecycleBridgeSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    bool bEnabled = false;
    bool bAdvanced = false;
    float Elapsed = 0.0f;
};
