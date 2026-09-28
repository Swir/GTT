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
    bool DidCompleteSuccessfully() const { return bEnabled && bFinished && bScenarioPassed; }
    FName GetProvenRoadblockVehicleId() const { return ProvenRoadblockVehicleId; }
private:
    void Pass(const TCHAR* Step);
    void PrepareAcceptanceFleet();
    void DriveNativeRoadblockCrossing();
    bool bEnabled = false;
    bool bFinished = false;
    bool bScenarioPassed = false;
    bool bCrimeInjected = false;
    bool bAcceptanceFleetPrepared = false;
    bool bControlActionLogged = false;
    bool bRoadblockCrossingStaged = false;
    bool bRoadblockTimeoutLogged = false;
    bool bPostSpikeEscapeStarted = false;
    bool bPostSpikeEscapeStaged = false;
    float Elapsed = 0.0f;
    float PursuitStartDistance = -1.0f;
    float RoadblockCrossingStartSeconds = -1.0f;
    float PostSpikeEscapeStartSeconds = -1.0f;
    float PostSpikeStartSpeedCmS = 0.0f;
    FVector PostSpikeStartLocation = FVector::ZeroVector;
    float RoadblockBaselineTires = 1.0f;
    float RoadblockBaselineWheelRisk = 0.0f;
    float RoadblockBaselineThrottleLimit = 1.0f;
    float RoadblockBaselineSteeringLimit = 1.0f;
    TWeakObjectPtr<AGTTPolicePursuitVehicle> ObservedPursuitVehicle;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> RoadblockTestVehicle;
    FName ProvenRoadblockVehicleId = NAME_None;
    TWeakObjectPtr<AGTTRoadblock> RoadblockTestActor;
    TSet<FName> Passed;
};
