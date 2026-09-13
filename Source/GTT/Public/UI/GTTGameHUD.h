#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GTTGameHUD.generated.h"

class AGTTVehicleBase;

UCLASS()
class GTT_API AGTTGameHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    FString BuildWantedBar(int32 WantedLevel) const;
    FString BuildMissionText() const;
    FString BuildPrimaryObjective() const;
    FString BuildContextHint(const APawn* ControlledPawn, const AGTTVehicleBase* Vehicle) const;
    FString BuildVehicleStatus(const AGTTVehicleBase* Vehicle) const;
    FString BuildVehicleAlert(const AGTTVehicleBase* Vehicle) const;
    void DrawHudText(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale = 1.0f);
};
