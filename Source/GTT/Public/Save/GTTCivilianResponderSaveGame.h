#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTCivilianResponderSaveGame.generated.h"

/**
 * Transaction-free continuity sidecar for the 0.1.55 civilian responder.
 * It stores only incident/phase timing facts; never actor pointers, cash,
 * repair mutation, Wanted state or ranger authority.
 */
UCLASS()
class GTT_API UGTTCivilianResponderSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY() int32 SchemaVersion = 1;
    UPROPERTY() bool bActive = false;
    UPROPERTY() FName IncidentId = NAME_None;
    UPROPERTY() uint8 Phase = 0;
    UPROPERTY() float PlayerGraceElapsed = 0.0f;
    UPROPERTY() float SceneHoldRemaining = 0.0f;
};
