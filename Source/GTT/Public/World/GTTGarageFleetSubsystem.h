#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTGarageFleetSubsystem.generated.h"

class AGTTRoadVehicleNativePawn;
class AGTTVehicleBase;

USTRUCT(BlueprintType)
struct GTT_API FGTTGarageFleetSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") int32 SlotIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") FName VehicleId = NAME_None;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") FString DisplayName;
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

    AGTTVehicleBase* ResolveLegacyVehicleForSlot(int32 SlotIndex) const;
    AGTTRoadVehicleNativePawn* FindActiveNativeRoadVehicle(FName VehicleId) const;

private:
    TArray<AGTTVehicleBase*> GatherOwnedVehicles() const;
    static int32 GetVehicleSortPriority(const AGTTVehicleBase* Vehicle);
};
