#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeRoadIncidentSubsystem.generated.h"

class AGTTRoadVehicleNativePawn;
class AGTTTrafficCarPawn;

UCLASS()
class GTT_API UGTTNativeRoadIncidentSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

private:
    struct FActiveTrafficIncident
    {
        TWeakObjectPtr<AGTTRoadVehicleNativePawn> NativeVehicle;
        TWeakObjectPtr<AGTTTrafficCarPawn> Victim;
        FVector Origin = FVector::ZeroVector;
        float ImpactSpeedKmh = 0.0f;
        float ExpiresAtSeconds = 0.0f;
        bool bHitAndRunEscalated = false;
    };

    void ScanNativeRoadIncidents();
    void UpdateActiveIncidents(float NowSeconds);

    FTimerHandle IncidentScanTimer;
    TMap<TWeakObjectPtr<AGTTRoadVehicleNativePawn>, int32> LastSeenImpactCounts;
    TArray<FActiveTrafficIncident> ActiveIncidents;
};
