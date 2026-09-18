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

UENUM(BlueprintType)
enum class EGTTFleetMissionReadiness : uint8
{
    Ready,
    Advisory,
    ServiceRequired,
    Unavailable
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
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage") bool bRoleLoadout = false;
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

USTRUCT(BlueprintType)
struct GTT_API FGTTFleetMissionAssessment
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") FName JobTag = NAME_None;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") EGTTGarageFleetRole RequiredRole = EGTTGarageFleetRole::Utility;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") FName AssignedVehicleId = NAME_None;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") FString AssignedVehicleName;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") int32 AssignedSlot = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") EGTTFleetMissionReadiness Readiness = EGTTFleetMissionReadiness::Unavailable;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") float ConditionPercent = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") float FuelPercent = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") float TireIntegrity = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") float BodyHealth = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") int32 PrepEstimate = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Garage|Mission") FString Reason;
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

    /** Hard recovery/service state used by garage UI and future Blueprint presentation. */
    UFUNCTION(BlueprintPure, Category="GTT|Garage|Service")
    bool IsVehicleOnWorkshopHold(FName VehicleId) const;

    UFUNCTION(BlueprintPure, Category="GTT|Garage|Service")
    int32 GetWorkshopHoldCount(int32 MaxSlots = 4) const;

    UFUNCTION(BlueprintPure, Category="GTT|Garage|Dispatch")
    FName GetPreferredVehicleId() const { return PreferredVehicleId; }

    UFUNCTION(BlueprintCallable, Category="GTT|Garage|Dispatch")
    bool SetPreferredVehicleId(FName VehicleId);

    UFUNCTION(BlueprintCallable, Category="GTT|Garage|Dispatch")
    void RestorePreferredVehicleId(FName VehicleId);

    UFUNCTION(BlueprintPure, Category="GTT|Garage|Loadout")
    FName GetRoleLoadoutVehicleId(EGTTGarageFleetRole Role) const;

    UFUNCTION(BlueprintCallable, Category="GTT|Garage|Loadout")
    bool SetRoleLoadoutVehicleId(EGTTGarageFleetRole Role, FName VehicleId);

    UFUNCTION(BlueprintCallable, Category="GTT|Garage|Loadout")
    void RestoreRoleLoadouts(FName TractorVehicleId, FName RoadVehicleId, FName CargoVehicleId);

    UFUNCTION(BlueprintCallable, Category="GTT|Garage|Mission")
    FGTTFleetMissionAssessment AssessJobReadiness(FName JobTag) const;

    UFUNCTION(BlueprintPure, Category="GTT|Garage|Dispatch")
    FString BuildJobDispatchHint(FName JobTag) const;

    UFUNCTION(BlueprintPure, Category="GTT|Garage|Dispatch")
    bool IsPreferredVehicleFitForJob(FName JobTag) const;

    UFUNCTION(BlueprintPure, Category="GTT|Garage|Mission")
    bool IsJobFleetReady(FName JobTag) const;

    AGTTVehicleBase* ResolveLegacyVehicleForSlot(int32 SlotIndex) const;
    AGTTRoadVehicleNativePawn* FindActiveNativeRoadVehicle(FName VehicleId) const;

    static EGTTGarageFleetRole ClassifyVehicleRole(FName VehicleId);
    static FString FleetRoleLabel(EGTTGarageFleetRole Role);
    static FString MissionReadinessLabel(EGTTFleetMissionReadiness Readiness);
    static FName RecommendedVehicleForJob(FName JobTag);
    static EGTTGarageFleetRole RecommendedRoleForJob(FName JobTag);

private:
    TArray<AGTTVehicleBase*> GatherOwnedVehicles() const;
    static int32 GetVehicleSortPriority(const AGTTVehicleBase* Vehicle);
    bool IsOwnedFleetVehicle(FName VehicleId) const;
    void SeedRoleLoadoutIfNeeded(EGTTGarageFleetRole Role, FName VehicleId);

    UPROPERTY(Transient)
    FName PreferredVehicleId = NAME_None;

    UPROPERTY(Transient)
    FName PreferredTractorVehicleId = NAME_None;

    UPROPERTY(Transient)
    FName PreferredRoadVehicleId = NAME_None;

    UPROPERTY(Transient)
    FName PreferredCargoVehicleId = NAME_None;
};
