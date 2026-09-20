#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GTTGameHUD.generated.h"

class AGTTVehicleBase;
class AGTTRoadVehicleNativePawn;
class AGTTFieldmasterNativePawn;

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
    FString BuildContextHint(const APawn* ControlledPawn, const AGTTVehicleBase* Vehicle, const AGTTRoadVehicleNativePawn* NativeRoad, const AGTTFieldmasterNativePawn* NativeFieldmaster) const;
    FString BuildVehicleStatus(const AGTTVehicleBase* Vehicle) const;
    FString BuildVehicleAlert(const AGTTVehicleBase* Vehicle) const;
    FString BuildNativeRoadStatus(const AGTTRoadVehicleNativePawn* Vehicle) const;
    FString BuildNativeRoadRecovery(const AGTTRoadVehicleNativePawn* Vehicle) const;
    FString BuildNativeFieldmasterStatus(const AGTTFieldmasterNativePawn* Vehicle) const;
    FString BuildNativeFieldmasterAlert(const AGTTFieldmasterNativePawn* Vehicle) const;
    void DrawRangerStopPanel();
    void DrawFarmCargoRecoveryPanel(const AGTTRoadVehicleNativePawn* NativeRoad);
    void DrawHudText(const FString& Text, const FLinearColor& Color, float X, float Y, float Scale = 1.0f);
};
