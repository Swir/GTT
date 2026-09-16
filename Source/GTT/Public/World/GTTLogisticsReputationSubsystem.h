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

    void RecordCourierSuccess(int32 Payout, float ParcelIntegrity, bool bFastDelivery, bool bPoliceIncident, int32 NativeImpacts);
    void RecordCourierFailure(bool bSevereFailure);

    void CaptureToSave(UGTTSaveGame* Save) const;
    void RestoreFromSave(const UGTTSaveGame* Save);

private:
    float GetTimeOfDayHours() const;

    int32 Reputation = 0;
    int32 CleanStreak = 0;
    int32 CompletedRuns = 0;
    int32 FailedRuns = 0;
    int32 LifetimeRevenue = 0;
};
