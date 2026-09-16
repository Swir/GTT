#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTStructuralDriveConsequenceSubsystem.generated.h"

class AGTTRoadVehicleNativePawn;
class AGTTServiceTerminal;

USTRUCT(BlueprintType)
struct GTT_API FGTTStructuralDriveState
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Structural")
    float DamageSeverity = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Structural")
    float DragRatePerSecond = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Structural")
    float LateralPullRate = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Structural")
    float PowerRetention = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Structural")
    float SteeringRetention = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Structural")
    float CoolingStress = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Structural")
    int32 DetachedPanels = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GTT|Vehicle|Structural")
    bool bLimpHomeActive = false;
};

/**
 * Converts persistent Native road-vehicle body damage into physical, continuous drive consequences.
 * The system deliberately consumes the same structural snapshot that is stored by save schema v5,
 * so a damaged Rattleback/Mulebox stays harder to drive after reload and a paid workshop repair
 * removes the consequence without maintaining a parallel damage model.
 */
UCLASS()
class GTT_API UGTTStructuralDriveConsequenceSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGTTStructuralDriveConsequenceSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return true; }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Structural")
    FGTTStructuralDriveState GetDriveStateForVehicle(const AGTTRoadVehicleNativePawn* Vehicle) const;

private:
    enum class EEvidencePhase : uint8
    {
        WaitForStructuralRecovery,
        StageDamage,
        VerifyDamagedDynamics,
        SaveDamagedDynamics,
        VerifyReloadedDynamics,
        PrepareWorkshop,
        InvokeWorkshop,
        VerifyRecoveredDynamics,
        Complete
    };

    void ApplyDriveConsequences(AGTTRoadVehicleNativePawn* Vehicle, float DeltaTime);
    void TickEvidence(float DeltaTime);
    AGTTRoadVehicleNativePawn* FindActiveOwnedRoadVehicle() const;
    AGTTServiceTerminal* FindWorkshopTerminal() const;
    void FailEvidence(const FString& Reason);

    bool bEvidenceEnabled = false;
    bool bEvidenceFinished = false;
    float EvidenceElapsed = 0.0f;
    float EvidencePhaseStarted = 0.0f;
    float TelemetryAccumulator = 0.0f;
    EEvidencePhase EvidencePhase = EEvidencePhase::WaitForStructuralRecovery;
    TWeakObjectPtr<AGTTRoadVehicleNativePawn> EvidenceVehicle;
    FName EvidenceVehicleId = NAME_None;
    FGTTStructuralDriveState DamagedState;
    int32 CashBeforeWorkshop = 0;
};
