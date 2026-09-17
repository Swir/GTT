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

    // The active order is the oldest real stock-backed reservation when one exists, otherwise
    // the player's same-day negotiated choice, then the living market recommendation.
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

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Negotiation")
    TArray<int32> GetCargoNegotiationOptions() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Negotiation")
    FString GetCargoNegotiationOptionsLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Negotiation")
    FString GetCargoNegotiationStatusLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Negotiation")
    bool HasExplicitCargoNegotiation() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Negotiation")
    int32 GetNegotiatedCargoOrderTier() const { return CargoNegotiatedOrderTier; }

    UFUNCTION(BlueprintCallable, Category="GTT|Logistics|Cargo|Negotiation")
    bool CycleCargoNegotiatedOrder(FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Logistics|Cargo|Negotiation")
    void ClearCargoNegotiatedOrder();

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Reservations")
    int32 GetCargoReservationCount() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Reservations")
    int32 GetReservedCargoOrderTier() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Cargo|Reservations")
    FString GetCargoReservationSummary() const;

    // Convert the currently negotiated order into a stock-backed hold. The relationship layer
    // supplies the bounded queue capacity and hold duration; stock is debited immediately so
    // the same scarce pallets cannot be promised twice.
    UFUNCTION(BlueprintCallable, Category="GTT|Logistics|Cargo|Reservations")
    bool ReserveNegotiatedCargoOrder(int32 HoldMinutes, int32 MaxQueue, FString& OutSummary);

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
    void ExpireCargoReservations(int32 CurrentDay, float CurrentHour) const;
    int32 GetRecommendedCargoOrderTier() const;
    bool IsCargoOrderTierAvailableInternal(int32 Tier) const;
    static int32 GetCargoOrderUnitsForTier(int32 Tier);
    FString BuildCargoTierChoiceLabel(int32 Tier) const;

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

    // 0 means "follow the market recommendation". A positive tier is an explicit dispatcher
    // negotiation and only remains authoritative while that tier still has stock, demand and
    // reputation capability on the same world day.
    mutable int32 CargoNegotiatedOrderTier = 0;
    mutable int32 CargoNegotiationDay = 0;

    // 0.1.9 stock-backed dispatcher reservations. A trusted driver can hold up to two real
    // orders. Units are removed from free depot stock immediately, consumed without a second
    // debit when the job starts, and returned if the hold expires before pickup. Expiry also
    // adds backlog pressure, so repeatedly blocking scarce stock has a gameplay consequence.
    mutable int32 CargoReservationDay = 0;
    mutable TArray<int32> CargoReservedOrderTiers;
    mutable TArray<int32> CargoReservedUnits;
    mutable TArray<float> CargoReservationExpiryHours;

    TArray<FName> RecentContractTags;
    TArray<int32> RecentPayouts;
    TArray<int32> RecentQualityPercent;
};
