#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTVillageEventDirector.generated.h"

class AGTTCitizenPawn;
class AGTTDayNightCycle;
class AGTTVillageEventMarker;

UCLASS()
class GTT_API AGTTVillageEventDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTVillageEventDirector();
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintPure, Category="GTT|Village Nights")
    bool IsNightlifeOpen() const;

    UFUNCTION(BlueprintPure, Category="GTT|Village Nights")
    FString GetNightlifeSummary() const;

protected:
    UPROPERTY(EditDefaultsOnly, Category="GTT|Village Nights")
    TSubclassOf<AGTTVillageEventMarker> EventMarkerClass;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Village Nights", meta=(ClampMin="0", ClampMax="16"))
    int32 PartyCrowdSize = 6;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Village Nights", meta=(ClampMin="10.0"))
    float MinSecondsBetweenEvents = 45.0f;

    UPROPERTY(EditDefaultsOnly, Category="GTT|Village Nights", meta=(ClampMin="10.0"))
    float MaxSecondsBetweenEvents = 85.0f;

private:
    void EvaluateNightlife();
    void SpawnNightEvent();
    void EnsurePartyCrowd(bool bShouldExist);
    void CleanupInvalidCrowd();

    TWeakObjectPtr<AGTTDayNightCycle> DayNightCycle;
    TWeakObjectPtr<AGTTVillageEventMarker> ActiveEvent;

    UPROPERTY()
    TArray<TObjectPtr<AGTTCitizenPawn>> PartyCrowd;

    FTimerHandle EvaluationTimer;
    float EventCountdown = 8.0f;
    float EvaluationInterval = 5.0f;
};
