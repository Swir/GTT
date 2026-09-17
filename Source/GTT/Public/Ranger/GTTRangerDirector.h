#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTRangerDirector.generated.h"

class APawn;
class AGTTRangerPawn;
class AGTTRangerPatrolVehicle;
class AGTTRangerPullOverMarker;

UCLASS()
class GTT_API AGTTRangerDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTRangerDirector();
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintPure, Category="GTT|Ranger")
    int32 GetActiveRangerCount() const { return ActiveRangers.Num(); }

    UFUNCTION(BlueprintPure, Category="GTT|Ranger")
    int32 GetEnforcementTier() const { return EnforcementTier; }

    UFUNCTION(BlueprintPure, Category="GTT|Ranger")
    bool IsPoliceHandoffActive() const { return bPoliceHandoffIssued; }

    UFUNCTION(BlueprintPure, Category="GTT|Ranger")
    bool IsNightReinforcementActive() const { return bNightReinforcementActive; }

    UFUNCTION(BlueprintPure, Category="GTT|Ranger")
    bool IsPatrolVehicleAvailable() const { return PatrolVehicle.IsValid(); }

    UFUNCTION(BlueprintPure, Category="GTT|Ranger")
    bool IsPullOverMarkerAvailable() const { return PullOverMarker.IsValid(); }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger")
    TSubclassOf<AGTTRangerPawn> RangerClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger")
    TSubclassOf<AGTTRangerPatrolVehicle> PatrolVehicleClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger")
    TSubclassOf<AGTTRangerPullOverMarker> PullOverMarkerClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="0.25"))
    float ResponseInterval = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="1", ClampMax="3"))
    int32 NightReinforcementAlertLevel = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="1", ClampMax="3"))
    int32 PoliceHandoffAlertLevel = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Ranger", meta=(ClampMin="0.0"))
    float PoliceHandoffHeat = 35.0f;

private:
    void UpdateResponse();
    void CleanupInvalidRangers();
    void ApplyPoliceHandoff(APawn* PlayerPawn, int32 AlertLevel);

    FTimerHandle ResponseTimer;
    TArray<TWeakObjectPtr<AGTTRangerPawn>> ActiveRangers;
    TWeakObjectPtr<AGTTRangerPatrolVehicle> PatrolVehicle;
    TWeakObjectPtr<AGTTRangerPullOverMarker> PullOverMarker;
    int32 EnforcementTier = 0;
    bool bPoliceHandoffIssued = false;
    bool bNightReinforcementActive = false;
};