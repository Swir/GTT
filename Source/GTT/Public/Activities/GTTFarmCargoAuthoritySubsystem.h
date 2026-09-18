#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTFarmCargoAuthoritySubsystem.generated.h"

class UGTTSaveGame;

/**
 * Owns the identity of the physical vehicle that accepted a Farm Cargo load.
 *
 * The farm-job director remains authoritative for contract stage, payout, reputation and
 * cargo condition. This subsystem only hardens the physical handoff: the vehicle that was
 * actually loaded at Feed Depot must be the vehicle present and stopped at each buyer.
 */
UCLASS()
class GTT_API UGTTFarmCargoAuthoritySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob|CargoAuthority")
    bool BindLoadedVehicle(APawn* PlayerPawn, FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|FarmJob|CargoAuthority")
    bool ValidateHandoff(const FVector& HandoffLocation, float MaxDistanceCm, float MaxSpeedKmh, FString& OutReason);

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob|CargoAuthority")
    bool HasBoundCargoVehicle() const { return BoundCargoVehicle.IsValid(); }

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob|CargoAuthority")
    FName GetBoundCargoVehicleId() const { return BoundCargoVehicleId; }

    UFUNCTION(BlueprintPure, Category="GTT|FarmJob|CargoAuthority")
    APawn* GetBoundCargoVehicle() const { return BoundCargoVehicle.Get(); }

    // 0.1.30: keep stable identity in the existing primary save and resolve a replacement
    // actor after load/garage recovery. A different nearby vehicle never inherits the load.
    void CaptureToSave(UGTTSaveGame* Save) const;
    void RestoreFromSave(const UGTTSaveGame* Save);
    bool TryRebindBoundVehicle();

    void ClearLoadedVehicle(const TCHAR* Reason);

private:
    APawn* ResolveVehicleLoadedAtDepot(APawn* PlayerPawn) const;
    APawn* ResolveVehicleByPersistentId(FName VehicleId) const;
    static FName ResolvePersistentVehicleId(const APawn* Vehicle);

    TWeakObjectPtr<APawn> BoundCargoVehicle;
    FName BoundCargoVehicleId = NAME_None;
    bool bObservedLoadedContract = false;
};
