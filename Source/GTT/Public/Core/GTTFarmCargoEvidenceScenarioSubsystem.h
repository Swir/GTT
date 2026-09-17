#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTFarmCargoEvidenceScenarioSubsystem.generated.h"

class AGTTFarmJobDirector;
class AGTTFarmJobTerminal;
class AGTTMuleboxNativePawn;
class APawn;

UCLASS()
class GTT_API UGTTFarmCargoEvidenceScenarioSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTFarmCargoEvidenceScenarioSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return bEnabled && !bFinished; }

private:
    void Pass(const TCHAR* Step);
    void Fail(const FString& Reason);
    bool ResolveActors();
    void StageVehicleAt(const FVector& Location);
    void CompleteScenario();

    bool bEnabled = false;
    bool bFinished = false;
    bool bEnvironmentPrepared = false;
    int32 Step = 0;
    float Elapsed = 0.0f;
    float StepStartedAt = 0.0f;
    int32 CashBefore = 0;
    int32 CargoRunsBefore = 0;
    FVector PickupLocation = FVector::ZeroVector;
    FVector HillFarmLocation = FVector::ZeroVector;
    FVector FinalStopLocation = FVector::ZeroVector;
    bool bHasPickup = false;
    bool bHasHillFarm = false;
    bool bHasFinalStop = false;
    TWeakObjectPtr<APawn> OriginalPlayerPawn;
    TWeakObjectPtr<AGTTFarmJobDirector> FarmDirector;
    TWeakObjectPtr<AGTTFarmJobTerminal> PickupTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> HillFarmTerminal;
    TWeakObjectPtr<AGTTFarmJobTerminal> FinalStopTerminal;
    TWeakObjectPtr<AGTTMuleboxNativePawn> CargoVehicle;
    TSet<FName> Passed;
};
