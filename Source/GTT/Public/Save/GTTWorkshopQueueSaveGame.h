#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GTTWorkshopQueueSaveGame.generated.h"

/**
 * Small transactional sidecar for one deferred workshop repair reservation.
 *
 * Cash and vehicle mutation remain authoritative in gameplay systems. This sidecar stores
 * only the exact target identity, locked quote and schedule so save/load cannot create a
 * second vehicle, re-price a reservation, or charge before the service actually executes.
 */
UCLASS()
class GTT_API UGTTWorkshopQueueSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame) int32 SchemaVersion = 1;
    UPROPERTY(SaveGame) bool bQueued = false;
    UPROPERTY(SaveGame) FName PersistentVehicleId = NAME_None;
    UPROPERTY(SaveGame) int32 LockedQuote = 0;
    UPROPERTY(SaveGame) int32 RequestedDay = 1;
    UPROPERTY(SaveGame) float RequestedHour = 0.0f;
    UPROPERTY(SaveGame) int32 ReadyDay = 1;
    UPROPERTY(SaveGame) float ReadyHour = 6.5f;
};
