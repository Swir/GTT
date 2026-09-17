#include "World/GTTLogisticsReputationSubsystem.h"

namespace
{
int32 ToUrgency(int32 Pressure, int32 PriorityThreshold, int32 UrgentThreshold, int32 CriticalThreshold)
{
    if (Pressure >= CriticalThreshold) return 3;
    if (Pressure >= UrgentThreshold) return 2;
    if (Pressure >= PriorityThreshold) return 1;
    return 0;
}

const TCHAR* UrgencyLabel(int32 Urgency)
{
    switch (Urgency)
    {
        case 3: return TEXT("CRITICAL");
        case 2: return TEXT("RUSH");
        case 1: return TEXT("PRIORITY");
        default: return TEXT("STANDARD");
    }
}

int32 CargoPickupSlaMinutesForUrgency(int32 Urgency)
{
    switch (Urgency)
    {
        case 3: return 15;
        case 2: return 25;
        case 1: return 40;
        default: return 0;
    }
}

float RoadPickupSlaSecondsForUrgency(int32 Urgency)
{
    switch (Urgency)
    {
        case 3: return 50.0f;
        case 2: return 70.0f;
        case 1: return 90.0f;
        default: return 0.0f;
    }
}
}

int32 UGTTLogisticsReputationSubsystem::GetRoadPriorityUrgency() const
{
    EnsureCargoMarketForCurrentDay();

    // The Rattleback carries village/farm parts, so ROAD urgency only escalates when the
    // living market is actually rotating FARM PARTS. This keeps the rush contract grounded
    // in the same supply chain instead of inventing an unrelated daily bonus.
    if (CargoRotationIndex != 2) return 0;

    const int32 Pressure = WoodYardDemand + CargoBacklogPressure * 2;
    return ToUrgency(Pressure, 5, 9, 14);
}

int32 UGTTLogisticsReputationSubsystem::GetCargoPriorityUrgency() const
{
    EnsureCargoMarketForCurrentDay();

    // A stock-backed reservation remains a valid priority commitment even when free stock is
    // now zero because those units have already been removed from Feed Depot inventory.
    const bool bHasDeliverableLoad = FeedDepotStock > 0 || CargoReservedUnits.Num() > 0;
    if (!bHasDeliverableLoad) return 0;

    const int32 StrongestBuyerNeed = FMath::Max(HillFarmDemand, WoodYardDemand);
    const int32 ActiveTier = FMath::Clamp(GetActiveCargoOrderTier(), 1, 3);
    const int32 Pressure = StrongestBuyerNeed + CargoBacklogPressure * 2 + (ActiveTier - 1);
    return ToUrgency(Pressure, 6, 11, 16);
}

int32 UGTTLogisticsReputationSubsystem::GetPriorityChainStreak() const
{
    // CleanStreak is already persisted by schema v8 and is shared by successful ROAD/CARGO
    // work. Capping the priority chain at three keeps the emergency bonus meaningful but small.
    return FMath::Clamp(CleanStreak, 0, 3);
}

float UGTTLogisticsReputationSubsystem::GetPriorityChainRewardMultiplier() const
{
    const int32 HighestUrgency = FMath::Max(GetRoadPriorityUrgency(), GetCargoPriorityUrgency());
    if (HighestUrgency <= 0) return 1.0f;
    return 1.0f + static_cast<float>(GetPriorityChainStreak()) * 0.03f;
}

float UGTTLogisticsReputationSubsystem::GetRoadPriorityRewardMultiplier() const
{
    const int32 RoadUrgency = GetRoadPriorityUrgency();
    if (RoadUrgency <= 0) return 1.0f;

    float UrgencyMultiplier = 1.0f;
    switch (RoadUrgency)
    {
        case 3: UrgencyMultiplier = 1.18f; break;
        case 2: UrgencyMultiplier = 1.12f; break;
        case 1: UrgencyMultiplier = 1.06f; break;
        default: break;
    }

    // 0.1.11 adds a bounded cross-lane clean chain to genuinely urgent ROAD work. CARGO can
    // build that same persisted chain, but STANDARD ROAD work never receives an emergency bonus.
    return FMath::Min(1.28f, UrgencyMultiplier * GetPriorityChainRewardMultiplier());
}

float UGTTLogisticsReputationSubsystem::GetRoadPriorityTimeScale() const
{
    switch (GetRoadPriorityUrgency())
    {
        case 3: return 0.82f;
        case 2: return 0.88f;
        case 1: return 0.94f;
        default: return 1.0f;
    }
}

float UGTTLogisticsReputationSubsystem::GetCargoPriorityRewardMultiplier() const
{
    // Kept intentionally neutral as a separate multiplier. CARGO already receives bounded
    // CleanStreak economics through GetCargoMarketMultiplier(); 0.1.11 changes its pickup SLA
    // instead of silently stacking a second payout multiplier on the capped market.
    return 1.0f;
}

float UGTTLogisticsReputationSubsystem::GetRoadPriorityPickupSlaSeconds() const
{
    const float BaseSeconds = RoadPickupSlaSecondsForUrgency(GetRoadPriorityUrgency());
    if (BaseSeconds <= 0.0f) return 0.0f;
    // Each clean priority-chain step grants ten seconds of earned dispatch grace, max +30 sec.
    return BaseSeconds + static_cast<float>(GetPriorityChainStreak()) * 10.0f;
}

