#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTNativeAxleTractionSubsystem.generated.h"

class AWheeledVehiclePawn;
class UChaosWheeledVehicleMovementComponent;

USTRUCT(BlueprintType)
struct GTT_API FGTTNativeAxleTractionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bActive = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ValidWheels = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ContactWheels = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 FrontContacts = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 RearContacts = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 LeftContacts = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 RightContacts = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float FrontSlipRisk = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RearSlipRisk = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float LeftLoadProxy = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float RightLoadProxy = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float AxleImbalance = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float TractionAuthority = 1.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float BrakeAssist = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bTorqueCut = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bSuspensionRuntimeReady = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 SuspensionSamples = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MinSuspensionTravel = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float MaxSuspensionTravel = 0.0f;
};

/**
 * Shared Native Chaos wheel/axle evidence service for the accepted Fieldmaster and road fleet.
 * Samples actual FWheelStatus data and produces bounded torque/brake/steering recommendations.
 * Final throttle/brake/steering/gear composition is owned by UGTTNativeDriveDynamicsSubsystem
 * so multiple safety systems cannot overwrite each other according to subsystem tick order.
 */
UCLASS()
class GTT_API UGTTNativeAxleTractionSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

    UFUNCTION(BlueprintPure, Category="GTT|Chaos|Axle")
    FGTTNativeAxleTractionSnapshot GetSnapshot(const AWheeledVehiclePawn* Vehicle) const;

    /** Same-frame sampler used by the final command composer. It does not mutate movement input. */
    FGTTNativeAxleTractionSnapshot SampleSnapshot(
        UChaosWheeledVehicleMovementComponent* Movement,
        float TireIntegrity,
        int32 TireUpgradeLevel) const;

private:
    struct FRuntimeState
    {
        FGTTNativeAxleTractionSnapshot Snapshot;
        float EvidenceSeconds = 0.0f;
        float InterventionCooldown = 0.0f;
    };

    void EvaluateVehicle(
        AWheeledVehiclePawn* Vehicle,
        UChaosWheeledVehicleMovementComponent* Movement,
        FName VehicleId,
        bool bNativeTakeoverActive,
        bool bDriverPresent,
        float TireIntegrity,
        int32 TireUpgradeLevel,
        float DeltaSeconds);

    TMap<TWeakObjectPtr<AWheeledVehiclePawn>, FRuntimeState> RuntimeByVehicle;
};
