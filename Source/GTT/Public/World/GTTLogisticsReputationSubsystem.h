#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTLogisticsReputationSubsystem.generated.h"

class UGTTSaveGame;

UCLASS()
class GTT_API UGTTLogisticsReputationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    int32 GetReputation() const { return Reputation; }

    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    int32 GetCleanStreak() const { return CleanStreak; }

    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    int32 GetCompletedRuns() const { return CompletedRuns; }

    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    int32 GetFailedRuns() const { return FailedRuns; }

    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    int32 GetLifetimeRevenue() const { return LifetimeRevenue; }

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo")
    int32 GetCargoCompletedRuns() const { return CargoCompletedRuns; }

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo")
    int32 GetCargoFailedRuns() const { return CargoFailedRuns; }

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo")
    int32 GetCargoLifetimeRevenue() const { return CargoLifetimeRevenue; }

    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    FString GetTierLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    bool IsRoadCourierWindowOpen() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    bool IsLateShift() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    FString GetRoadCourierScheduleLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    float GetRoadCourierRewardMultiplier() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics")
    FString GetRoadSupplySignalLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo")
    bool IsCargoDepotWindowOpen() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo")
    FString GetCargoScheduleLabel() const;

    // Reputation capability tier. The actual offered order can step down to a Hill Farm
    // direct run when that buyer has the stronger backlog.
    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo")
    int32 GetCargoRouteTier() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Orders")
    int32 GetActiveCargoOrderTier() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Orders")
    int32 GetCargoOrderUnits() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Orders")
    FString GetCargoOrderPriorityLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Orders")
    FString GetCargoOrderRouteLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Orders")
    int32 GetCargoBacklogPressure() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo")
    float GetCargoMarketMultiplier() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo")
    FString GetCargoMarketLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Market")
    FString GetCargoCommodityLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Market")
    FString GetCargoStockSummary() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Market")
    int32 GetFeedDepotStock() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Market")
    int32 GetHillFarmDemand() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Market")
    int32 GetWoodYardDemand() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Market")
    bool CanAcceptCargoContract() const;

    UFUNCTION(BlueprintCallable, Category="GTT|Logistics|Cargo|Market")
    bool ReserveCargoContract(int32 RouteTier, int32& OutReservedUnits, FString& OutReason);

    UFUNCTION(BlueprintCallable, Category="GTT|Logistics|Cargo|Market")
    void SettleCargoContract(int32 ReservedUnits, bool bExtendedRoute, bool bSuccess);

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|History")
    FString GetRecentHistorySummary() const;

    void RecordCourierSuccess(int32 Payout, float ParcelIntegrity, bool bFastDelivery, bool bPoliceIncident, int32 NativeImpacts);
    void RecordCourierFailure(bool bSevereFailure);
    void RecordCargoSuccess(int32 Payout, float CargoIntegrity, bool bFastDelivery, bool bPoliceIncident, bool bExtendedRoute);
    void RecordCargoFailure(float CargoIntegrity, bool bSevereFailure);

    void CaptureToSave(UGTTSaveGame* Save) const;
    void RestoreFromSave(const UGTTSaveGame* Save);

private:
    float GetTimeOfDayHours() const;
    int32 GetDayNumber() const;
    void AppendHistory(FName ContractTag, int32 Payout, int32 QualityPercent);
    void EnsureCargoMarketForCurrentDay() const;

    int32 Reputation = 0;
    int32 CleanStreak = 0;
    int32 CompletedRuns = 0;
    int32 FailedRuns = 0;
    int32 LifetimeRevenue = 0;
    int32 CargoCompletedRuns = 0;
    int32 CargoFailedRuns = 0;
    int32 CargoLifetimeRevenue = 0;

    // Living-market state. Mutable because read-only board queries can be the first touch
    // after a world-day rollover and therefore lazily apply stock/backlog evolution.
    mutable int32 MarketDay = 0;
    mutable int32 FeedDepotStock = 10;
    mutable int32 HillFarmDemand = 6;
    mutable int32 WoodYardDemand = 4;
    mutable int32 CargoRotationIndex = 0;
    mutable int32 CargoBacklogPressure = 0;

    TArray<FName> RecentContractTags;
    TArray<int32> RecentPayouts;
    TArray<int32> RecentQualityPercent;
};