int32 UGTTLogisticsReputationSubsystem::GetCargoPriorityPickupSlaMinutes() const
{
    const int32 BaseMinutes = CargoPickupSlaMinutesForUrgency(GetCargoPriorityUrgency());
    if (BaseMinutes <= 0) return 0;
    // Reservation timestamps live in the world clock, so CARGO keeps minute semantics.
    return BaseMinutes + GetPriorityChainStreak() * 5;
}

FString UGTTLogisticsReputationSubsystem::GetPriorityVehicleLabel() const
{
    const int32 RoadUrgency = GetRoadPriorityUrgency();
    const int32 CargoUrgency = GetCargoPriorityUrgency();

    if (RoadUrgency == 0 && CargoUrgency == 0) return TEXT("ANY READY LOGISTICS VEHICLE");
    if (RoadUrgency > CargoUrgency) return TEXT("RATTLEBACK 82 / ROAD");
    if (CargoUrgency > RoadUrgency) return TEXT("MULEBOX 1200 / CARGO");

    // Equal urgency is broken by real unresolved buyer pressure. FARM PARTS favors the
    // Rattleback only when North Wood Yard is at least as constrained as Hill Farm.
    if (CargoRotationIndex == 2 && WoodYardDemand >= HillFarmDemand) return TEXT("RATTLEBACK 82 / ROAD");
    return TEXT("MULEBOX 1200 / CARGO");
}

FString UGTTLogisticsReputationSubsystem::GetPriorityDispatchLabel() const
{
    const int32 RoadUrgency = GetRoadPriorityUrgency();
    const int32 CargoUrgency = GetCargoPriorityUrgency();
    const int32 Highest = FMath::Max(RoadUrgency, CargoUrgency);

    if (Highest <= 0) return TEXT("STANDARD DISPATCH");
    if (RoadUrgency == CargoUrgency)
    {
        return FString::Printf(TEXT("%s SPLIT DISPATCH"), UrgencyLabel(Highest));
    }
    return FString::Printf(TEXT("%s %s"), UrgencyLabel(Highest), RoadUrgency > CargoUrgency ? TEXT("ROAD DISPATCH") : TEXT("CARGO DISPATCH"));
}

FString UGTTLogisticsReputationSubsystem::GetPriorityChainSummary() const
{
    const int32 Chain = GetPriorityChainStreak();
    const float RoadSla = GetRoadPriorityPickupSlaSeconds();
    const int32 CargoSla = GetCargoPriorityPickupSlaMinutes();
    const int32 ChainBonusPercent = FMath::RoundToInt((GetPriorityChainRewardMultiplier() - 1.0f) * 100.0f);

    const FString RoadWindow = RoadSla > 0.0f ? FString::Printf(TEXT("%.0f SEC"), RoadSla) : TEXT("NORMAL WINDOW");
    const FString CargoWindow = CargoSla > 0 ? FString::Printf(TEXT("%d MIN"), CargoSla) : TEXT("NORMAL WINDOW");
    return FString::Printf(
        TEXT("CHAIN %d/3 +%d%% URGENT ROAD | ROAD PICKUP %s | CARGO PICKUP %s"),
        Chain, ChainBonusPercent, *RoadWindow, *CargoWindow);
}

FString UGTTLogisticsReputationSubsystem::GetPriorityDispatchSummary() const
{
    EnsureCargoMarketForCurrentDay();
    const int32 RoadUrgency = GetRoadPriorityUrgency();
    const int32 CargoUrgency = GetCargoPriorityUrgency();

    const int32 RoadBonusPercent = FMath::RoundToInt((GetRoadPriorityRewardMultiplier() - 1.0f) * 100.0f);
    const int32 RoadWindowPercent = FMath::RoundToInt(GetRoadPriorityTimeScale() * 100.0f);
    const float RoadSla = GetRoadPriorityPickupSlaSeconds();
    const int32 CargoSla = GetCargoPriorityPickupSlaMinutes();
    const FString RoadPickup = RoadSla > 0.0f ? FString::Printf(TEXT("%.0fs"), RoadSla) : TEXT("normal");
    const FString CargoPickup = CargoSla > 0 ? FString::Printf(TEXT("%dm"), CargoSla) : TEXT("normal");

    return FString::Printf(
        TEXT("%s | VEHICLE %s | ROAD U%d %s +%d%% reward / %d%% delivery / pickup %s | CARGO U%d %s / pickup %s | %s | stock %d | hill %d | wood %d | backlog %d"),
        *GetPriorityDispatchLabel(), *GetPriorityVehicleLabel(),
        RoadUrgency, UrgencyLabel(RoadUrgency), RoadBonusPercent, RoadWindowPercent, *RoadPickup,
        CargoUrgency, UrgencyLabel(CargoUrgency), *CargoPickup,
        *GetPriorityChainSummary(),
        FeedDepotStock, HillFarmDemand, WoodYardDemand, CargoBacklogPressure);
}
