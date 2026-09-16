#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTDamageRecoveryEvidenceSubsystem.generated.h"

class AGTTRoadVehicleNativePawn;
class AGTTVehicleBase;
class AGTTServiceTerminal;

UCLASS()
class GTT_API UGTTDamageRecoveryEvidenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTDamageRecoveryEvidenceSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return bEnabled && !bFinished; }

private:
    enum class EEvidencePhase : uint8
    {
        WaitForCoreScenario,
        SaveDamagedState,
        VerifyLoadRoundTrip,
        SettleReloadedNative,
        InvokeWorkshop,
        VerifyWorkshop,
        Complete
    };

    AGTTRoadVehicleNativePawn* FindDamagedNativeRoadVehicle() const;
    AGTTVehicleBase* FindLegacyVehicle(FName VehicleId) const;
    AGTTServiceTerminal* FindWorkshopTerminal() const;
    void Fail(const FString& Reason);

    bool bEnabled = false;
    bool bFinished = false;
    float Elapsed = 0.0f;
    float PhaseStartedSeconds = 0.0f;
    EEvidencePhase Phase = EEvidencePhase::WaitForCoreScenario;

    TWeakObjectPtr<AGTTRoadVehicleNativePawn> TargetVehicle;
    FName TargetVehicleId = NAME_None;
    float DamagedTireIntegrity = 1.0f;
    float DamagedCondition = 1.0f;
    float DamagedWheelRisk = 0.0f;
    float DamagedThrottleLimit = 1.0f;
    float DamagedSteeringLimit = 1.0f;
    float ReloadedTireIntegrity = 1.0f;
    float ReloadedCondition = 1.0f;
    int32 CashBeforeWorkshop = 0;
    int32 CashAfterWorkshop = 0;
};
