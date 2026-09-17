#include "World/GTTLogisticsReputationSubsystem.h"

#include "Core/GTTGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Save/GTTSaveGame.h"
#include "World/GTTDayNightCycle.h"

namespace
{
    constexpr int32 LogisticsSaveVersion = 8;
    constexpr float CourierOpenHour = 6.0f;
    constexpr float CourierCloseHour = 21.5f;
    constexpr float LateShiftHour = 18.5f;
    // Matches the existing citizen work schedule (07:00-17:30): cargo counters are staffed while rural NPCs are at work.
    constexpr float CargoOpenHour = 7.0f;
    constexpr float CargoCloseHour = 17.5f;
    constexpr int32 MaxRecentContracts = 6;
    constexpr int32 MaxFeedDepotStock = 18;
    constexpr int32 MaxHillFarmDemand = 12;
    constexpr int32 MaxWoodYardDemand = 10;
    constexpr int32 MaxCargoBacklogPressure = 6;
}

float UGTTLogisticsReputationSubsystem::GetTimeOfDayHours() const
{
    const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const AGTTDayNightCycle* Cycle = GameMode ? GameMode->GetDayNightCycle() : nullptr;
    return Cycle ? Cycle->GetTimeOfDayHours() : 12.0f;
}

int32 UGTTLogisticsReputationSubsystem::GetDayNumber() const
{
    const AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this));
    const AGTTDayNightCycle* Cycle = GameMode ? GameMode->GetDayNightCycle() : nullptr;
    return Cycle ? FMath::Max(1, Cycle->GetDayNumber()) : 1;
}

void UGTTLogisticsReputationSubsystem::EnsureCargoMarketForCurrentDay() const
{
    const int32 CurrentDay = GetDayNumber();

    // Negotiated orders are same-shift commitments. A saved choice from an earlier world day
    // must not silently override a fresh stock/demand picture after the next rural restock.
    if (CargoNegotiationDay > 0 && CargoNegotiationDay != CurrentDay)
    {
        CargoNegotiatedOrderTier = 0;
        CargoNegotiationDay = 0;
    }

    if (MarketDay == CurrentDay) return;

    // New/default v8 profiles already carry sensible opening stock and demand. Stamp the
    // first observed world day without manufacturing an extra overnight cycle on load.
    if (MarketDay <= 0)
    {
        CargoRotationIndex = (CurrentDay + CargoCompletedRuns + CompletedRuns) % 3;
        MarketDay = CurrentDay;
        return;
    }

    const int32 ElapsedDays = FMath::Clamp(CurrentDay - MarketDay, 1, 7);
    const int32 Restock = 3 * ElapsedDays + ((CurrentDay + CargoCompletedRuns) % 4);
    FeedDepotStock = FMath::Clamp(FeedDepotStock + Restock, 0, MaxFeedDepotStock);

    // 0.1.6 changes demand from a daily reset into a real backlog. Unserved demand survives
    // midnight, each work day adds deterministic fresh orders, and yesterday's failed CARGO
    // raises tomorrow's pressure before gradually decaying. Successful deliveries are the
    // only way to actively pull the destination counters back down.
    const int32 FailureCarry = FMath::Min(4, CargoBacklogPressure);
    const int32 HillFreshOrders = ElapsedDays * (1 + ((CurrentDay + CompletedRuns) % 2));
    const int32 WoodFreshOrders = ElapsedDays + ((CurrentDay * 2 + CargoCompletedRuns + FailedRuns) % 2);
    HillFarmDemand = FMath::Clamp(HillFarmDemand + HillFreshOrders + (FailureCarry + 1) / 2, 0, MaxHillFarmDemand);
    WoodYardDemand = FMath::Clamp(WoodYardDemand + WoodFreshOrders + FailureCarry / 2, 0, MaxWoodYardDemand);
    CargoBacklogPressure = FMath::Max(0, CargoBacklogPressure - ElapsedDays);
    CargoRotationIndex = (CurrentDay + CargoCompletedRuns + CompletedRuns) % 3;
    MarketDay = CurrentDay;
}

