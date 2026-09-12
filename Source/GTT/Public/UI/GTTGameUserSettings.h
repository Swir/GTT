#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "GTTGameUserSettings.generated.h"

UENUM(BlueprintType)
enum class EGTTColorVisionMode : uint8
{
    Off,
    Protanopia,
    Deuteranopia,
    Tritanopia
};

UCLASS(Config=GameUserSettings)
class GTT_API UGTTGameUserSettings : public UGameUserSettings
{
    GENERATED_BODY()

public:
    UGTTGameUserSettings();

    UFUNCTION(BlueprintPure, Category="GTT|Settings") static UGTTGameUserSettings* Get();
    UFUNCTION(BlueprintCallable, Category="GTT|Settings") void ApplyGTTSettings(bool bSave=true);
    UFUNCTION(BlueprintCallable, Category="GTT|Settings") void ResetGTTSettings();

    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Settings", meta=(ClampMin="0.25", ClampMax="3.0")) float LookSensitivity = 1.0f;
    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Settings") bool bInvertLookY = false;
    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Accessibility", meta=(ClampMin="0.75", ClampMax="1.50")) float HUDScale = 1.0f;
    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Accessibility") bool bSubtitlesEnabled = true;
    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Accessibility", meta=(ClampMin="0.80", ClampMax="1.80")) float SubtitleScale = 1.0f;
    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Accessibility") bool bReduceCameraMotion = false;
    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Accessibility") EGTTColorVisionMode ColorVisionMode = EGTTColorVisionMode::Off;
    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Audio", meta=(ClampMin="0.0", ClampMax="1.0")) float MasterVolume = 1.0f;
    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Audio", meta=(ClampMin="0.0", ClampMax="1.0")) float RadioVolume = 0.75f;
    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Controller") bool bControllerVibration = true;
    UPROPERTY(Config, BlueprintReadWrite, Category="GTT|Controller", meta=(ClampMin="0.05", ClampMax="0.50")) float ControllerDeadZone = 0.18f;
};
