#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTCivilianIncidentSaveGame.generated.h"

/**
 * Small sidecar checkpoint for the 0.1.54 civilian dispatch.
 * It stores dispatch facts only; no actor pointer, helper, repair mutation or payout.
 */
UCLASS()
class GTT_API UGTTCivilianIncidentSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY() int32 SchemaVersion = 1;
    UPROPERTY() bool bActive = false;
    UPROPERTY() FName IncidentId = NAME_None;
    UPROPERTY() FVector IncidentLocation = FVector::ZeroVector;
    UPROPERTY() float Severity = 0.0f;
    UPROPERTY() bool bAssistanceWasInProgress = false;
};
