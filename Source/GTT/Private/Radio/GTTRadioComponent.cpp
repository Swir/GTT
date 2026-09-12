#include "Radio/GTTRadioComponent.h"

UGTTRadioComponent::UGTTRadioComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UGTTRadioComponent::BeginPlay()
{
    Super::BeginPlay();

    Stations = {
        {TEXT("GRAVEL FM"), {TEXT("Dustline"), TEXT("Two-Lane Morning"), TEXT("Rust on the Gate"), TEXT("County Sunrise")}},
        {TEXT("BARNBEAT 96"), {TEXT("Neon Silo"), TEXT("Midnight Combine"), TEXT("Haywire Horizon"), TEXT("Concrete Dancefloor")}},
        {TEXT("RUST & DIESEL"), {TEXT("Cold Start"), TEXT("Grease Under Moonlight"), TEXT("County Line"), TEXT("Old Steel Heart")}},
        {TEXT("NIGHT SHIFT"), {TEXT("Static Fields"), TEXT("After Hours"), TEXT("Last Lamp Home"), TEXT("Blue Road at 2AM")}}
    };
}

void UGTTRadioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!IsRadioOn())
    {
        return;
    }

    TrackTimeRemaining -= DeltaTime;
    if (TrackTimeRemaining <= 0.0f)
    {
        AdvanceTrack();
    }
}

void UGTTRadioComponent::CycleStation()
{
    if (Stations.IsEmpty())
    {
        return;
    }

    ++CurrentStationIndex;
    if (CurrentStationIndex >= Stations.Num())
    {
        TurnOff();
        return;
    }

    CurrentTrackIndex = FMath::RandRange(0, Stations[CurrentStationIndex].Tracks.Num() - 1);
    TrackTimeRemaining = FMath::FRandRange(28.0f, 46.0f);
    BroadcastState();
}

void UGTTRadioComponent::TurnOff()
{
    CurrentStationIndex = -1;
    CurrentTrackIndex = 0;
    TrackTimeRemaining = 0.0f;
    BroadcastState();
}

FString UGTTRadioComponent::GetStationName() const
{
    return IsRadioOn() ? Stations[CurrentStationIndex].Name : TEXT("RADIO OFF");
}

FString UGTTRadioComponent::GetTrackTitle() const
{
    if (!IsRadioOn() || !Stations[CurrentStationIndex].Tracks.IsValidIndex(CurrentTrackIndex))
    {
        return FString();
    }
    return Stations[CurrentStationIndex].Tracks[CurrentTrackIndex];
}

FString UGTTRadioComponent::GetDisplayLine() const
{
    if (!IsRadioOn())
    {
        return TEXT("RADIO OFF | R: next station");
    }
    return FString::Printf(TEXT("RADIO | %s | NOW: %s | R: next"), *GetStationName(), *GetTrackTitle());
}

void UGTTRadioComponent::AdvanceTrack()
{
    if (!IsRadioOn())
    {
        return;
    }

    const int32 TrackCount = Stations[CurrentStationIndex].Tracks.Num();
    if (TrackCount <= 0)
    {
        return;
    }

    CurrentTrackIndex = (CurrentTrackIndex + 1) % TrackCount;
    TrackTimeRemaining = FMath::FRandRange(28.0f, 46.0f);
    BroadcastState();
}

void UGTTRadioComponent::BroadcastState()
{
    OnRadioChanged.Broadcast(GetStationName(), GetTrackTitle());
}
