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

float UGTTLogisticsReputationSubsystem::GetRoadPriorityRewardMultiplier() const
{
    switch (GetRoadPriorityUrgency())
    {
        case 3: return 1.18f;
        case 2: return 1.12f;
        case 1: return 1.06f;
        default: return 1.0f;
    }
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
    // Kept intentionally neutral in 0.1.10. CARGO urgency is a dispatch/routing signal and
    // existing CARGO economics continue to come from stock, demand, backlog and route tier.
    // This avoids silently stacking another payout multiplier on the already capped market.
    return 1.0f;
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

FString UGTTLogisticsReputationSubsystem::GetPriorityDispatchSummary() const
{
    EnsureCargoMarketForCurrentDay();
    const int32 RoadUrgency = GetRoadPriorityUrgency();
    const int32 CargoUrgency = GetCargoPriorityUrgency();

    const int32 RoadBonusPercent = FMath::RoundToInt((GetRoadPriorityRewardMultiplier() - 1.0f) * 100.0f);
    const int32 RoadWindowPercent = FMath::RoundToInt(GetRoadPriorityTimeScale() * 100.0f);

    return FString::Printf(
        TEXT("%s | VEHICLE %s | ROAD U%d %s +%d%% reward / %d%% window | CARGO U%d %s | stock %d | hill %d | wood %d | backlog %d"),
        *GetPriorityDispatchLabel(), *GetPriorityVehicleLabel(),
        RoadUrgency, UrgencyLabel(RoadUrgency), RoadBonusPercent, RoadWindowPercent,
        CargoUrgency, UrgencyLabel(CargoUrgency),
        FeedDepotStock, HillFarmDemand, WoodYardDemand, CargoBacklogPressure);
}
