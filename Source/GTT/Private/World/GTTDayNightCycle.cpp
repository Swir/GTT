#include "World/GTTDayNightCycle.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "EngineUtils.h"

AGTTDayNightCycle::AGTTDayNightCycle()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTTDayNightCycle::BeginPlay()
{
    Super::BeginPlay();

    TimeOfDayHours = StartingHour;

    if (GetWorld())
    {
        for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
        {
            Sun = *It;
            break;
        }

        for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It)
        {
            Sky = *It;
            break;
        }
    }

    UpdateLighting();
}

void AGTTDayNightCycle::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (RealSecondsPerGameDay <= 0.0f)
    {
        return;
    }

    TimeOfDayHours += DeltaSeconds * (24.0f / RealSecondsPerGameDay);
    while (TimeOfDayHours >= 24.0f)
    {
        TimeOfDayHours -= 24.0f;
        ++DayNumber;
    }

    UpdateLighting();
}

FString AGTTDayNightCycle::GetClockText() const
{
    const int32 Hour = FMath::FloorToInt(TimeOfDayHours) % 24;
    const int32 Minute = FMath::FloorToInt((TimeOfDayHours - FMath::FloorToFloat(TimeOfDayHours)) * 60.0f) % 60;
    return FString::Printf(TEXT("DAY %d  %02d:%02d"), DayNumber, Hour, Minute);
}

void AGTTDayNightCycle::RestoreTime(int32 InDayNumber, float InTimeOfDayHours)
{
    DayNumber = FMath::Max(1, InDayNumber);
    TimeOfDayHours = FMath::Fmod(FMath::Max(0.0f, InTimeOfDayHours), 24.0f);
    UpdateLighting();
}

void AGTTDayNightCycle::UpdateLighting()
{
    const float SunPitch = (TimeOfDayHours / 24.0f) * 360.0f - 90.0f;
    const float DayAlpha = FMath::Clamp(FMath::Sin((TimeOfDayHours - 6.0f) / 12.0f * PI), 0.0f, 1.0f);

    if (ADirectionalLight* SunActor = Sun.Get())
    {
        SunActor->SetActorRotation(FRotator(SunPitch, -35.0f, 0.0f));
        if (UDirectionalLightComponent* Light = SunActor->GetDirectionalLightComponent())
        {
            Light->SetIntensity(FMath::Lerp(0.08f, 7.5f, DayAlpha));
        }
    }

    if (ASkyLight* SkyActor = Sky.Get())
    {
        if (USkyLightComponent* SkyLight = SkyActor->GetLightComponent())
        {
            SkyLight->SetIntensity(FMath::Lerp(0.18f, 1.25f, DayAlpha));
        }
    }
}
