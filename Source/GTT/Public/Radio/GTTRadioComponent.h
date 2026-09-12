#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GTTRadioComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGTTRadioChanged, FString, StationName, FString, TrackTitle);

UCLASS(ClassGroup=(GTT), meta=(BlueprintSpawnableComponent))
class GTT_API UGTTRadioComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTTRadioComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="GTT|Radio")
    void CycleStation();

    UFUNCTION(BlueprintCallable, Category="GTT|Radio")
    void TurnOff();

    UFUNCTION(BlueprintPure, Category="GTT|Radio")
    bool IsRadioOn() const { return CurrentStationIndex >= 0 && Stations.IsValidIndex(CurrentStationIndex); }

    UFUNCTION(BlueprintPure, Category="GTT|Radio")
    FString GetStationName() const;

    UFUNCTION(BlueprintPure, Category="GTT|Radio")
    FString GetTrackTitle() const;

    UFUNCTION(BlueprintPure, Category="GTT|Radio")
    FString GetDisplayLine() const;

    UPROPERTY(BlueprintAssignable, Category="GTT|Radio")
    FGTTRadioChanged OnRadioChanged;

private:
    struct FStationRuntime
    {
        FString Name;
        TArray<FString> Tracks;
    };

    void AdvanceTrack();
    void BroadcastState();

    TArray<FStationRuntime> Stations;
    int32 CurrentStationIndex = -1;
    int32 CurrentTrackIndex = 0;
    float TrackTimeRemaining = 0.0f;
};
