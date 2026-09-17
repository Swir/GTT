#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GTTDispatcherRelationshipSubsystem.generated.h"

class UGTTLogisticsReputationSubsystem;

/**
 * Relationship layer for the rural logistics desk.
 *
 * Scores are deliberately derived from the already-persistent logistics record instead of
 * introducing a second mutable reputation save. This means old schema-v8 profiles inherit a
 * deterministic relationship immediately and save/load cannot make the dispatcher disagree
 * with the authoritative delivery history.
 */
UCLASS()
class GTT_API UGTTDispatcherRelationshipSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Relationships")
    int32 GetFeedDispatcherRelationship() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Relationships")
    int32 GetHillReceiverRelationship() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Relationships")
    int32 GetWoodForemanRelationship() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Relationships")
    FString GetFeedRelationshipLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Relationships")
    FString GetHillRelationshipLabel() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Relationships")
    FString GetWoodRelationshipLabel() const;

    /** Highest CARGO tier the Feed Dispatcher will reserve manually for this driver. */
    UFUNCTION(BlueprintPure, Category="GTT|Logistics|ContractDesk")
    int32 GetCargoDeskAccessTier() const;

    UFUNCTION(BlueprintPure, Category="GTT|Logistics|ContractDesk")
    bool CanAccessCargoTier(int32 Tier) const;

    /** Compact shared ROAD/CARGO desk line used by the dispatcher and contract boards. */
    UFUNCTION(BlueprintPure, Category="GTT|Logistics|ContractDesk")
    FString GetContractDeskSummary() const;

    /** Short NPC reaction based on the same persistent performance record. */
    UFUNCTION(BlueprintPure, Category="GTT|Logistics|Relationships")
    FString GetDispatcherReaction(FName RoleTag) const;

private:
    const UGTTLogisticsReputationSubsystem* GetLogistics() const;
    int32 CalculateRelationship(FName RoleTag) const;
    static FString RelationshipLabel(int32 Score);
};
