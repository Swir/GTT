#include "World/GTTDispatcherRelationshipSubsystem.h"

#include "Engine/World.h"
#include "World/GTTLogisticsReputationSubsystem.h"
#include "World/GTTLogisticsRoutePlanner.h"

namespace
{
    const FName FeedDepotDispatcherRole(TEXT("FeedDepotDispatcher"));
    const FName HillFarmReceiverRole(TEXT("HillFarmReceiver"));
    const FName WoodYardForemanRole(TEXT("WoodYardForeman"));
}

const UGTTLogisticsReputationSubsystem* UGTTDispatcherRelationshipSubsystem::GetLogistics() const
{
    return GetWorld() ? GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>() : nullptr;
}

int32 UGTTDispatcherRelationshipSubsystem::CalculateRelationship(FName RoleTag) const
{
    const UGTTLogisticsReputationSubsystem* Logistics = GetLogistics();
    if (!Logistics) return 0;

    const int32 Reputation = Logistics->GetReputation();
    const int32 CleanStreak = Logistics->GetCleanStreak();
    const int32 CargoWins = Logistics->GetCargoCompletedRuns();
    const int32 CargoLosses = Logistics->GetCargoFailedRuns();
    const int32 RoadWins = Logistics->GetCompletedRuns();
    const int32 RoadLosses = Logistics->GetFailedRuns();

    int32 Score = 0;
    if (RoleTag == FeedDepotDispatcherRole)
    {
        Score = 15 + CargoWins * 6 - CargoLosses * 8 + FMath::Min(CleanStreak, 6) * 3 + Reputation / 4;
    }
    else if (RoleTag == HillFarmReceiverRole)
    {
        Score = 20 + CargoWins * 5 - CargoLosses * 7 + FMath::Min(CleanStreak, 6) * 2 + Reputation / 5;
    }
    else if (RoleTag == WoodYardForemanRole)
    {
        Score = 15 + CargoWins * 4 + RoadWins * 3 - CargoLosses * 6 - RoadLosses * 5 + Reputation / 5;
    }
    return FMath::Clamp(Score, 0, 100);
}

FString UGTTDispatcherRelationshipSubsystem::RelationshipLabel(int32 Score)
{
    if (Score >= 70) return TEXT("PREFERRED");
    if (Score >= 45) return TEXT("TRUSTED");
    if (Score >= 25) return TEXT("KNOWN");
    return TEXT("NEW DRIVER");
}

int32 UGTTDispatcherRelationshipSubsystem::GetFeedDispatcherRelationship() const
{
    return CalculateRelationship(FeedDepotDispatcherRole);
}

int32 UGTTDispatcherRelationshipSubsystem::GetHillReceiverRelationship() const
{
    return CalculateRelationship(HillFarmReceiverRole);
}

int32 UGTTDispatcherRelationshipSubsystem::GetWoodForemanRelationship() const
{
    return CalculateRelationship(WoodYardForemanRole);
}

FString UGTTDispatcherRelationshipSubsystem::GetFeedRelationshipLabel() const
{
    return RelationshipLabel(GetFeedDispatcherRelationship());
}

FString UGTTDispatcherRelationshipSubsystem::GetHillRelationshipLabel() const
{
    return RelationshipLabel(GetHillReceiverRelationship());
}

FString UGTTDispatcherRelationshipSubsystem::GetWoodRelationshipLabel() const
{
    return RelationshipLabel(GetWoodForemanRelationship());
}

int32 UGTTDispatcherRelationshipSubsystem::GetCargoDeskAccessTier() const
{
    const UGTTLogisticsReputationSubsystem* Logistics = GetLogistics();
    if (!Logistics) return 1;

    const int32 Relationship = GetFeedDispatcherRelationship();
    const int32 RelationshipTier = Relationship >= 70 ? 3 : (Relationship >= 35 ? 2 : 1);
    return FMath::Min(RelationshipTier, FMath::Clamp(Logistics->GetCargoRouteTier(), 1, 3));
}

bool UGTTDispatcherRelationshipSubsystem::CanAccessCargoTier(int32 Tier) const
{
    return Tier >= 1 && Tier <= GetCargoDeskAccessTier();
}

int32 UGTTDispatcherRelationshipSubsystem::GetCargoReservationCapacity() const
{
    return GetFeedDispatcherRelationship() >= 45 ? 2 : 1;
}

int32 UGTTDispatcherRelationshipSubsystem::GetCargoReservationHoldMinutes() const
{
    const int32 Relationship = GetFeedDispatcherRelationship();
    if (Relationship >= 70) return 110;
    if (Relationship >= 45) return 80;
    if (Relationship >= 25) return 50;
    return 35;
}

