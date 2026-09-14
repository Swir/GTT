#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeRolloverSafetySubsystem.generated.h"

class AWheeledVehiclePawn;

USTRUCT(BlueprintType)
struct GTT_API FGTTNativeRolloverSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bActive = false;
    UPROPERTY(BlueprintReadOnly) int32 WheelContacts = 0;
    UPROPERTY(BlueprintReadOnly) float SpeedKmh = 0.0f;
    UPROPERTY(BlueprintReadOnly) float TiltAngleDeg = 0.0f;
    UPROPERTY(BlueprintReadOnly) float CorrectionStrength = 0.0f;
    UPROPERTY(BlueprintReadOnly) float TippedSeconds = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool bEmergencyRighting = false;
};

UCLASS()
class GTT_API UGTTNativeRolloverSafetySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return !IsTemplate(); }

    UFUNCTION(BlueprintPure, Category="GTT|Vehicle|Native|Safety")
    FGTTNativeRolloverSnapshot GetSnapshot(const AWheeledVehiclePawn* Vehicle) const;

private:
    struct FRuntimeState
    {
        FGTTNativeRolloverSnapshot Snapshot;
        float TippedAccumulator = 0.0f;
        float EvidenceCooldown = 0.0f;
        float EmergencyCooldown = 0.0f;
        float RecoveryPulseRemaining = 0.0f;
    };

    void EvaluateVehicle(AWheeledVehiclePawn* Vehicle, bool bTakeoverActive, bool bDriverPresent, float TireIntegrity, float DeltaSeconds);
    int32 CountNativeWheelContacts(AWheeledVehiclePawn* Vehicle) const;
    void EmitEvidence(AWheeledVehiclePawn* Vehicle, FRuntimeState& State) const;

    TMap<TWeakObjectPtr<AWheeledVehiclePawn>, FRuntimeState> RuntimeByVehicle;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float AntiRollStartAngleDeg = 14.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float AntiRollFullAngleDeg = 58.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float AntiRollMinSpeedKmh = 8.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float AntiRollTorque = 145000.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float EmergencyTiltAngleDeg = 105.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float EmergencyMaxSpeedKmh = 4.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float EmergencyArmSeconds = 2.25f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float EmergencyRecoverySeconds = 1.15f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float EmergencyCooldownSeconds = 8.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float EmergencyRightingTorque = 360000.0f;
    UPROPERTY(EditDefaultsOnly, Category="GTT|Vehicle|Native|Safety") float EmergencyLiftForce = 1150.0f;
};