FString UGTTLogisticsReputationSubsystem::GetTierLabel() const
{
    if (Reputation >= 80) return TEXT("COUNTY PRO");
    if (Reputation >= 50) return TEXT("RELIABLE");
    if (Reputation >= 20) return TEXT("TRUSTED");
    return TEXT("NEWCOMER");
}

bool UGTTLogisticsReputationSubsystem::IsRoadCourierWindowOpen() const
{
    const float Hour = GetTimeOfDayHours();
    return Hour >= CourierOpenHour && Hour < CourierCloseHour;
}

bool UGTTLogisticsReputationSubsystem::IsLateShift() const
{
    const float Hour = GetTimeOfDayHours();
    return Hour >= LateShiftHour && Hour < CourierCloseHour;
}

FString UGTTLogisticsReputationSubsystem::GetRoadCourierScheduleLabel() const
{
    if (!IsRoadCourierWindowOpen()) return TEXT("CLOSED 21:30-06:00");
    if (IsLateShift()) return TEXT("LATE SHIFT +10%");
    return TEXT("OPEN 06:00-21:30");
}

float UGTTLogisticsReputationSubsystem::GetRoadCourierRewardMultiplier() const
{
    EnsureCargoMarketForCurrentDay();
    const float ReputationBonus = FMath::Min(0.20f, static_cast<float>(Reputation) * 0.002f);
    const float StreakBonus = FMath::Min(0.05f, static_cast<float>(CleanStreak) * 0.01f);
    const float ShiftMultiplier = IsLateShift() ? 1.10f : 1.0f;
    // When the rotating market is FARM PARTS, unresolved Wood Yard demand also creates a
    // genuine same-world incentive for the smaller/faster Rattleback courier route.
    const float PartsPressureBonus = CargoRotationIndex == 2
        ? FMath::Min(0.10f, static_cast<float>(WoodYardDemand) * 0.01f + static_cast<float>(CargoBacklogPressure) * 0.01f)
        : 0.0f;
    return (1.0f + ReputationBonus + StreakBonus + PartsPressureBonus) * ShiftMultiplier;
}

FString UGTTLogisticsReputationSubsystem::GetRoadSupplySignalLabel() const
{
    EnsureCargoMarketForCurrentDay();
    if (CargoRotationIndex != 2) return TEXT("STANDARD PARTS ROUTE");
    const int32 Bonus = FMath::RoundToInt(FMath::Min(0.10f,
        static_cast<float>(WoodYardDemand) * 0.01f + static_cast<float>(CargoBacklogPressure) * 0.01f) * 100.0f);
    if (Bonus <= 0) return TEXT("PARTS DEMAND SATISFIED");
    return FString::Printf(TEXT("WOOD PARTS BACKLOG +%d%%"), Bonus);
}

bool UGTTLogisticsReputationSubsystem::IsCargoDepotWindowOpen() const
{
    const float Hour = GetTimeOfDayHours();
    return Hour >= CargoOpenHour && Hour < CargoCloseHour;
}

FString UGTTLogisticsReputationSubsystem::GetCargoScheduleLabel() const
{
    return IsCargoDepotWindowOpen() ? TEXT("STAFFED 07:00-17:30") : TEXT("CLOSED 17:30-07:00");
}

int32 UGTTLogisticsReputationSubsystem::GetCargoRouteTier() const
{
    if (Reputation >= 50) return 3;
    if (Reputation >= 20) return 2;
    return 1;
}

int32 UGTTLogisticsReputationSubsystem::GetCargoOrderUnitsForTier(int32 Tier)
{
    if (Tier >= 3) return 4;
    if (Tier >= 2) return 3;
    return 2;
}

bool UGTTLogisticsReputationSubsystem::IsCargoOrderTierAvailableInternal(int32 Tier) const
{
    if (Tier < 1 || Tier > GetCargoRouteTier()) return false;
    const int32 RequiredStock = GetCargoOrderUnitsForTier(Tier);
    if (FeedDepotStock < RequiredStock || HillFarmDemand <= 0) return false;
    if (Tier >= 2 && WoodYardDemand <= 0) return false;
    return true;
}

