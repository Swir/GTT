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
    if (MarketDay == CurrentDay) return;

    const int32 ElapsedDays = MarketDay > 0 ? FMath::Clamp(CurrentDay - MarketDay, 1, 7) : 1;
    const int32 Restock = 3 * ElapsedDays + ((CurrentDay + CargoCompletedRuns) % 4);
    FeedDepotStock = FMath::Clamp(FeedDepotStock + Restock, 0, MaxFeedDepotStock);

    // Demand is deterministic from the living world day plus the player's real logistics history.
    // Success/failure therefore changes tomorrow's route pressure without requiring random network state.
    HillFarmDemand = FMath::Clamp(4 + ((CurrentDay * 3 + CompletedRuns + CargoFailedRuns) % 7), 1, MaxHillFarmDemand);
    WoodYardDemand = FMath::Clamp(2 + ((CurrentDay * 2 + CargoCompletedRuns + FailedRuns) % 6), 1, MaxWoodYardDemand);
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
    const float ReputationBonus = FMath::Min(0.20f, static_cast<float>(Reputation) * 0.002f);
    const float StreakBonus = FMath::Min(0.05f, static_cast<float>(CleanStreak) * 0.01f);
    const float ShiftMultiplier = IsLateShift() ? 1.10f : 1.0f;
    return (1.0f + ReputationBonus + StreakBonus) * ShiftMultiplier;
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
    return FString::Printf(TEXT("%s | DEPOT %d | HILL NEED %d | WOOD NEED %d"),
        *GetCargoCommodityLabel(), FeedDepotStock, HillFarmDemand, WoodYardDemand);
}

bool UGTTLogisticsReputationSubsystem::CanAcceptCargoContract() const
{
    EnsureCargoMarketForCurrentDay();
    const int32 RouteTier = GetCargoRouteTier();
    const int32 RequiredStock = RouteTier >= 2 ? 3 : 2;
    if (FeedDepotStock < RequiredStock || HillFarmDemand <= 0) return false;
    if (RouteTier >= 2 && WoodYardDemand <= 0) return false;
    return true;
}

bool UGTTLogisticsReputationSubsystem::ReserveCargoContract(int32 RouteTier, int32& OutReservedUnits, FString& OutReason)
{
    EnsureCargoMarketForCurrentDay();
    OutReservedUnits = 0;
    OutReason.Reset();

    const int32 RequiredStock = RouteTier >= 2 ? 3 : 2;
    if (FeedDepotStock < RequiredStock)
    {
        OutReason = FString::Printf(TEXT("Feed Depot only has %d load units; this route needs %d. The next daily restock may reopen it."), FeedDepotStock, RequiredStock);
        return false;
    }
    if (HillFarmDemand <= 0)
    {
        OutReason = TEXT("Hill Farm demand is already satisfied for today. Check tomorrow's contract rotation.");
        return false;
    }
    if (RouteTier >= 2 && WoodYardDemand <= 0)
    {
        OutReason = TEXT("North Wood Yard has no remaining demand for the extended chain today.");
        return false;
    }

    FeedDepotStock -= RequiredStock;
    OutReservedUnits = RequiredStock;
    OutReason = FString::Printf(TEXT("Reserved %d units of %s | depot stock now %d."), RequiredStock, *GetCargoCommodityLabel(), FeedDepotStock);
    return true;
}

void UGTTLogisticsReputationSubsystem::SettleCargoContract(int32 ReservedUnits, bool bExtendedRoute, bool bSuccess)
{
    EnsureCargoMarketForCurrentDay();
    if (!bSuccess || ReservedUnits <= 0) return; // failed cargo is lost; buyer demand remains open.

    if (bExtendedRoute)
    {
        const int32 HillUnits = FMath::Max(1, ReservedUnits / 3);
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
    return FMath::Clamp(1.0f + TimeDemandBonus + DailyDemandBonus + ReputationBonus + StreakBonus, 1.0f, 1.38f);
}

FString UGTTLogisticsReputationSubsystem::GetCargoMarketLabel() const
{
    EnsureCargoMarketForCurrentDay();
    if (!IsCargoDepotWindowOpen()) return FString::Printf(TEXT("MARKET PAUSED | %s"), *GetCargoCommodityLabel());
    const float Hour = GetTimeOfDayHours();
    const FString Demand = Hour < 10.0f ? TEXT("MORNING RUSH") : (Hour >= 14.0f ? TEXT("LATE FEED DEMAND") : TEXT("STEADY DEMAND"));
    const int32 Bonus = FMath::RoundToInt((GetCargoMarketMultiplier() - 1.0f) * 100.0f);
    return FString::Printf(TEXT("%s +%d%% | ROUTE T%d | %s"), *Demand, Bonus, GetCargoRouteTier(), *GetCargoCommodityLabel());
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
    AppendHistory(bExtendedRoute ? FName(TEXT("CargoChain")) : FName(TEXT("FarmCargo")), Payout, FMath::RoundToInt(CargoIntegrity * 100.0f));
}

void UGTTLogisticsReputationSubsystem::RecordCargoFailure(float CargoIntegrity, bool bSevereFailure)
{
    Reputation = FMath::Clamp(Reputation - (bSevereFailure ? 10 : 6), 0, 100);
    CleanStreak = 0;
    ++FailedRuns;
    ++CargoFailedRuns;
    AppendHistory(FName(TEXT("CargoFail")), 0, FMath::RoundToInt(CargoIntegrity * 100.0f));
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
