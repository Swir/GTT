#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTTDayNightCycle.generated.h"

class ADirectionalLight;
class ASkyLight;

UCLASS()
class GTT_API AGTTDayNightCycle : public AActor
{
    GENERATED_BODY()

public:
    AGTTDayNightCycle();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category="GTT|World|Time")
    float GetTimeOfDayHours() const { return TimeOfDayHours; }

    UFUNCTION(BlueprintPure, Category="GTT|World|Time")
    int32 GetDayNumber() const { return DayNumber; }

    UFUNCTION(BlueprintPure, Category="GTT|World|Time")
    bool IsNight() const { return TimeOfDayHours < 6.0f || TimeOfDayHours >= 21.5f; }

    UFUNCTION(BlueprintPure, Category="GTT|World|Time")
    FString GetClockText() const;

    UFUNCTION(BlueprintCallable, Category="GTT|World|Time")
    void RestoreTime(int32 InDayNumber, float InTimeOfDayHours);

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|World|Time", meta=(ClampMin="60.0"))
    float RealSecondsPerGameDay = 1080.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GTT|World|Time", meta=(ClampMin="0.0", ClampMax="24.0"))
    float StartingHour = 8.0f;

private:
    void UpdateLighting();

    float TimeOfDayHours = 8.0f;
    int32 DayNumber = 1;

    TWeakObjectPtr<ADirectionalLight> Sun;
    TWeakObjectPtr<ASkyLight> Sky;
};
