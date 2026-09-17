#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeRuntimeTelemetrySubsystem.generated.h"

class AGTTFieldmasterNativePawn;

/**
 * Packaged-runtime telemetry for the accepted Fieldmaster Chaos path.
 *
 * This subsystem deliberately samples the live Chaos movement component rather
 * than mirroring source configuration. The resulting log contract is consumed
 * by the Win64 evidence pipeline so a source-only CI pass cannot stand in for
 * actual wheel/contact/suspension/drivetrain execution.
 */
UCLASS()
class GTT_API UGTTNativeRuntimeTelemetrySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    void SampleFieldmaster(AGTTFieldmasterNativePawn* Vehicle, float DeltaSeconds);

    TMap<TWeakObjectPtr<AGTTFieldmasterNativePawn>, float> EvidenceSecondsByVehicle;
};
