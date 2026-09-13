#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GTTRadioComponent.generated.h"

class UAudioComponent;
class USoundWaveProcedural;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGTTRadioChanged, FString, StationName, FString, TrackTitle);

UCLASS(ClassGroup=(GTT), meta=(BlueprintSpawnableComponent))
class GTT_API UGTTRadioComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTTRadioComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

    UFUNCTION(BlueprintPure, Category="GTT|Radio")
    bool HasAudibleProgram() const;

    UPROPERTY(BlueprintAssignable, Category="GTT|Radio")
    FGTTRadioChanged OnRadioChanged;

private:
    struct FStationRuntime
    {
        FString Name;
        TArray<FString> Tracks;
        float BaseBpm = 100.0f;
        int32 SoundPalette = 0;
    };

    void AdvanceTrack();
    void BroadcastState();
    void StartCurrentTrackAudio();
    void StopAudio();
    void RefreshAudioVolume();
    void GenerateTrackPcm(TArray<int16>& OutSamples, float DurationSeconds, int32 Seed, float Bpm, int32 Palette) const;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AudioComponent;

    UPROPERTY(Transient)
    TObjectPtr<USoundWaveProcedural> ProceduralWave;

    TArray<FStationRuntime> Stations;
    int32 CurrentStationIndex = -1;
    int32 CurrentTrackIndex = 0;
    float TrackTimeRemaining = 0.0f;
    float VolumeRefreshRemaining = 0.0f;

    static constexpr int32 RadioSampleRate = 22050;
    static constexpr float TrackDurationSeconds = 36.0f;
};