int32 UGTTLogisticsReputationSubsystem::GetRecommendedCargoOrderTier() const
{
    const int32 CapabilityTier = GetCargoRouteTier();
    if (CapabilityTier <= 1 || WoodYardDemand <= 0) return 1;

    // A heavy Hill Farm backlog can pull an experienced driver back onto a short direct
    // order instead of forcing every TRUSTED+ contract into the same relay shape.
    if (HillFarmDemand >= WoodYardDemand + 3 && HillFarmDemand >= 5) return 1;

    if (CapabilityTier >= 3 &&
        (CargoBacklogPressure >= 2 || WoodYardDemand >= 7 || (WoodYardDemand >= HillFarmDemand && FeedDepotStock >= 4)))
    {
        return 3;
    }
    return 2;
}

int32 UGTTLogisticsReputationSubsystem::GetActiveCargoOrderTier() const
{
    EnsureCargoMarketForCurrentDay();

    if (CargoNegotiationDay == GetDayNumber() && CargoNegotiatedOrderTier > 0)
    {
        if (IsCargoOrderTierAvailableInternal(CargoNegotiatedOrderTier))
        {
            return CargoNegotiatedOrderTier;
        }

        // Stock/demand can change after another completed load. Never keep presenting a
        // negotiated tier that the same authoritative market can no longer fulfill.
        CargoNegotiatedOrderTier = 0;
        CargoNegotiationDay = 0;
    }

    const int32 RecommendedTier = GetRecommendedCargoOrderTier();
    if (IsCargoOrderTierAvailableInternal(RecommendedTier)) return RecommendedTier;
    for (int32 Tier = FMath::Min(GetCargoRouteTier(), 3); Tier >= 1; --Tier)
    {
        if (IsCargoOrderTierAvailableInternal(Tier)) return Tier;
    }
    return RecommendedTier;
}

int32 UGTTLogisticsReputationSubsystem::GetCargoOrderUnits() const
{
    switch (GetActiveCargoOrderTier())
    {
        case 3: return 4;
        case 2: return 3;
        default: return 2;
    }
}

FString UGTTLogisticsReputationSubsystem::GetCargoOrderPriorityLabel() const
{
    EnsureCargoMarketForCurrentDay();
    const int32 Tier = GetActiveCargoOrderTier();
    if (Tier >= 3) return CargoBacklogPressure >= 2 ? TEXT("BACKLOG RECOVERY") : TEXT("BULK WOOD ORDER");
    if (Tier >= 2) return WoodYardDemand >= HillFarmDemand ? TEXT("WOOD YARD PRIORITY") : TEXT("BALANCED RELAY");
    return HillFarmDemand >= 8 ? TEXT("HILL FARM URGENT") : TEXT("HILL FARM DIRECT");
}

FString UGTTLogisticsReputationSubsystem::GetCargoOrderRouteLabel() const
{
    const int32 Tier = GetActiveCargoOrderTier();
    if (Tier >= 3) return TEXT("FEED DEPOT -> HILL FARM -> NORTH WOOD YARD | BULK");
    if (Tier >= 2) return TEXT("FEED DEPOT -> HILL FARM -> NORTH WOOD YARD");
    return TEXT("FEED DEPOT -> HILL FARM");
}

int32 UGTTLogisticsReputationSubsystem::GetCargoBacklogPressure() const
{
    EnsureCargoMarketForCurrentDay();
    return CargoBacklogPressure;
}

TArray<int32> UGTTLogisticsReputationSubsystem::GetCargoNegotiationOptions() const
{
    EnsureCargoMarketForCurrentDay();
    TArray<int32> Options;
    const int32 CapabilityTier = FMath::Clamp(GetCargoRouteTier(), 1, 3);
    for (int32 Tier = 1; Tier <= CapabilityTier; ++Tier)
    {
        if (IsCargoOrderTierAvailableInternal(Tier)) Options.Add(Tier);
    }
    return Options;
}

