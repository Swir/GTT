#include "World/GTTDispatcherRelationshipSubsystem.h"

#include "Engine/World.h"
#include "World/GTTLogisticsReputationSubsystem.h"

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
        // The dispatcher values reliable cargo throughput most strongly.
        Score = 15 + CargoWins * 6 - CargoLosses * 8 + FMath::Min(CleanStreak, 6) * 3 + Reputation / 4;
    }
    else if (RoleTag == HillFarmReceiverRole)
    {
        // Hill Farm cares about cargo completion and clean handling, but is slightly more forgiving.
        Score = 20 + CargoWins * 5 - CargoLosses * 7 + FMath::Min(CleanStreak, 6) * 2 + Reputation / 5;
    }
    else if (RoleTag == WoodYardForemanRole)
    {
        // The foreman sees both Mulebox relay work and Rattleback parts traffic.
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
    return FString::Printf(TEXT("DESK CARGO %s | %s | FEED %s %d"),
        *Cargo, *Road, *GetFeedRelationshipLabel(), GetFeedDispatcherRelationship());
}

FString UGTTDispatcherRelationshipSubsystem::GetDispatcherReaction(FName RoleTag) const
{
    const int32 Score = CalculateRelationship(RoleTag);
    if (RoleTag == FeedDepotDispatcherRole)
    {
        if (Score >= 70) return TEXT("You've kept our routes moving. The heavy orders are on your desk.");
        if (Score >= 45) return TEXT("You've earned trust here. I can put more serious loads in front of you.");
        if (Score >= 25) return TEXT("I know your truck now. Keep bringing loads in clean and on time.");
        return TEXT("Start with the direct work. Show me you can bring the load home.");
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
