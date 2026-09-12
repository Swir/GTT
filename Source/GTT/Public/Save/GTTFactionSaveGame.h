#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTFactionSaveGame.generated.h"

UCLASS()
class GTT_API UGTTFactionSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 SaveVersion = 1;
    UPROPERTY(SaveGame) int32 FactionVictories = 0;
    UPROPERTY(SaveGame) int32 RustDogsDefeated = 0;
    UPROPERTY(SaveGame) int32 StoneCrowsDefeated = 0;
    UPROPERTY(SaveGame) int32 MudJackalsDefeated = 0;
};