FString UGTTLogisticsReputationSubsystem::GetCargoNegotiationOptionsLabel() const
{
    const TArray<int32> Options = GetCargoNegotiationOptions();
    if (Options.Num() <= 0) return TEXT("NO OPEN OPTIONS");

    TArray<FString> Labels;
    Labels.Reserve(Options.Num());
    for (const int32 Tier : Options) Labels.Add(FString::Printf(TEXT("T%d"), Tier));
    return FString::Printf(TEXT("OPTIONS %s"), *FString::Join(Labels, TEXT("/")));
}

bool UGTTLogisticsReputationSubsystem::HasExplicitCargoNegotiation() const
{
    EnsureCargoMarketForCurrentDay();
    return CargoNegotiationDay == GetDayNumber() && CargoNegotiatedOrderTier > 0 &&
        IsCargoOrderTierAvailableInternal(CargoNegotiatedOrderTier);
}

FString UGTTLogisticsReputationSubsystem::GetCargoNegotiationStatusLabel() const
{
    EnsureCargoMarketForCurrentDay();
    const int32 MarketTier = GetRecommendedCargoOrderTier();
    const int32 ActiveTier = GetActiveCargoOrderTier();
    return FString::Printf(TEXT("%s T%d | MARKET T%d | %s"),
        HasExplicitCargoNegotiation() ? TEXT("NEGOTIATED") : TEXT("AUTO"),
        ActiveTier,
        MarketTier,
        *GetCargoNegotiationOptionsLabel());
}

FString UGTTLogisticsReputationSubsystem::BuildCargoTierChoiceLabel(int32 Tier) const
{
    if (Tier >= 3)
    {
        return TEXT("T3 BULK | 4 units | North Wood chain | +$120 route bonus | HEAVY 1.20x load");
    }
    if (Tier >= 2)
    {
        return TEXT("T2 RELAY | 3 units | Hill + Wood chain | +$70 route bonus | MEDIUM load");
    }
    return TEXT("T1 DIRECT | 2 units | Hill Farm | quickest route | LOW load");
}

bool UGTTLogisticsReputationSubsystem::CycleCargoNegotiatedOrder(FString& OutSummary)
{
    EnsureCargoMarketForCurrentDay();
    const TArray<int32> Options = GetCargoNegotiationOptions();
    if (Options.Num() <= 0)
    {
        OutSummary = FString::Printf(TEXT("No negotiable CARGO order is currently fulfillable: %s."), *GetCargoStockSummary());
        return false;
    }

    const int32 CurrentTier = GetActiveCargoOrderTier();
    int32 CurrentIndex = Options.IndexOfByKey(CurrentTier);
    if (CurrentIndex == INDEX_NONE) CurrentIndex = 0;
    const int32 NextIndex = (CurrentIndex + 1) % Options.Num();
    CargoNegotiatedOrderTier = Options[NextIndex];
    CargoNegotiationDay = GetDayNumber();

    const int32 MarketTier = GetRecommendedCargoOrderTier();
    OutSummary = FString::Printf(TEXT("DISPATCHER DEAL: %s | market recommends T%d | %s. Press interact again to cycle."),
        *BuildCargoTierChoiceLabel(CargoNegotiatedOrderTier), MarketTier, *GetCargoNegotiationOptionsLabel());
    return true;
}

void UGTTLogisticsReputationSubsystem::ClearCargoNegotiatedOrder()
{
    CargoNegotiatedOrderTier = 0;
    CargoNegotiationDay = 0;
}

FString UGTTLogisticsReputationSubsystem::GetCargoCommodityLabel() const
{
    EnsureCargoMarketForCurrentDay();
    switch (CargoRotationIndex)
    {
        case 1: return TEXT("SEED PALLETS");
        case 2: return TEXT("FARM PARTS");
        default: return TEXT("ANIMAL FEED");
    }
}

