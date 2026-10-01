#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vehicles/GTTRoadVehicleNativePawn.h"
#include "GTTStructuralDamageEvidenceSubsystem.generated.h"

class AGTTServiceTerminal;

UCLASS()
class GTT_API UGTTStructuralDamageEvidenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTStructuralDamageEvidenceSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return bEnabled && !bFinished; }
    bool DidCompleteSuccessfully() const { return bFinished && bSucceeded; }
    FName GetEvidenceVehicleId() const { return TargetVehicleId; }

private:
    enum class EEvidencePhase : uint8
    {
        WaitForDamageRecovery,
        StageStructuralDamage,
        SaveStructuralState,
        VerifyStructuralReload,
        PrepareWorkshop,
        InvokeWorkshop,
        VerifyWorkshop,
        Complete
    };

    AGTTRoadVehicleNativePawn* FindRoadVehicleById(FName VehicleId) const;
    AGTTServiceTerminal* FindWorkshopTerminal() const;
    void Fail(const FString& Reason);

    bool bEnabled = false;
    bool bFinished = false;
    bool bSucceeded = false;
    float Elapsed = 0.0f;
    float PhaseStartedSeconds = 0.0f;
    EEvidencePhase Phase = EEvidencePhase::WaitForDamageRecovery;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> TargetVehicle;
    FName TargetVehicleId = NAME_None;
    FGTTRoadBodyDamageSnapshot SavedBody;
    int32 SavedPanelMask = 0;
    int32 SavedRepairSurcharge = 0;
    int32 CashBeforeWorkshop = 0;
    int32 ExpectedWorkshopQuote = 0;
};
