#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GTTWantedComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGTTWantedChanged, int32, NewWantedLevel, float, CurrentHeat);

UCLASS(ClassGroup=(GTT), meta=(BlueprintSpawnableComponent))
class GTT_API UGTTWantedComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTTWantedComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Wanted")
    void AddHeat(float Amount);

    UFUNCTION(BlueprintCallable, Category="GTT|Wanted")
    void ClearWanted();

    UFUNCTION(BlueprintPure, Category="GTT|Wanted")
    int32 GetWantedLevel() const { return WantedLevel; }

    UFUNCTION(BlueprintPure, Category="GTT|Wanted")
    float GetHeat() const { return CurrentHeat; }

    UPROPERTY(BlueprintAssignable, Category="GTT|Wanted")
    FGTTWantedChanged OnWantedChanged;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Wanted", meta=(ClampMin="0.0"))
    float HeatDecayDelay = 12.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Wanted", meta=(ClampMin="0.0"))
    float HeatDecayPerSecond = 2.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|Wanted")
    TArray<float> WantedThresholds;

private:
    void RecalculateWantedLevel();

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Wanted")
    float CurrentHeat = 0.0f;

    UPROPERTY(VisibleInstanceOnly, Category="GTT|Wanted")
    int32 WantedLevel = 0;

    float SecondsSinceCrime = 0.0f;
};
