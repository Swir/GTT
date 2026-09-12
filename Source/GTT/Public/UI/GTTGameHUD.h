#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GTTGameHUD.generated.h"

UCLASS()
class GTT_API AGTTGameHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    FString BuildWantedBar(int32 WantedLevel) const;
    FString BuildMissionText() const;
};
