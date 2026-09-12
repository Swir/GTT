#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTArc4Save.generated.h"

UCLASS()
class GTT_API UGTTArc4Save : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Arc4SaveVersion = 1;
    UPROPERTY(SaveGame) int32 Arc4Stage = 0;
    UPROPERTY(SaveGame) int32 StartingFactionVictories = 0;
    UPROPERTY(SaveGame) bool bContrabandPrepared = false;
};
