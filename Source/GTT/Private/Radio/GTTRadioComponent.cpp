#include "Radio/GTTRadioComponent.h"

#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundWaveProcedural.h"
#include "UI/GTTGameUserSettings.h"

namespace
{
    float SoftClip(float Value)
    {
        return Value / (1.0f + FMath::Abs(Value));
    }

    float Saw(float Phase)
    {
        const float Wrapped = FMath::Fmod(Phase, 1.0f);
        return Wrapped * 2.0f - 1.0f;
    }

    float Pulse(float Phase, float Width)
    {
        return FMath::Fmod(Phase, 1.0f) < Width ? 1.0f : -1.0f;
    }
}

UGTTRadioComponent::UGTTRadioComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UGTTRadioComponent::BeginPlay()
{
    Super::BeginPlay();

    Stations = {
        {TEXT("GRAVEL FM"), {TEXT("Dustline"), TEXT("Two-Lane Morning"), TEXT("Rust on the Gate"), TEXT("County Sunrise")}, 92.0f, 0},
        {TEXT("BARNBEAT 96"), {TEXT("Neon Silo"), TEXT("Midnight Combine"), TEXT("Haywire Horizon"), TEXT("Concrete Dancefloor")}, 132.0f, 1},
        {TEXT("RUST & DIESEL"), {TEXT("Cold Start"), TEXT("Grease Under Moonlight"), TEXT("County Line"), TEXT("Old Steel Heart")}, 108.0f, 2},
        {TEXT("NIGHT SHIFT"), {TEXT("Static Fields"), TEXT("After Hours"), TEXT("Last Lamp Home"), TEXT("Blue Road at 2AM")}, 76.0f, 3}
    };

    if (AActor* Owner = GetOwner())
    {
        AudioComponent = NewObject<UAudioComponent>(Owner, TEXT("GTTOriginalRadioAudio"));
        if (AudioComponent)
        {
            AudioComponent->bAutoActivate = false;
            AudioComponent->bAllowSpatialization = false;
            AudioComponent->RegisterComponent();
            if (USceneComponent* Root = Owner->GetRootComponent())
            {
                AudioComponent->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
            }
        }
    }
    RefreshAudioVolume();
}

void UGTTRadioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopAudio();
    ProceduralWave = nullptr;
    Super::EndPlay(EndPlayReason);
}

void UGTTRadioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    VolumeRefreshRemaining -= DeltaTime;
    if (VolumeRefreshRemaining <= 0.0f)
    {
        RefreshAudioVolume();
        VolumeRefreshRemaining = 0.5f;
    }

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
    TrackTimeRemaining = TrackDurationSeconds;
    StartCurrentTrackAudio();
    BroadcastState();
}

void UGTTRadioComponent::TurnOff()
{
    CurrentStationIndex = -1;
    CurrentTrackIndex = 0;
    TrackTimeRemaining = 0.0f;
    StopAudio();
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
    return FString::Printf(TEXT("RADIO | %s | NOW: %s | ORIGINAL GTT AUDIO | R: next"), *GetStationName(), *GetTrackTitle());
}

bool UGTTRadioComponent::HasAudibleProgram() const
{
    return IsRadioOn() && AudioComponent && AudioComponent->IsPlaying() && ProceduralWave;
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
    TrackTimeRemaining = TrackDurationSeconds;
    StartCurrentTrackAudio();
    BroadcastState();
}

void UGTTRadioComponent::BroadcastState()
{
    OnRadioChanged.Broadcast(GetStationName(), GetTrackTitle());
}

void UGTTRadioComponent::StartCurrentTrackAudio()
{
    StopAudio();
    if (!AudioComponent || !IsRadioOn())
    {
        return;
    }

    const FStationRuntime& Station = Stations[CurrentStationIndex];
    const FString SeedText = Station.Name + TEXT("|") + GetTrackTitle();
    const int32 Seed = static_cast<int32>(GetTypeHash(SeedText));

    ProceduralWave = NewObject<USoundWaveProcedural>(this);
    if (!ProceduralWave)
    {
        return;
    }

    ProceduralWave->SetSampleRate(RadioSampleRate);
    ProceduralWave->NumChannels = 1;
    ProceduralWave->Duration = TrackDurationSeconds;
    ProceduralWave->bLooping = false;

    TArray<int16> Samples;
    GenerateTrackPcm(Samples, TrackDurationSeconds, Seed, Station.BaseBpm, Station.SoundPalette);
    if (!Samples.IsEmpty())
    {
        ProceduralWave->QueueAudio(reinterpret_cast<const uint8*>(Samples.GetData()), Samples.Num() * sizeof(int16));
    }

    RefreshAudioVolume();
    AudioComponent->SetSound(ProceduralWave);
    AudioComponent->Play(0.0f);
}

void UGTTRadioComponent::StopAudio()
{
    if (AudioComponent)
    {
        AudioComponent->Stop();
        AudioComponent->SetSound(nullptr);
    }
}

