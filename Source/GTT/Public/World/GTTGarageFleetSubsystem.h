#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTGarageFleetSubsystem.generated.h"

class AGTTRoadVehicleNativePawn;
class AGTTVehicleBase;

UENUM(BlueprintType)
enum class EGTTGarageFleetRole : uint8
{
    Tractor,
    Road,
    Cargo,
    Utility
};

USTRUCT(BlueprintType)
struct GTT_API FGTTGarageFleetSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") int32 SlotIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") FName VehicleId = NAME_None;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") FString DisplayName;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") EGTTGarageFleetRole Role = EGTTGarageFleetRole::Utility;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") bool bPreferredDispatch = false;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") float ConditionPercent = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") float FuelPercent = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") float FuelLiters = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") float FuelCapacityLiters = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") float TireIntegrity = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") float BodyHealth = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") int32 EngineUpgradeLevel = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") int32 TireUpgradeLevel = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") bool bNativeAuthority = false;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") bool bOccupied = false;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") FString ServiceStatus = TEXT("READY");
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") FString NextServiceAction = TEXT("NONE");
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") int32 RepairEstimate = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") int32 TowEstimate = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") int32 FuelEstimate = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") int32 TireServiceEstimate = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") int32 NextEngineUpgradeCost = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") int32 NextTireUpgradeCost = 0;
};

UCLASS()
class GTT_API UGTTGarageFleetSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="GTT|Garage")
    TArray<FGTTGarageFleetSnapshot> BuildFleetSnapshot(int32 MaxSlots = 4) const;

    UFUNCTION(BlueprintCallable, Category="GTT|Garage")
    bool GetSlotSnapshot(int32 SlotIndex, FGTTGarageFleetSnapshot& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category="GTT|Garage")
    FString BuildFleetSummary(int32 MaxSlots = 4) const;

    UFUNCTION(BlueprintPure, Category="GTT|Garage|Dispatch")
    FName GetPreferredVehicleId() const { return PreferredVehicleId; }

    UFUNCTION(BlueprintCallable, Category="GTT|Garage|Dispatch")
    bool SetPreferredVehicleId(FName VehicleId);

    UFUNCTION(BlueprintCallable, Category="GTT|Garage|Dispatch")
    void RestorePreferredVehicleId(FName VehicleId);

    UFUNCTION(BlueprintPure, Category="GTT|Garage|Dispatch")
    FString BuildJobDispatchHint(FName JobTag) const;

    UFUNCTION(BlueprintPure, Category="GTT|Garage|Dispatch")
    bool IsPreferredVehicleFitForJob(FName JobTag) const;

    AGTTVehicleBase* ResolveLegacyVehicleForSlot(int32 SlotIndex) const;
    AGTTRoadVehicleNativePawn* FindActiveNativeRoadVehicle(FName VehicleId) const;

    static EGTTGarageFleetRole ClassifyVehicleRole(FName VehicleId);
    static FString FleetRoleLabel(EGTTGarageFleetRole Role);
    static FName RecommendedVehicleForJob(FName JobTag);

private:
    TArray<AGTTVehicleBase*> GatherOwnedVehicles() const;
    static int32 GetVehicleSortPriority(const AGTTVehicleBase* Vehicle);
    bool IsOwnedFleetVehicle(FName VehicleId) const;

    UPROPERTY(Transient)
    FName PreferredVehicleId = NAME_None;
};
