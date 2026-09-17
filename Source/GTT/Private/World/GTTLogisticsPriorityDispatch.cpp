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

int32 PickupSlaMinutesForUrgency(int32 Urgency, bool bCargo)
{
    if (bCargo)
    {
        switch (Urgency)
        {
            case 3: return 15;
            case 2: return 25;
            case 1: return 40;
            default: return 0;
        }
    }

    switch (Urgency)
    {
        case 3: return 20;
        case 2: return 30;
        case 1: return 45;
        default: return 0;
    }
}

FString PickupSlaLabel(int32 Minutes)
{
    return Minutes > 0 ? FString::Printf(TEXT("%d MIN"), Minutes) : TEXT("NORMAL WINDOW");
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
    float UrgencyMultiplier = 1.0f;
    switch (GetRoadPriorityUrgency())
    {
        case 3: UrgencyMultiplier = 1.18f; break;
        case 2: UrgencyMultiplier = 1.12f; break;
        case 1: UrgencyMultiplier = 1.06f; break;
        default: break;
    }

    // 0.1.11 adds a bounded cross-lane chain reward. The same clean streak already feeds the
    // CARGO market, while ROAD receives this explicit emergency-chain multiplier. Hard cap keeps
    // CRITICAL + three clean jobs below a runaway 1.30x urgency layer.
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

int32 UGTTLogisticsReputationSubsystem::GetRoadPriorityPickupSlaMinutes() const
{
    const int32 Urgency = GetRoadPriorityUrgency();
    const int32 BaseMinutes = PickupSlaMinutesForUrgency(Urgency, false);
    if (BaseMinutes <= 0) return 0;
    return BaseMinutes + GetPriorityChainStreak() * 5;
}

int32 UGTTLogisticsReputationSubsystem::GetCargoPriorityPickupSlaMinutes() const
{
    const int32 Urgency = GetCargoPriorityUrgency();
    const int32 BaseMinutes = PickupSlaMinutesForUrgency(Urgency, true);
    if (BaseMinutes <= 0) return 0;
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
    const int32 RoadSla = GetRoadPriorityPickupSlaMinutes();
    const int32 CargoSla = GetCargoPriorityPickupSlaMinutes();
    const int32 ChainBonusPercent = FMath::RoundToInt((GetPriorityChainRewardMultiplier() - 1.0f) * 100.0f);

    return FString::Printf(
        TEXT("CHAIN %d/3 +%d%% ROAD | ROAD PICKUP %s | CARGO PICKUP %s"),
        Chain,
        ChainBonusPercent,
        *PickupSlaLabel(RoadSla),
        *PickupSlaLabel(CargoSla));
}

FString UGTTLogisticsReputationSubsystem::GetPriorityDispatchSummary() const
{
    EnsureCargoMarketForCurrentDay();
    const int32 RoadUrgency = GetRoadPriorityUrgency();
    const int32 CargoUrgency = GetCargoPriorityUrgency();

    const int32 RoadBonusPercent = FMath::RoundToInt((GetRoadPriorityRewardMultiplier() - 1.0f) * 100.0f);
    const int32 RoadWindowPercent = FMath::RoundToInt(GetRoadPriorityTimeScale() * 100.0f);

    return FString::Printf(
        TEXT("%s | VEHICLE %s | ROAD U%d %s +%d%% reward / %d%% delivery / pickup %s | CARGO U%d %s / pickup %s | %s | stock %d | hill %d | wood %d | backlog %d"),
        *GetPriorityDispatchLabel(), *GetPriorityVehicleLabel(),
        RoadUrgency, UrgencyLabel(RoadUrgency), RoadBonusPercent, RoadWindowPercent, *PickupSlaLabel(GetRoadPriorityPickupSlaMinutes()),
        CargoUrgency, UrgencyLabel(CargoUrgency), *PickupSlaLabel(GetCargoPriorityPickupSlaMinutes()),
        *GetPriorityChainSummary(),
        FeedDepotStock, HillFarmDemand, WoodYardDemand, CargoBacklogPressure);
}
