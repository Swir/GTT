#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTTrafficDirector.generated.h"

class AGTTDayNightCycle;
class AGTTTrafficCarPawn;

UCLASS()
class GTT_API AGTTTrafficDirector : public AActor
{
    GENERATED_BODY()

public:
    AGTTTrafficDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Population")
    int32 GetTargetTrafficCount() const { return TargetTrafficCount; }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Population")
    int32 GetManagedTrafficCount() const { return ManagedTrafficCars.Num(); }

    UFUNCTION(BlueprintPure, Category="GTT|Traffic|Population")
    FString GetTrafficProfileText() const { return TrafficProfileText; }

protected:
    /** Baseline village-loop population before rural commuters and time-of-day shaping. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Population", meta=(ClampMin="1", ClampMax="12"))
    int32 TrafficCarCount = 6;

    /** Rural commuters that keep farm/woodland roads alive outside the village loop. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Population", meta=(ClampMin="0", ClampMax="6"))
    int32 RuralCommuterCount = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Population", meta=(ClampMin="0", ClampMax="8"))
    int32 RushHourBonus = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Population", meta=(ClampMin="0", ClampMax="8"))
    int32 NightTrafficReduction = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Population", meta=(ClampMin="2", ClampMax="20"))
    int32 MaximumManagedTraffic = 14;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Population", meta=(ClampMin="0.5", ClampMax="15.0"))
    float PopulationUpdateIntervalSeconds = 3.0f;

    /** Managed traffic inside this radius is never culled during a profile transition. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Population", meta=(ClampMin="1000.0"))
    float MinimumCullDistanceFromPlayer = 3200.0f;

    /** Avoid burst spawn/despawn when the clock crosses a profile boundary. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic|Population", meta=(ClampMin="1", ClampMax="6"))
    int32 MaxPopulationAdjustmentPerPass = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Traffic")
    TSubclassOf<AGTTTrafficCarPawn> TrafficCarClass;

private:
    void CacheDayNightCycle();
    void CompactManagedTraffic();
    void ReconcileTrafficPopulation(bool bFillImmediately = false);
    int32 CalculateTargetTrafficCount(FString& OutProfileText) const;
    bool SpawnManagedTrafficCar();
    bool CullOneManagedTrafficCar();
    bool CanCullManagedCar(const AGTTTrafficCarPawn* TrafficCar) const;
    TArray<FVector> BuildSpawnRoute(int32 SpawnSerial, int32& OutStartIndex) const;

    TWeakObjectPtr<AGTTDayNightCycle> DayNightCycle;
    TArray<TWeakObjectPtr<AGTTTrafficCarPawn>> ManagedTrafficCars;
    int32 SpawnSequence = 0;
    int32 TargetTrafficCount = 0;
    FString TrafficProfileText = TEXT("DAY");
};