int32 UGTTDispatcherRelationshipSubsystem::GetEffectiveCargoReservationHoldMinutes() const
{
    const int32 RelationshipHoldMinutes = GetCargoReservationHoldMinutes();
    const UGTTLogisticsReputationSubsystem* Logistics = GetLogistics();
    if (!Logistics) return RelationshipHoldMinutes;

    const int32 PrioritySlaMinutes = Logistics->GetCargoPriorityPickupSlaMinutes();
    if (PrioritySlaMinutes <= 0) return RelationshipHoldMinutes;

    // Preserve the 0.1.11 emergency ceiling first, then apply the 0.1.12 cross-lane planner.
    // This keeps relationship trust meaningful while making a competing, materially stronger
    // ROAD emergency release scarce CARGO stock sooner if the player does not collect it.
    const int32 PriorityLimitedHold = FMath::Min(RelationshipHoldMinutes, PrioritySlaMinutes);
    const int32 ConflictHoldMinutes = FGTTLogisticsRoutePlanner::GetCargoConflictHoldCapMinutes(GetWorld());
    return ConflictHoldMinutes > 0
        ? FMath::Min(PriorityLimitedHold, ConflictHoldMinutes)
        : PriorityLimitedHold;
}

FString UGTTDispatcherRelationshipSubsystem::GetCargoReservationFavorLabel() const
{
    const int32 Relationship = GetFeedDispatcherRelationship();
    FString BaseLabel;
    if (Relationship >= 70) BaseLabel = TEXT("PREFERRED FAVOR: DOUBLE DESK / 110 MIN HOLD");
    else if (Relationship >= 45) BaseLabel = TEXT("TRUST FAVOR: DOUBLE DESK / 80 MIN HOLD");
    else if (Relationship >= 25) BaseLabel = TEXT("KNOWN DRIVER: 50 MIN HOLD");
    else BaseLabel = TEXT("STANDARD DESK: 35 MIN HOLD");

    const UGTTLogisticsReputationSubsystem* Logistics = GetLogistics();
    const int32 PrioritySlaMinutes = Logistics ? Logistics->GetCargoPriorityPickupSlaMinutes() : 0;
    if (PrioritySlaMinutes > 0)
    {
        const int32 ConflictHoldMinutes = FGTTLogisticsRoutePlanner::GetCargoConflictHoldCapMinutes(GetWorld());
        const FString Conflict = ConflictHoldMinutes > 0
            ? FString::Printf(TEXT(" | ROUTE CONFLICT CAP %d MIN"), ConflictHoldMinutes)
            : TEXT("");
        return FString::Printf(TEXT("%s | PRIORITY SLA %d MIN | EFFECTIVE %d MIN%s"),
            *BaseLabel, PrioritySlaMinutes, GetEffectiveCargoReservationHoldMinutes(), *Conflict);
    }
    return BaseLabel;
}

FString UGTTDispatcherRelationshipSubsystem::GetContractDeskSummary() const
{
    const UGTTLogisticsReputationSubsystem* Logistics = GetLogistics();
    if (!Logistics) return TEXT("CONTRACT DESK OFFLINE");

    const int32 AccessTier = GetCargoDeskAccessTier();
    TArray<FString> CargoOptions;
    for (const int32 Tier : Logistics->GetCargoNegotiationOptions())
    {
        if (Tier <= AccessTier) CargoOptions.Add(FString::Printf(TEXT("T%d"), Tier));
    }
    const FString Cargo = CargoOptions.Num() > 0 ? FString::Join(CargoOptions, TEXT("/")) : TEXT("NONE");
    const FString Road = Logistics->IsRoadCourierWindowOpen() ? TEXT("ROAD OPEN") : TEXT("ROAD CLOSED");
    return FString::Printf(TEXT("DESK CARGO %s | %s | FEED %s %d | %s | %s | %s | %s"),
        *Cargo, *Road, *GetFeedRelationshipLabel(), GetFeedDispatcherRelationship(),
        *GetCargoReservationFavorLabel(), *Logistics->GetCargoReservationSummary(), *Logistics->GetPriorityChainSummary(),
        *FGTTLogisticsRoutePlanner::BuildSummary(GetWorld()));
}

FString UGTTDispatcherRelationshipSubsystem::GetDispatcherReaction(FName RoleTag) const
{
    const int32 Score = CalculateRelationship(RoleTag);
    if (RoleTag == FeedDepotDispatcherRole)
    {
        if (Score >= 70) return TEXT("You've kept our routes moving. I can hold two scarce loads longer for you.");
        if (Score >= 45) return TEXT("You've earned trust here. Two live orders can sit on your desk while you plan the route.");
        if (Score >= 25) return TEXT("I know your truck now. I can protect one load a little longer if you commit.");
        return TEXT("Start with the direct work. I will hold one load briefly; don't leave the farm waiting.");
    }
    if (RoleTag == HillFarmReceiverRole)
    {
        if (Score >= 70) return TEXT("Good to see you. Your deliveries have been dependable around here.");
        if (Score >= 45) return TEXT("We trust you with the farm orders. Keep the cargo intact.");
        if (Score >= 25) return TEXT("You're becoming a familiar driver. Don't leave us waiting.");
        return TEXT("Bring the first loads in clean and we'll remember it.");
    }
    if (RoleTag == WoodYardForemanRole)
    {
        if (Score >= 70) return TEXT("Preferred driver. If the yard has a backlog, I want you on it.");
        if (Score >= 45) return TEXT("You know this yard. Clean parts and cargo runs matter here.");
        if (Score >= 25) return TEXT("We've seen you before. Keep the police out of our gate.");
        return TEXT("Prove you can finish the route before I call you dependable.");
    }
    return TEXT("Keep the deliveries clean and the relationship will improve.");
}
