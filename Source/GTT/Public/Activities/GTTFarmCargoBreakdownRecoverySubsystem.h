#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTFarmCargoBreakdownRecoverySubsystem.generated.h"

class APawn;
class AGTTRoadVehicleNativePawn;

UENUM(BlueprintType)
enum class EGTTFarmCargoRecoveryState : uint8
{
    None,
    Healthy,
    Degraded,
    TowRecommended,
    PatchPending,
    Patched,
    TowPending,
    PoliceImpoundPending,
    AwaitingExactVehicle,
    Recovered
};

/**
 * Connects the Farm Cargo exact-vehicle authority to native breakdown/recovery choices.
 *
 * This subsystem never owns the contract, payout, inventory or vehicle identity. Instead it
 * checkpoints the existing primary save around emergency patch/tow/impound actions, verifies
 * that the same PersistentVehicleId remains authoritative afterwards, and keeps delivery blocked
 * while that exact vehicle is absent. The farm-job timer intentionally keeps running throughout.
 */
UCLASS()
class GTT_API UGTTFarmCargoBreakdownRecoverySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob|Recovery")
    EGTTFarmCargoRecoveryState GetRecoveryState() const { return RecoveryState; }

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob|Recovery")
    FName GetRecoveryVehicleId() const { return RecoveryVehicleId; }

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob|Recovery")
    FString GetRecoveryStateLabel() const;

private:
    void EvaluateCargoRecovery();
    void SetRecoveryState(EGTTFarmCargoRecoveryState NewState, APawn* Driver, const FString& PlayerMessage);
    bool CheckpointPrimarySave(const TCHAR* Reason) const;
    bool VerifyExactCargoVehicle(class UGTTFarmCargoAuthoritySubsystem* CargoAuthority, AGTTRoadVehicleNativePawn* ExpectedVehicle, FName ExpectedId) const;
    void ResetRecoveryState();

    EGTTFarmCargoRecoveryState RecoveryState = EGTTFarmCargoRecoveryState::None;
    FName RecoveryVehicleId = NAME_None;
    FVector RecoveryStartLocation = FVector::ZeroVector;
    float EvaluationAccumulator = 0.0f;
    bool bPreRecoveryCheckpointWritten = false;
};