int32 UGTTLogisticsReputationSubsystem::GetFeedDepotStock() const
{
    EnsureCargoMarketForCurrentDay();
    return FeedDepotStock;
}

int32 UGTTLogisticsReputationSubsystem::GetHillFarmDemand() const
{
    EnsureCargoMarketForCurrentDay();
    return HillFarmDemand;
}

int32 UGTTLogisticsReputationSubsystem::GetWoodYardDemand() const
{
    EnsureCargoMarketForCurrentDay();
    return WoodYardDemand;
}

FString UGTTLogisticsReputationSubsystem::GetCargoStockSummary() const
{
    EnsureCargoMarketForCurrentDay();
    return FString::Printf(TEXT("%s | DEPOT %d | HILL NEED %d | WOOD NEED %d | BACKLOG %d"),
        *GetCargoCommodityLabel(), FeedDepotStock, HillFarmDemand, WoodYardDemand, CargoBacklogPressure);
}

bool UGTTLogisticsReputationSubsystem::CanAcceptCargoContract() const
{
    EnsureCargoMarketForCurrentDay();
    const int32 RouteTier = GetActiveCargoOrderTier();
    const int32 RequiredStock = GetCargoOrderUnits();
    if (FeedDepotStock < RequiredStock || HillFarmDemand <= 0) return false;
    if (RouteTier >= 2 && WoodYardDemand <= 0) return false;
    return true;
}

bool UGTTLogisticsReputationSubsystem::ReserveCargoContract(int32 RouteTier, int32& OutReservedUnits, FString& OutReason)
{
    EnsureCargoMarketForCurrentDay();
    OutReservedUnits = 0;
    OutReason.Reset();

    // Keep the original T1/T2 reservation contract explicit for old source verifiers, then
    // let the new T3 bulk order consume one additional pallet.
    int32 RequiredStock = RouteTier >= 2 ? 3 : 2;
    if (RouteTier >= 3) RequiredStock = 4;
    if (FeedDepotStock < RequiredStock)
    {
        OutReason = FString::Printf(TEXT("Feed Depot only has %d load units; this route needs %d. The next daily restock may reopen it."), FeedDepotStock, RequiredStock);
        return false;
    }
    if (HillFarmDemand <= 0)
    {
        OutReason = TEXT("Hill Farm demand is already satisfied. Wait for fresh rural orders.");
        return false;
    }
    if (RouteTier >= 2 && WoodYardDemand <= 0)
    {
        OutReason = TEXT("North Wood Yard has no remaining demand for the extended chain; the board will rotate to a direct Hill order if available.");
        return false;
    }

    FeedDepotStock -= RequiredStock;
    OutReservedUnits = RequiredStock;
    OutReason = FString::Printf(TEXT("Reserved %d units of %s | %s | depot stock now %d."),
        RequiredStock, *GetCargoCommodityLabel(), *GetCargoOrderPriorityLabel(), FeedDepotStock);
    return true;
}

void UGTTLogisticsReputationSubsystem::SettleCargoContract(int32 ReservedUnits, bool bExtendedRoute, bool bSuccess)
{
    EnsureCargoMarketForCurrentDay();
    if (!bSuccess || ReservedUnits <= 0) return; // failed cargo is lost; buyer demand remains open.

    if (bExtendedRoute)
    {
        const int32 HillUnits = FMath::Max(1, ReservedUnits / 2);
        const int32 WoodUnits = FMath::Max(1, ReservedUnits - HillUnits);
        HillFarmDemand = FMath::Max(0, HillFarmDemand - HillUnits);
        WoodYardDemand = FMath::Max(0, WoodYardDemand - WoodUnits);
    }
    else
    {
        HillFarmDemand = FMath::Max(0, HillFarmDemand - ReservedUnits);
    }
}