void UGTTRadioComponent::RefreshAudioVolume()
{
    if (!AudioComponent)
    {
        return;
    }

    float Master = 1.0f;
    float Radio = 0.75f;
    if (const UGTTGameUserSettings* Settings = UGTTGameUserSettings::Get())
    {
        Master = Settings->MasterVolume;
        Radio = Settings->RadioVolume;
    }
    AudioComponent->SetVolumeMultiplier(FMath::Clamp(Master * Radio, 0.0f, 1.0f));
}

void UGTTRadioComponent::GenerateTrackPcm(TArray<int16>& OutSamples, float DurationSeconds, int32 Seed, float Bpm, int32 Palette) const
{
    const int32 SampleCount = FMath::Max(1, FMath::RoundToInt(DurationSeconds * RadioSampleRate));
    OutSamples.SetNumUninitialized(SampleCount);

    FRandomStream Random(Seed);
    const float RootChoices[] = {55.0f, 61.735f, 65.406f, 73.416f, 82.407f};
    const float Root = RootChoices[FMath::Abs(Seed) % UE_ARRAY_COUNT(RootChoices)];
    const float BeatSeconds = 60.0f / FMath::Max(40.0f, Bpm);
    const int32 Notes[] = {0, 3, 5, 7, 10, 12, 7, 5};
    const float LeadGain = Palette == 3 ? 0.13f : 0.18f;

    for (int32 Index = 0; Index < SampleCount; ++Index)
    {
        const float T = static_cast<float>(Index) / static_cast<float>(RadioSampleRate);
        const float Beat = T / BeatSeconds;
        const int32 BeatIndex = FMath::FloorToInt(Beat);
        const float BeatPhase = FMath::Frac(Beat);
        const int32 Step = BeatIndex % UE_ARRAY_COUNT(Notes);
        const float NoteHz = Root * FMath::Pow(2.0f, Notes[Step] / 12.0f);

        const float KickEnv = FMath::Exp(-BeatPhase * (Palette == 1 ? 12.0f : 9.0f));
        const float KickFreq = 48.0f + 52.0f * KickEnv;
        float Mix = FMath::Sin(TWO_PI * KickFreq * T) * KickEnv * (Palette == 3 ? 0.14f : 0.25f);

        const float BassPhase = NoteHz * 0.5f * T;
        if (Palette == 0)
        {
            Mix += FMath::Sin(TWO_PI * BassPhase) * 0.24f;
            Mix += FMath::Sin(TWO_PI * NoteHz * T) * LeadGain * (0.55f + 0.45f * FMath::Sin(TWO_PI * 0.18f * T));
            Mix += FMath::Sin(TWO_PI * NoteHz * 1.5f * T) * 0.08f;
        }
        else if (Palette == 1)
        {
            Mix += Saw(BassPhase) * 0.22f;
            Mix += Pulse(NoteHz * 2.0f * T, 0.28f) * LeadGain * (BeatPhase < 0.55f ? 1.0f : 0.35f);
            const float Offbeat = FMath::Frac(Beat + 0.5f);
            Mix += FMath::Sin(TWO_PI * NoteHz * 4.0f * T) * FMath::Exp(-Offbeat * 8.0f) * 0.10f;
        }
        else if (Palette == 2)
        {
            Mix += SoftClip(Saw(BassPhase) * 1.7f) * 0.25f;
            Mix += SoftClip(FMath::Sin(TWO_PI * NoteHz * T) * 2.4f) * LeadGain;
            Mix += FMath::Sin(TWO_PI * NoteHz * 2.01f * T) * 0.07f;
        }
        else
        {
            const float Pad = FMath::Sin(TWO_PI * NoteHz * 0.5f * T) + FMath::Sin(TWO_PI * NoteHz * 0.75f * T);
            Mix += Pad * 0.12f;
            Mix += FMath::Sin(TWO_PI * NoteHz * T + FMath::Sin(TWO_PI * 0.09f * T) * 1.8f) * LeadGain;
        }

        const bool bHatStep = (BeatIndex % 2) == 1 || Palette == 1;
        if (bHatStep && BeatPhase < 0.16f)
        {
            const float HatEnv = FMath::Exp(-BeatPhase * 28.0f);
            Mix += Random.FRandRange(-1.0f, 1.0f) * HatEnv * (Palette == 3 ? 0.025f : 0.07f);
        }

        const float BarPulse = 0.92f + FMath::Sin(TWO_PI * (Beat / 16.0f)) * 0.08f;
        const float IntroFade = FMath::Clamp(T / 1.2f, 0.0f, 1.0f);
        const float OutroFade = FMath::Clamp((DurationSeconds - T) / 0.9f, 0.0f, 1.0f);
        const float Final = FMath::Clamp(SoftClip(Mix * BarPulse) * IntroFade * OutroFade * 0.82f, -1.0f, 1.0f);
        OutSamples[Index] = static_cast<int16>(Final * 32767.0f);
    }
}
