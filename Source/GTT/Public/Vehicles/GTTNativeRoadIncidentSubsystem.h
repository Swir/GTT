#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeRoadIncidentSubsystem.generated.h"

class AGTTRoadVehicleNativePawn;

UCLASS()
class GTT_API UGTTNativeRoadIncidentSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

private:
    void ScanNativeRoadIncidents();

    FTimerHandle IncidentScanTimer;
    TMap<TWeakObjectPtr<AGTTRoadVehicleNativePawn>, int32> LastSeenImpactCounts;
};
