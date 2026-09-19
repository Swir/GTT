#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTWorkshopLegacyEvidencePickupBridgeSubsystem.generated.h"

/**
 * Evidence-only compatibility bridge for pre-0.1.51 packaged workshop scenarios.
 *
 * Older queue/capacity runtime evidence correctly expected a successfully paid service to leave
 * no appointment sidecar. Normal 0.1.51 gameplay now keeps paid work READY_FOR_PICKUP instead.
 * This bridge is enabled only by the historical demo-smoke command-line scenarios and performs
 * the new production exact-ID pickup release after those scenarios finish their real service path.
 * It is never active in ordinary gameplay and owns no cash or repair mutation.
 */
UCLASS()
class GTT_API UGTTWorkshopLegacyEvidencePickupBridgeSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    bool bEnabled = false;
    float TickAccumulator = 0.0f;
};