float UGTTLogisticsReputationSubsystem::GetCargoMarketMultiplier() const
{
    EnsureCargoMarketForCurrentDay();
    const float Hour = GetTimeOfDayHours();
    const float TimeDemandBonus = Hour < 10.0f ? 0.12f : (Hour >= 14.0f ? 0.08f : 0.0f);
    const int32 DemandCycle = (GetDayNumber() + CargoCompletedRuns + CompletedRuns) % 3;
    const float CycleBonus = DemandCycle == 0 ? 0.06f : (DemandCycle == 1 ? 0.03f : 0.0f);
    const float DemandPressure = FMath::Clamp(static_cast<float>(HillFarmDemand + WoodYardDemand - FeedDepotStock) / 18.0f, 0.0f, 1.0f);
    const float CommodityBonus = CargoRotationIndex == 2 ? 0.04f : (CargoRotationIndex == 1 ? 0.02f : 0.0f);
    const float DailyDemandBonus = FMath::Min(0.10f, CycleBonus + DemandPressure * 0.05f + CommodityBonus);
    const float ReputationBonus = FMath::Min(0.12f, static_cast<float>(Reputation) * 0.0015f);
    const float StreakBonus = FMath::Min(0.04f, static_cast<float>(CleanStreak) * 0.008f);
    const float BaseMarketMultiplier = FMath::Clamp(1.0f + TimeDemandBonus + DailyDemandBonus + ReputationBonus + StreakBonus, 1.0f, 1.38f);
    const float BacklogBonus = FMath::Min(0.10f, static_cast<float>(CargoBacklogPressure) * 0.02f);
    return FMath::Clamp(BaseMarketMultiplier + BacklogBonus, 1.0f, 1.48f);
}

FString UGTTLogisticsReputationSubsystem::GetCargoMarketLabel() const
{
    EnsureCargoMarketForCurrentDay();
    if (!IsCargoDepotWindowOpen()) return FString::Printf(TEXT("MARKET PAUSED | %s | %s"), *GetCargoCommodityLabel(), *GetCargoOrderPriorityLabel());
    const float Hour = GetTimeOfDayHours();
    const FString Demand = Hour < 10.0f ? TEXT("MORNING RUSH") : (Hour >= 14.0f ? TEXT("LATE FEED DEMAND") : TEXT("STEADY DEMAND"));
    const int32 Bonus = FMath::RoundToInt((GetCargoMarketMultiplier() - 1.0f) * 100.0f);
    return FString::Printf(TEXT("%s +%d%% | ORDER T%d | %s | %s"),
        *Demand, Bonus, GetActiveCargoOrderTier(), *GetCargoOrderPriorityLabel(), *GetCargoCommodityLabel());
}

FString UGTTLogisticsReputationSubsystem::GetRecentHistorySummary() const
{
    if (RecentContractTags.Num() <= 0) return TEXT("NO DELIVERY HISTORY");
    const int32 Last = RecentContractTags.Num() - 1;
    return FString::Printf(TEXT("LAST %s | $%d | Q%d%%"),
        *RecentContractTags[Last].ToString().ToUpper(),
        RecentPayouts.IsValidIndex(Last) ? RecentPayouts[Last] : 0,
        RecentQualityPercent.IsValidIndex(Last) ? RecentQualityPercent[Last] : 0);
}

void UGTTLogisticsReputationSubsystem::AppendHistory(FName ContractTag, int32 Payout, int32 QualityPercent)
{
    RecentContractTags.Add(ContractTag);
    RecentPayouts.Add(FMath::Max(0, Payout));
    RecentQualityPercent.Add(FMath::Clamp(QualityPercent, 0, 100));
    while (RecentContractTags.Num() > MaxRecentContracts)
    {
        RecentContractTags.RemoveAt(0);
        RecentPayouts.RemoveAt(0);
        RecentQualityPercent.RemoveAt(0);
    }
}

