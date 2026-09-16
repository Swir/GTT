#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/GTTGarageFleetSubsystem.h"
#include "GTTContractBoardSubsystem.generated.h"

USTRUCT(BlueprintType)
struct GTT_API FGTTContractBoardOffer
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") FName JobTag = NAME_None;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") FString Title;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") EGTTGarageFleetRole RequiredRole = EGTTGarageFleetRole::Utility;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") bool bHardFleetRequirement = false;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") int32 BaseReward = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") int32 MaximumReward = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") int32 ServiceEstimate = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") int32 DispatchEstimate = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") int32 PreparationEstimate = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") int32 MaximumNetReward = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") bool bCanAcceptNow = false;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") bool bNeedsPreparation = false;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") bool bScheduleOpen = true;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") FString ScheduleStatus;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") int32 LogisticsReputation = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") FString LogisticsTier;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") int32 PayoutBonusPercent = 0;
    UPROPERTY(BlueprintReadOnly, Category="GTT|Contracts") FGTTFleetMissionAssessment Fleet;
};

UCLASS()
class GTT_API UGTTContractBoardSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Contracts")
    FGTTContractBoardOffer BuildOffer(FName JobTag) const;

    UFUNCTION(BlueprintCallable, Category="GTT|Contracts")
    bool TryPrepareContract(APawn* PlayerPawn, FName JobTag, const FTransform& StagingTransform, FString& OutSummary);

    UFUNCTION(BlueprintCallable, Category="GTT|Contracts")
    bool TryAcceptContract(APawn* PlayerPawn, FName JobTag, FString& OutSummary);

    UFUNCTION(BlueprintPure, Category="GTT|Contracts")
    bool IsAnyLegalContractActive() const;

    static FString ContractTitle(FName JobTag);
    static int32 ContractBaseReward(FName JobTag);
    static int32 ContractMaximumReward(FName JobTag);
    static bool IsHardFleetRequirement(FName JobTag);

private:
    static int32 CalculateServiceEstimate(const FGTTGarageFleetSnapshot& Snapshot);
    bool IsLegalWorkLocked(APawn* PlayerPawn, FString& OutReason) const;
    bool DispatchAssignedVehicle(const FGTTGarageFleetSnapshot& Snapshot, const FTransform& Destination, FString& OutFailure) const;
    bool ServiceAssignedVehicle(const FGTTGarageFleetSnapshot& Snapshot, FString& OutFailure) const;
};
