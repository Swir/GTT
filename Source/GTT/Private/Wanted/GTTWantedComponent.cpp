#include "Wanted/GTTWantedComponent.h"

UGTTWantedComponent::UGTTWantedComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    WantedThresholds = {20.0f, 45.0f, 75.0f, 110.0f, 150.0f};
}

void UGTTWantedComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    SecondsSinceCrime += DeltaTime;

    if (CurrentHeat > 0.0f && SecondsSinceCrime >= HeatDecayDelay)
    {
        CurrentHeat = FMath::Max(0.0f, CurrentHeat - HeatDecayPerSecond * DeltaTime);
        RecalculateWantedLevel();
    }
}

void UGTTWantedComponent::AddHeat(float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }

    CurrentHeat += Amount;
    SecondsSinceCrime = 0.0f;
    RecalculateWantedLevel();
}

void UGTTWantedComponent::ClearWanted()
{
    CurrentHeat = 0.0f;
    SecondsSinceCrime = 0.0f;
    RecalculateWantedLevel();
}

void UGTTWantedComponent::RecalculateWantedLevel()
{
    int32 NewWantedLevel = 0;

    for (const float Threshold : WantedThresholds)
    {
        if (CurrentHeat >= Threshold)
        {
            ++NewWantedLevel;
        }
    }

    NewWantedLevel = FMath::Clamp(NewWantedLevel, 0, 5);

    if (NewWantedLevel != WantedLevel)
    {
        WantedLevel = NewWantedLevel;
        OnWantedChanged.Broadcast(WantedLevel, CurrentHeat);
    }
}