void UGTTLogisticsReputationSubsystem::RecordCourierSuccess(
    int32 Payout,
    float ParcelIntegrity,
    bool bFastDelivery,
    bool bPoliceIncident,
    int32 NativeImpacts)
{
    const bool bClean = ParcelIntegrity >= 0.97f && !bPoliceIncident && NativeImpacts <= 0;
    int32 ReputationDelta = bClean ? 8 + (bFastDelivery ? 2 : 0) : (ParcelIntegrity >= 0.75f ? 4 : 1);
    if (bPoliceIncident) ReputationDelta -= 3;

    Reputation = FMath::Clamp(Reputation + ReputationDelta, 0, 100);
    CleanStreak = bClean ? FMath::Min(CleanStreak + 1, 99) : 0;
    ++CompletedRuns;
    LifetimeRevenue = FMath::Max(0, LifetimeRevenue + FMath::Max(0, Payout));
    AppendHistory(FName(TEXT("RoadRun")), Payout, FMath::RoundToInt(ParcelIntegrity * 100.0f));
}

void UGTTLogisticsReputationSubsystem::RecordCourierFailure(bool bSevereFailure)
{
    Reputation = FMath::Clamp(Reputation - (bSevereFailure ? 12 : 8), 0, 100);
    CleanStreak = 0;
    ++FailedRuns;
    AppendHistory(FName(TEXT("RoadRunFail")), 0, 0);
}

void UGTTLogisticsReputationSubsystem::RecordCargoSuccess(
    int32 Payout,
    float CargoIntegrity,
    bool bFastDelivery,
    bool bPoliceIncident,
    bool bExtendedRoute)
{
    const bool bClean = CargoIntegrity >= 0.94f && !bPoliceIncident;
    int32 ReputationDelta = bClean ? 6 + (bFastDelivery ? 2 : 0) : (CargoIntegrity >= 0.70f ? 3 : 1);
    if (bExtendedRoute) ReputationDelta += 2;
    if (bPoliceIncident) ReputationDelta -= 3;

    Reputation = FMath::Clamp(Reputation + ReputationDelta, 0, 100);
    CleanStreak = bClean ? FMath::Min(CleanStreak + 1, 99) : 0;
    ++CompletedRuns;
    ++CargoCompletedRuns;
    LifetimeRevenue = FMath::Max(0, LifetimeRevenue + FMath::Max(0, Payout));
    CargoLifetimeRevenue = FMath::Max(0, CargoLifetimeRevenue + FMath::Max(0, Payout));
    CargoBacklogPressure = FMath::Max(0, CargoBacklogPressure - (bExtendedRoute ? 2 : 1));
    AppendHistory(bExtendedRoute ? FName(TEXT("CargoChain")) : FName(TEXT("FarmCargo")), Payout, FMath::RoundToInt(CargoIntegrity * 100.0f));
    ClearCargoNegotiatedOrder();
}

void UGTTLogisticsReputationSubsystem::RecordCargoFailure(float CargoIntegrity, bool bSevereFailure)
{
    Reputation = FMath::Clamp(Reputation - (bSevereFailure ? 10 : 6), 0, 100);
    CleanStreak = 0;
    ++FailedRuns;
    ++CargoFailedRuns;
    CargoBacklogPressure = FMath::Clamp(CargoBacklogPressure + (bSevereFailure ? 2 : 1), 0, MaxCargoBacklogPressure);
    AppendHistory(FName(TEXT("CargoFail")), 0, FMath::RoundToInt(CargoIntegrity * 100.0f));
    ClearCargoNegotiatedOrder();
}

void UGTTLogisticsReputationSubsystem::CaptureToSave(UGTTSaveGame* Save) const
{
    if (!Save) return;
    EnsureCargoMarketForCurrentDay();
    Save->LogisticsReputation = Reputation;
    Save->LogisticsCleanStreak = CleanStreak;
    Save->LogisticsCompletedRuns = CompletedRuns;
    Save->LogisticsFailedRuns = FailedRuns;
    Save->LogisticsLifetimeRevenue = LifetimeRevenue;
    Save->LogisticsCargoCompletedRuns = CargoCompletedRuns;
    Save->LogisticsCargoFailedRuns = CargoFailedRuns;
    Save->LogisticsCargoLifetimeRevenue = CargoLifetimeRevenue;
    Save->LogisticsMarketDay = MarketDay;
    Save->FeedDepotStock = FeedDepotStock;
    Save->HillFarmDemand = HillFarmDemand;
    Save->WoodYardDemand = WoodYardDemand;
    Save->CargoRotationIndex = CargoRotationIndex;
    Save->CargoBacklogPressure = CargoBacklogPressure;
    Save->CargoNegotiatedOrderTier = CargoNegotiatedOrderTier;
    Save->CargoNegotiationDay = CargoNegotiationDay;
    Save->LogisticsRecentContractTags = RecentContractTags;
    Save->LogisticsRecentPayouts = RecentPayouts;
    Save->LogisticsRecentQualityPercent = RecentQualityPercent;
}

