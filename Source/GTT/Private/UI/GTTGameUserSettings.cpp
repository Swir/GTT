#include "UI/GTTGameUserSettings.h"

#include "Engine/Engine.h"

UGTTGameUserSettings::UGTTGameUserSettings()
{
    SetToDefaults();
    LookSensitivity = 1.0f;
    bInvertLookY = false;
    HUDScale = 1.0f;
    bSubtitlesEnabled = true;
    SubtitleScale = 1.0f;
    bReduceCameraMotion = false;
    ColorVisionMode = EGTTColorVisionMode::Off;
    MasterVolume = 1.0f;
    RadioVolume = 0.75f;
    bControllerVibration = true;
    ControllerDeadZone = 0.18f;
}

UGTTGameUserSettings* UGTTGameUserSettings::Get()
{
    return GEngine ? Cast<UGTTGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UGTTGameUserSettings::ApplyGTTSettings(bool bSave)
{
    LookSensitivity = FMath::Clamp(LookSensitivity, 0.25f, 3.0f);
    HUDScale = FMath::Clamp(HUDScale, 0.75f, 1.50f);
    SubtitleScale = FMath::Clamp(SubtitleScale, 0.80f, 1.80f);
    MasterVolume = FMath::Clamp(MasterVolume, 0.0f, 1.0f);
    RadioVolume = FMath::Clamp(RadioVolume, 0.0f, 1.0f);
    ControllerDeadZone = FMath::Clamp(ControllerDeadZone, 0.05f, 0.50f);

    ApplySettings(false);
    if (bSave)
    {
        SaveSettings();
    }
}

void UGTTGameUserSettings::ResetGTTSettings()
{
    LookSensitivity = 1.0f;
    bInvertLookY = false;
    HUDScale = 1.0f;
    bSubtitlesEnabled = true;
    SubtitleScale = 1.0f;
    bReduceCameraMotion = false;
    ColorVisionMode = EGTTColorVisionMode::Off;
    MasterVolume = 1.0f;
    RadioVolume = 0.75f;
    bControllerVibration = true;
    ControllerDeadZone = 0.18f;
    ApplyGTTSettings(true);
}
