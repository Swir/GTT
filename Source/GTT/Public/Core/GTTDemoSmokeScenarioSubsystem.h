#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTDemoSmokeScenarioSubsystem.generated.h"

class AGTTPolicePursuitVehicle;
class AGTTRoadVehicleNativePawn;
class AGTTRoadblock;

UCLASS()
class GTT_API UGTTDemoSmokeScenarioSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTDemoSmokeScenarioSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return bEnabled && !bFinished; }
private:
    void Pass(const TCHAR* Step);
    void DriveNativeRoadblockCrossing();
    bool bEnabled = false;
    bool bFinished = false;
    bool bCrimeInjected = false;
    bool bControlActionLogged = false;
    bool bRoadblockCrossingStaged = false;
    float Elapsed = 0.0f;
    float PursuitStartDistance = -1.0f;
    float RoadblockCrossingStartSeconds = -1.0f;
    float RoadblockBaselineTires = 1.0f;
    float RoadblockBaselineWheelRisk = 0.0f;
    TWeakObjectPtr<AGTTPolicePursuitVehicle> ObservedPursuitVehicle;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> RoadblockTestVehicle;
    TWeakObjectPtr<AGTTRoadblock> RoadblockTestActor;
    TSet<FName> Passed;
};