void UGTTLogisticsReputationSubsystem::RestoreFromSave(const UGTTSaveGame* Save)
{
    if (!Save || Save->SaveVersion < LogisticsSaveVersion)
    {
        Reputation = 0;
        CleanStreak = 0;
        CompletedRuns = 0;
        FailedRuns = 0;
        LifetimeRevenue = 0;
        CargoCompletedRuns = 0;
        CargoFailedRuns = 0;
        CargoLifetimeRevenue = 0;
        MarketDay = 0;
        FeedDepotStock = 10;
        HillFarmDemand = 6;
        WoodYardDemand = 4;
        CargoRotationIndex = 0;
        CargoBacklogPressure = 0;
        CargoNegotiatedOrderTier = 0;
        CargoNegotiationDay = 0;
        RecentContractTags.Reset();
        RecentPayouts.Reset();
        RecentQualityPercent.Reset();
        return;
    }

    Reputation = FMath::Clamp(Save->LogisticsReputation, 0, 100);
    CleanStreak = FMath::Max(0, Save->LogisticsCleanStreak);
    CompletedRuns = FMath::Max(0, Save->LogisticsCompletedRuns);
    FailedRuns = FMath::Max(0, Save->LogisticsFailedRuns);
    LifetimeRevenue = FMath::Max(0, Save->LogisticsLifetimeRevenue);
    CargoCompletedRuns = FMath::Max(0, Save->LogisticsCargoCompletedRuns);
    CargoFailedRuns = FMath::Max(0, Save->LogisticsCargoFailedRuns);
    CargoLifetimeRevenue = FMath::Max(0, Save->LogisticsCargoLifetimeRevenue);
    MarketDay = FMath::Max(0, Save->LogisticsMarketDay);
    FeedDepotStock = FMath::Clamp(Save->FeedDepotStock, 0, MaxFeedDepotStock);
    HillFarmDemand = FMath::Clamp(Save->HillFarmDemand, 0, MaxHillFarmDemand);
    WoodYardDemand = FMath::Clamp(Save->WoodYardDemand, 0, MaxWoodYardDemand);
    CargoRotationIndex = FMath::Clamp(Save->CargoRotationIndex, 0, 2);
    CargoBacklogPressure = FMath::Clamp(Save->CargoBacklogPressure, 0, MaxCargoBacklogPressure);
    CargoNegotiatedOrderTier = FMath::Clamp(Save->CargoNegotiatedOrderTier, 0, 3);
    CargoNegotiationDay = FMath::Max(0, Save->CargoNegotiationDay);
    RecentContractTags = Save->LogisticsRecentContractTags;
    RecentPayouts = Save->LogisticsRecentPayouts;
    RecentQualityPercent = Save->LogisticsRecentQualityPercent;

    const int32 AlignedCount = FMath::Min3(RecentContractTags.Num(), RecentPayouts.Num(), RecentQualityPercent.Num());
    RecentContractTags.SetNum(AlignedCount);
    RecentPayouts.SetNum(AlignedCount);
    RecentQualityPercent.SetNum(AlignedCount);
    while (RecentContractTags.Num() > MaxRecentContracts)
    {
        RecentContractTags.RemoveAt(0);
        RecentPayouts.RemoveAt(0);
        RecentQualityPercent.RemoveAt(0);
    }
    EnsureCargoMarketForCurrentDay();
}
